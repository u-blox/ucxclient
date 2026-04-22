/*
 * u_webserver.c
 * 
 * Pure C HTTP server implementation for ucxclient - EVENT-DRIVEN ARCHITECTURE
 * Platform-independent (Windows/Linux/FreeRTOS) via u_port.h abstraction
 * 
 * Uses NORA-W36's u_cx_socket API with async callbacks (not blocking BSD sockets)
 * 
 * Architecture:
 * - Listen socket registered with IncomingConnection callback
 * - Each client socket registered with DataAvailable and Closed callbacks
 * - HTTP parsing/response triggered by DataAvailable events
 * - No polling loops - fully event-driven
 * 
 * Copyright (c) 2025 u-blox
 * SPDX-License-Identifier: Apache-2.0
 */

#include "u_webserver.h"
#include "u_cx_socket.h"
#include "u_cx_log.h"
#include "u_port.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* Webserver log channel */
#define U_CX_LOG_CH_WEB    U_CX_LOG_DEBUG, "[WEB  ]"

/* ========================================
 * Internal Structures
 * ======================================== */

/**
 * Client connection state
 */
typedef struct {
    int32_t socket;                                /**< Client socket handle */
    bool active;                                   /**< Connection active */
    bool is_sse;                                   /**< SSE (keep-alive) connection */
    bool closing;                                  /**< Close initiated, ignore incoming data */
    bool pending_close;                            /**< Deferred close — Process() will call uCxSocketClose */
    char rx_buffer[U_WEBSERVER_MAX_REQUEST_SIZE];  /**< Receive buffer */
    int32_t rx_length;                             /**< Received bytes */
    bool request_complete;                         /**< Full request received */

    /* Deferred (chunked) TX state.
     *
     * URC callbacks (on_data_available, BroadcastEvent from Matter callbacks)
     * MUST NOT block on long AT roundtrips. Instead they fill tx_buffer here,
     * then uWebServerProcess() drains one chunk per call from the main loop.
     * This keeps URC dispatch fast and lets ProcessUrcs() run on time so
     * Matter UDP traffic isn't starved.
     */
    char    *tx_buffer;          /**< Heap-allocated response buffer (NULL when idle) */
    int32_t  tx_total;           /**< Total bytes to send */
    int32_t  tx_offset;          /**< Bytes already sent */
    bool     tx_close_when_done; /**< Mark pending_close once tx_buffer drains (non-SSE) */
} uWebServerClient_t;

/* Per-call chunk size for deferred sends.
 * NORA-W36 AT+USOWB max payload is 1000 bytes; one chunk per Process() call
 * keeps the main-loop blocking time bounded (~100-200ms per chunk over UART). */
#define U_WEBSERVER_TX_CHUNK_SIZE 1000

/* Forward declarations */
static void client_release_tx_buffer(uWebServerClient_t *client);
static int32_t client_drain_tx(uWebServer_t *server, uWebServerClient_t *client);

/**
 * Route entry
 */
typedef struct {
    uHttpMethod_t method;
    char path[128];
    uRouteHandler_t handler;
    void *user_data;
} uRoute_t;

/**
 * Web server context
 */
struct uWebServer {
    uCxHandle_t *ucx_handle;                       /**< uCX handle */
    int32_t listen_socket;                         /**< Listening socket */
    uint16_t port;                                 /**< TCP port */
    bool running;                                  /**< Server running */
    
    uWebServerClient_t clients[U_WEBSERVER_MAX_CLIENTS];  /**< Client connections */
    uRoute_t routes[U_WEBSERVER_MAX_ROUTES];               /**< Route handlers */
    int32_t route_count;                           /**< Number of routes */
};

/* Global server instance for callbacks (ucxclient callbacks don't support user_data) */
static uWebServer_t *g_server_instance = NULL;

/* ========================================
 * Helper Functions
 * ======================================== */

/**
 * Parse HTTP method from string
 */
static uHttpMethod_t parse_method(const char *method_str)
{
    if (strcmp(method_str, "GET") == 0) return U_HTTP_METHOD_GET;
    if (strcmp(method_str, "POST") == 0) return U_HTTP_METHOD_POST;
    if (strcmp(method_str, "PUT") == 0) return U_HTTP_METHOD_PUT;
    if (strcmp(method_str, "DELETE") == 0) return U_HTTP_METHOD_DELETE;
    if (strcmp(method_str, "OPTIONS") == 0) return U_HTTP_METHOD_OPTIONS;
    return U_HTTP_METHOD_UNKNOWN;
}

/**
 * Parse HTTP request from raw buffer
 */
static bool parse_request(const char *raw, uHttpRequest_t *request)
{
    memset(request, 0, sizeof(*request));
    
    // Parse request line: "GET /path?query HTTP/1.1\r\n"
    char method_str[16];
    char url[512];
    if (sscanf(raw, "%15s %511s", method_str, url) != 2) {
        return false;
    }
    
    request->method = parse_method(method_str);
    
    // Split path and query
    char *query_start = strchr(url, '?');
    if (query_start) {
        *query_start = '\0';
        strncpy(request->query, query_start + 1, sizeof(request->query) - 1);
    }
    strncpy(request->path, url, sizeof(request->path) - 1);
    
    // Find body (after "\r\n\r\n")
    const char *body_start = strstr(raw, "\r\n\r\n");
    if (body_start) {
        body_start += 4;
        request->body = body_start;
        request->body_length = (int32_t)strlen(body_start);
    }
    
    return true;
}

/**
 * Find matching route handler
 */
static uRoute_t *find_route(uWebServer_t *server, uHttpMethod_t method, const char *path)
{
    for (int32_t i = 0; i < server->route_count; i++) {
        if (server->routes[i].method == method && strcmp(server->routes[i].path, path) == 0) {
            return &server->routes[i];
        }
    }
    return NULL;
}

/**
 * Build HTTP response string
 */
static int32_t build_response(const uHttpResponse_t *response, char *buffer, size_t buffer_size, bool is_sse)
{
    const char *status_text = "OK";
    switch (response->status_code) {
        case 200: status_text = "OK"; break;
        case 201: status_text = "Created"; break;
        case 204: status_text = "No Content"; break;
        case 400: status_text = "Bad Request"; break;
        case 404: status_text = "Not Found"; break;
        case 500: status_text = "Internal Server Error"; break;
        default: status_text = "Unknown"; break;
    }
    
    int32_t body_len = response->body_length >= 0 ? response->body_length : 
                       (response->body ? (int32_t)strlen(response->body) : 0);
    
    const char *connection_header = is_sse ? "keep-alive" : "close";
    
    // Build headers
    char content_length_header[64] = "";
    if (!is_sse) {
        snprintf(content_length_header, sizeof(content_length_header), "Content-Length: %d\r\n", body_len);
    }
    
    int32_t written = snprintf(buffer, buffer_size,
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: %s\r\n"
        "%s"  // Content-Length (skip for SSE)
        "Connection: %s\r\n"
        "%s"  // Custom headers
        "\r\n",
        response->status_code, status_text,
        response->content_type[0] ? response->content_type : "text/plain",
        content_length_header,
        connection_header,
        response->custom_headers[0] ? response->custom_headers : ""
    );
    
    if (written < 0 || (size_t)written >= buffer_size) {
        U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "Response overflow: written=%d, buf=%llu", 
                      (int)written, (unsigned long long)buffer_size);
        return -1;
    }
    
    // Append body
    if (response->body && body_len > 0) {
        if ((size_t)(written + body_len) >= buffer_size) {
            U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "Body overflow: hdr=%d, body=%d, buf=%llu",
                          (int)written, (int)body_len, (unsigned long long)buffer_size);
            return -1;
        }
        memcpy(buffer + written, response->body, (size_t)body_len);
        written += body_len;
    }
    
    return written;
}

/**
 * Find client by socket handle
 */
static uWebServerClient_t *find_client_by_socket(uWebServer_t *server, int32_t socket)
{
    for (int32_t i = 0; i < U_WEBSERVER_MAX_CLIENTS; i++) {
        if (server->clients[i].active && server->clients[i].socket == socket) {
            return &server->clients[i];
        }
    }
    return NULL;
}

/**
 * Free a client's deferred TX buffer (if any) and clear the TX state.
 */
static void client_release_tx_buffer(uWebServerClient_t *client)
{
    if (client->tx_buffer) {
        free(client->tx_buffer);
        client->tx_buffer = NULL;
    }
    client->tx_total           = 0;
    client->tx_offset          = 0;
    client->tx_close_when_done = false;
}

/**
 * Send one chunk of a client's pending TX buffer over its socket.
 *
 * Called from uWebServerProcess() in the main loop \u2014 NOT from URC callbacks.
 * Returns:
 *   > 0  bytes sent this call
 *   = 0  nothing to send (idle)
 *   < 0  send failed; caller decides what to do (we close the client)
 *
 * One chunk per call keeps the main-loop blocking time bounded
 * (~100-200ms per AT+USOWB roundtrip over 115200 baud UART).
 */
static int32_t client_drain_tx(uWebServer_t *server, uWebServerClient_t *client)
{
    if (!client->active || !client->tx_buffer) {
        return 0;
    }

    int32_t remaining = client->tx_total - client->tx_offset;
    if (remaining <= 0) {
        // Already fully sent \u2014 finalize.
        client_release_tx_buffer(client);
        if (client->tx_close_when_done) {
            client->closing       = true;
            client->pending_close = true;
        }
        return 0;
    }

    int32_t chunk_len = (remaining > U_WEBSERVER_TX_CHUNK_SIZE) ? U_WEBSERVER_TX_CHUNK_SIZE : remaining;
    int32_t result = uCxSocketWrite(server->ucx_handle, client->socket,
                                    (const uint8_t *)(client->tx_buffer + client->tx_offset),
                                    chunk_len);
    if (result < 0) {
        U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "Deferred write failed at offset %d (socket=%d)",
                      (int)client->tx_offset, (int)client->socket);
        // Drop the buffer and tear down the client; on_socket_closed will
        // also fire if the remote closed, which is harmless (idempotent).
        client_release_tx_buffer(client);
        client->closing       = true;
        client->pending_close = true;
        return result;
    }

    client->tx_offset += result;

    // If this was the last chunk, finalize on this call so the next
    // Process() doesn't have to spin once more just to mark for close.
    if (client->tx_offset >= client->tx_total) {
        client_release_tx_buffer(client);
        if (client->tx_close_when_done) {
            client->closing       = true;
            client->pending_close = true;
        }
    }

    return result;
}

/**
 * Process complete HTTP request and send response
 */
static void handle_client_request(uWebServer_t *server, uWebServerClient_t *client)
{
    uHttpRequest_t request;
    uHttpResponse_t response;
    memset(&response, 0, sizeof(response));
    response.status_code = 404;
    response.body_length = -1;  // -1 means "use strlen(body)" - IMPORTANT!
    strcpy(response.content_type, "text/plain");
    response.body = "Not Found";
    
    bool parsed = parse_request(client->rx_buffer, &request);
    
    U_CX_LOG_LINE(U_CX_LOG_CH_WEB, "Request: %s %s", 
                  request.method == 1 ? "GET" : request.method == 2 ? "POST" : "?", 
                  request.path);
    
    if (parsed) {
        // Find route handler
        uRoute_t *route = find_route(server, request.method, request.path);
        if (route) {
            // Call handler
            route->handler(&request, &response, route->user_data);
        } else {
            U_CX_LOG_LINE(U_CX_LOG_CH_WEB, "Route not found: %s", request.path);
        }
    } else {
        U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "Failed to parse HTTP request");
    }
    
    // Check if this is an SSE connection (based on content-type)
    bool is_sse = (strstr(response.content_type, "text/event-stream") != NULL);
    client->is_sse = is_sse;
    
    // Build and send response
    // IMPORTANT: tx_buffer is heap-allocated because this function may run on
    // FreeRTOS UrcTask (4KB stack) — 32KB on stack would overflow instantly.
    char *tx_buffer = (char *)malloc(U_WEBSERVER_MAX_RESPONSE_SIZE);
    if (!tx_buffer) {
        U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "Failed to allocate %d bytes for response", (int)U_WEBSERVER_MAX_RESPONSE_SIZE);
        client->active = false;
        return;
    }
    int32_t response_len = build_response(&response, tx_buffer, U_WEBSERVER_MAX_RESPONSE_SIZE, is_sse);

    U_CX_LOG_LINE(U_CX_LOG_CH_WEB, "Response: %d %s (%d bytes) socket=%d",
                  response.status_code, response.content_type, (int)response_len, (int)client->socket);

    if (response_len <= 0) {
        // Nothing to send (build_response failed or empty body) \u2014 drop response,
        // close non-SSE clients so the browser doesn't hang.
        free(tx_buffer);
        if (!is_sse) {
            client->closing = true;
            client->pending_close = true;
        }
        return;
    }

    /* Hand the response off to the deferred TX state machine.
     *
     * Why: sending here would do ~30-40 sequential AT+USOWB roundtrips
     * (~3-8 seconds total) inside URC dispatch context, which blocks
     * uCxAtClientProcessUrcs() and starves Matter UDP traffic — enough to
     * make Apple Home commissioning fail with the dashboard open.
     * uWebServerProcess() will drain one chunk per main-loop iteration. */
    if (client->tx_buffer) {
        // A previous response is still in flight on this client. This shouldn't
        // happen for HTTP (one request -> one response, then close) but could
        // happen for SSE if the browser pipelines requests. Drop the old buffer.
        client_release_tx_buffer(client);
    }
    client->tx_buffer          = tx_buffer;
    client->tx_total           = response_len;
    client->tx_offset          = 0;
    client->tx_close_when_done = !is_sse;

    if (is_sse) {
        // Reset for next message (SSE can receive multiple events)
        client->rx_length = 0;
        client->request_complete = false;
    }
    // For non-SSE: client->pending_close will be set by client_drain_tx() once
    // the response has fully drained, NOT here — closing now would race the send.
}

/* ========================================
 * ucxclient Event Callbacks
 * ======================================== */

/**
 * Callback: New client connection accepted
 */
static void on_incoming_connection(struct uCxHandle *puCxHandle, int32_t client_socket, 
                                   uSockIpAddress_t *remote_ip, int32_t listening_socket)
{
    (void)remote_ip;  // Unused
    (void)listening_socket;  // Unused
    
    if (!g_server_instance) return;
    
    // Find free client slot
    uWebServerClient_t *client = NULL;
    for (int32_t i = 0; i < U_WEBSERVER_MAX_CLIENTS; i++) {
        if (!g_server_instance->clients[i].active) {
            client = &g_server_instance->clients[i];
            break;
        }
    }
    
    if (!client) {
        // All slots full — evict oldest non-SSE client to make room
        // If all clients are SSE, evict the first SSE client (stale browser tab)
        uWebServerClient_t *victim = NULL;
        for (int32_t i = 0; i < U_WEBSERVER_MAX_CLIENTS; i++) {
            if (g_server_instance->clients[i].active && !g_server_instance->clients[i].is_sse) {
                victim = &g_server_instance->clients[i];
                break;
            }
        }
        if (!victim) {
            // All clients are SSE — evict first one (stale tab)
            for (int32_t i = 0; i < U_WEBSERVER_MAX_CLIENTS; i++) {
                if (g_server_instance->clients[i].active) {
                    victim = &g_server_instance->clients[i];
                    break;
                }
            }
        }
        if (victim) {
            U_CX_LOG_LINE(U_CX_LOG_CH_WARN, "WebServer: evicting client socket=%d (is_sse=%d) for new connection",
                          (int)victim->socket, (int)victim->is_sse);
            // Defer eviction close to Process()
            victim->closing = true;
            victim->pending_close = true;
            client = victim;
        } else {
            // Should not happen, but safety fallback
            U_CX_LOG_LINE(U_CX_LOG_CH_WARN, "WebServer: no free client slots, rejecting socket=%d",
                          (int)client_socket);
            // Cannot defer — no client slot to store pending_close. Must close inline.
            // This is rare (all slots occupied + pending_close) and acceptable.
            uCxSocketClose(puCxHandle, client_socket);
            return;
        }
    }
    
    // Initialize client
    client->socket = client_socket;
    client->active = true;

    // Disable Nagle's algorithm for faster HTTP responses (small packets sent immediately)
    (void)uCxSocketSetOption(puCxHandle, client_socket, U_SOCKET_OPTION_NO_DELAY, 1);

    client->is_sse = false;
    client->closing = false;
    client->pending_close = false;
    client->rx_length = 0;
    client->request_complete = false;
    client->tx_buffer = NULL;
    client->tx_total = 0;
    client->tx_offset = 0;
    client->tx_close_when_done = false;
    memset(client->rx_buffer, 0, sizeof(client->rx_buffer));
}

/**
 * Callback: Data available to read
 */
static void on_data_available(struct uCxHandle *puCxHandle, int32_t socket, int32_t bytes_available)
{
    (void)bytes_available;
    
    if (!g_server_instance) {
        return;
    }
    
    uWebServerClient_t *client = find_client_by_socket(g_server_instance, socket);
    if (!client) {
        return;
    }
    
    // Read available data (max 1000 bytes per NORA-W36 AT command limit)
    int32_t rx_space = U_WEBSERVER_MAX_REQUEST_SIZE - client->rx_length - 1;
    if (rx_space > 1000) rx_space = 1000;  // NORA-W36 max read size
    if (rx_space <= 0) {
        // Buffer full — defer close to Process()
        client->closing = true;
        client->pending_close = true;
        return;
    }
    
    uint8_t *rx_ptr = (uint8_t*)(client->rx_buffer + client->rx_length);
    int32_t result = uCxSocketRead(puCxHandle, socket, rx_space, rx_ptr);
    
    if (result > 0) {
        client->rx_length += result;
        client->rx_buffer[client->rx_length] = '\0';
        
        // Check for complete HTTP request (ends with "\r\n\r\n")
        if (strstr(client->rx_buffer, "\r\n\r\n")) {
            client->request_complete = true;
            handle_client_request(g_server_instance, client);
        }
    } else if (result < 0) {
        // Read error — defer close to Process()
        client->closing = true;
        client->pending_close = true;
    }
}

/**
 * Callback: Socket closed by remote or error
 */
static void on_socket_closed(struct uCxHandle *puCxHandle, int32_t socket)
{
    (void)puCxHandle;  // Unused
    
    if (!g_server_instance) return;
    
    uWebServerClient_t *client = find_client_by_socket(g_server_instance, socket);
    if (client) {
        client->active = false;
        client_release_tx_buffer(client);
    }
}

/* ========================================
 * API Implementation
 * ======================================== */

uWebServer_t *uWebServerInit(uCxHandle_t *ucx_handle, uint16_t port)
{
    if (!ucx_handle) {
        return NULL;
    }
    
    uWebServer_t *server = (uWebServer_t *)malloc(sizeof(uWebServer_t));
    if (!server) {
        return NULL;
    }
    
    memset(server, 0, sizeof(*server));
    server->ucx_handle = ucx_handle;
    server->port = port;
    server->listen_socket = -1;
    
    // Set global instance for callbacks
    g_server_instance = server;
    
    return server;
}

int32_t uWebServerAddRoute(uWebServer_t *server, uHttpMethod_t method, const char *path,
                           uRouteHandler_t handler, void *user_data)
{
    if (!server || !path || !handler) {
        return -1;
    }
    
    if (server->route_count >= U_WEBSERVER_MAX_ROUTES) {
        return -2;  // Too many routes
    }
    
    uRoute_t *route = &server->routes[server->route_count];
    route->method = method;
    strncpy(route->path, path, sizeof(route->path) - 1);
    route->handler = handler;
    route->user_data = user_data;
    
    server->route_count++;
    return 0;
}

int32_t uWebServerStart(uWebServer_t *server)
{
    if (!server || server->running) {
        return -1;
    }
    
    // NOTE: We do NOT register callbacks here anymore!
    // The UcxClientManager registers multiplexed callbacks that dispatch
    // to us via uWebServerHandle*() functions. This allows UDP and TCP
    // to coexist without overwriting each other's callbacks.
    
    // Use DIRECT BINARY mode (USORM=2) for ALL sockets:
    // - UDP: +UESODBF fires with data inline (no uCxSocketRead needed)
    // - TCP: +UESODB fires as notification, we call uCxSocketRead to get data
    // Both work in direct mode, just different callback handling.
    
    // Create TCP listen socket (uses current USORM setting - direct binary)
    int32_t result = uCxSocketCreate1(server->ucx_handle, U_SOCKET_PROTOCOL_TCP, &server->listen_socket);
    
    if (result < 0) {
        return result;
    }
    
    // Bind to port
    result = uCxSocketBind(server->ucx_handle, server->listen_socket, (int32_t)server->port);
    if (result < 0) {
        uCxSocketClose(server->ucx_handle, server->listen_socket);
        server->listen_socket = -1;
        return result;
    }
    
    // Listen for connections (event-driven - IncomingConnection callback will fire)
    result = uCxSocketListen1(server->ucx_handle, server->listen_socket);
    if (result < 0) {
        uCxSocketClose(server->ucx_handle, server->listen_socket);
        server->listen_socket = -1;
        return result;
    }
    
    server->running = true;
    return 0;
}

int32_t uWebServerProcess(uWebServer_t *server)
{
    if (!server || !server->running) {
        return -1;
    }

    // Drain pending response/SSE chunks first — one chunk per client per call.
    // This is the deferred half of handle_client_request(): URC callbacks queue
    // bytes here, the main loop sends them. Keeps URC dispatch fast and avoids
    // multi-second AT command storms blocking Matter UDP traffic.
    for (int32_t i = 0; i < U_WEBSERVER_MAX_CLIENTS; i++) {
        if (server->clients[i].active && server->clients[i].tx_buffer) {
            (void)client_drain_tx(server, &server->clients[i]);
        }
    }

    // Process deferred socket closes.
    // Sockets are marked pending_close in handle_client_request() instead of
    // calling uCxSocketClose() synchronously inside URC callbacks. This avoids
    // a 20-second AT timeout when NORA-W36 generates URCs (+UESOIC for new
    // connections) before sending the OK response to AT+USOCL.
    //
    // NOTE: If the remote (browser) closed the connection first, +UESOCL fires
    // and uWebServerHandleSocketClosed() clears pending_close, so we skip the
    // AT+USOCL here entirely. The check below is the safety net for cases where
    // we want to close before the remote does.
    for (int32_t i = 0; i < U_WEBSERVER_MAX_CLIENTS; i++) {
        if (server->clients[i].pending_close) {
            int32_t sock = server->clients[i].socket;
            server->clients[i].pending_close = false;
            server->clients[i].active = false;
            // Release any TX buffer that survived (defensive — normally drained already).
            client_release_tx_buffer(&server->clients[i]);

            int32_t err = uCxSocketClose(server->ucx_handle, sock);
            // ERROR:11 (-11) means "socket already closed" — harmless race with
            // a +UESOCL URC that arrived between our pending_close check and
            // the AT+USOCL command. Don't log this as an issue.
            if (err < 0 && err != -11) {
                U_CX_LOG_LINE(U_CX_LOG_CH_WEB, "Close socket %d returned %d",
                              (int)sock, (int)err);
            }
        }
    }
    
    // Return number of active clients
    int32_t active_count = 0;
    for (int32_t i = 0; i < U_WEBSERVER_MAX_CLIENTS; i++) {
        if (server->clients[i].active) {
            active_count++;
        }
    }
    
    return active_count;
}

int32_t uWebServerStop(uWebServer_t *server)
{
    if (!server || !server->running) {
        return -1;
    }
    
    // NOTE: Do NOT unregister global ucxclient callbacks here!
    // UcxClientManager owns the multiplexed callbacks and dispatches
    // to us via RegisterTcpHandler(). Calling uCxSocketRegister*() with
    // NULL would wipe out ALL socket event handling (including UDP/Matter).
    
    // Close all client connections first (frees port 80 for rebind)
    for (int32_t i = 0; i < U_WEBSERVER_MAX_CLIENTS; i++) {
        if (server->clients[i].active || server->clients[i].pending_close) {
            U_CX_LOG_LINE(U_CX_LOG_CH_WEB, "Closing client socket %d", (int)server->clients[i].socket);
            uCxSocketClose(server->ucx_handle, server->clients[i].socket);
            server->clients[i].active = false;
            server->clients[i].is_sse = false;
            server->clients[i].closing = false;
            server->clients[i].pending_close = false;
            server->clients[i].rx_length = 0;
            server->clients[i].request_complete = false;
            client_release_tx_buffer(&server->clients[i]);
        }
    }
    
    // Close listening socket
    if (server->listen_socket >= 0) {
        U_CX_LOG_LINE(U_CX_LOG_CH_WEB, "Closing listen socket %d", (int)server->listen_socket);
        uCxSocketClose(server->ucx_handle, server->listen_socket);
        server->listen_socket = -1;
    }
    
    server->running = false;
    return 0;
}

void uWebServerFree(uWebServer_t *server)
{
    if (server) {
        uWebServerStop(server);
        
        if (g_server_instance == server) {
            g_server_instance = NULL;
        }
        
        free(server);
    }
}

/* ========================================
 * Helper Function Implementations
 * ======================================== */

void uWebServerSendJSON(uHttpResponse_t *response, const char *json_string)
{
    response->status_code = 200;
    strcpy(response->content_type, "application/json");
    response->body = json_string;
    response->body_length = -1;  // Use strlen
}

void uWebServerSendText(uHttpResponse_t *response, const char *text)
{
    response->status_code = 200;
    strcpy(response->content_type, "text/plain");
    response->body = text;
    response->body_length = -1;
}

void uWebServerSendHTML(uHttpResponse_t *response, const char *html)
{
    response->status_code = 200;
    strcpy(response->content_type, "text/html");
    response->body = html;
    response->body_length = -1;
}

void uWebServerSendError(uHttpResponse_t *response, int32_t status_code, const char *message)
{
    response->status_code = status_code;
    strcpy(response->content_type, "text/plain");
    response->body = message;
    response->body_length = -1;
}

int32_t uWebServerGetQueryParam(const uHttpRequest_t *request, const char *param, char *value, size_t max_len)
{
    if (!request || !param || !value || max_len == 0) {
        return -1;
    }
    
    // Find "param=" in query string
    char search[128];
    snprintf(search, sizeof(search), "%s=", param);
    
    const char *start = strstr(request->query, search);
    if (!start) {
        return -2;  // Not found
    }
    
    start += strlen(search);
    const char *end = strchr(start, '&');
    size_t len = end ? (size_t)(end - start) : strlen(start);
    
    if (len >= max_len) {
        len = max_len - 1;
    }
    
    memcpy(value, start, len);
    value[len] = '\0';
    
    return 0;
}

/* ========================================
 * SSE Implementation (Event-Driven)
 * ======================================== */

/* Static buffer for SSE initial event - we need it to survive after handler returns */
static char g_sse_initial_event[256];

uSseClient_t uWebServerRegisterSSE(uWebServer_t *server, const uHttpRequest_t *request, uHttpResponse_t *response)
{
    (void)request;  // Unused
    
    if (!server || !response) {
        return -1;
    }
    
    // Configure SSE response
    response->status_code = 200;
    strcpy(response->content_type, "text/event-stream");
    snprintf(response->custom_headers, sizeof(response->custom_headers),
             "Cache-Control: no-cache\r\n"
             "Access-Control-Allow-Origin: *\r\n");
    
    // Build initial event as the body (sent with headers)
    // Format: "event: connected\ndata: {\"message\":\"Event stream ready\"}\n\n"
    snprintf(g_sse_initial_event, sizeof(g_sse_initial_event),
             "event: connected\ndata: {\"message\":\"Event stream ready\"}\n\n");
    response->body = g_sse_initial_event;
    response->body_length = (int32_t)strlen(g_sse_initial_event);
    
    // Find which client slot this is and MARK AS SSE NOW
    // (so is_sse is true before the handler returns)
    for (int32_t i = 0; i < U_WEBSERVER_MAX_CLIENTS; i++) {
        if (server->clients[i].active && !server->clients[i].is_sse) {
            // Mark as SSE immediately so count is correct
            server->clients[i].is_sse = true;
            return i;
        }
    }
    
    return -2;  // No active client found
}

int32_t uWebServerSendSSEEvent(uWebServer_t *server, uSseClient_t client, const char *event_name, const char *data)
{
    if (!server || client < 0 || client >= U_WEBSERVER_MAX_CLIENTS) {
        return -1;
    }
    
    if (!server->clients[client].active || !server->clients[client].is_sse) {
        return -2;  // Client not connected or not SSE
    }
    
    // Build SSE event message
    char event_buffer[U_WEBSERVER_SSE_EVENT_SIZE];
    int32_t len = 0;
    
    if (event_name) {
        len += snprintf(event_buffer + len, sizeof(event_buffer) - (size_t)len, "event: %s\n", event_name);
    }
    
    if (data) {
        // Split multi-line data into "data: " prefixed lines
        const char *line_start = data;
        while (*line_start) {
            const char *line_end = strchr(line_start, '\n');
            int32_t line_len = line_end ? (int32_t)(line_end - line_start) : (int32_t)strlen(line_start);
            
            len += snprintf(event_buffer + len, sizeof(event_buffer) - (size_t)len, "data: %.*s\n", line_len, line_start);
            
            if (!line_end) break;
            line_start = line_end + 1;
        }
    }
    
    // Event terminator
    len += snprintf(event_buffer + len, sizeof(event_buffer) - (size_t)len, "\n");

    if (len <= 0) {
        return -3;
    }

    /* Enqueue into the client's deferred TX buffer rather than writing inline.
     *
     * BroadcastEvent() is called from Matter cluster callbacks (e.g. attribute
     * changed, commissioning progress) which run with the chip stack lock held.
     * A synchronous AT+USOWB here would block the main loop ~100-200ms per call,
     * and worse, can chain into many such calls during burst events. Drop the
     * event if a previous one is still draining — SSE is best-effort and the
     * next status tick will catch up.
     */
    uWebServerClient_t *c = &server->clients[client];
    if (c->tx_buffer) {
        // A previous event/response is still in flight on this SSE client.
        // Coalesce: drop this event. The browser will get the next one.
        return -4;
    }

    char *buf = (char *)malloc((size_t)len);
    if (!buf) {
        return -5;
    }
    memcpy(buf, event_buffer, (size_t)len);

    c->tx_buffer          = buf;
    c->tx_total           = len;
    c->tx_offset          = 0;
    c->tx_close_when_done = false;  // SSE: keep the connection open
    return 0;
}

int32_t uWebServerBroadcastSSEEvent(uWebServer_t *server, const char *event_name, const char *data)
{
    if (!server) {
        return -1;
    }
    
    int32_t sent_count = 0;
    
    for (int32_t i = 0; i < U_WEBSERVER_MAX_CLIENTS; i++) {
        if (server->clients[i].active && server->clients[i].is_sse) {
            if (uWebServerSendSSEEvent(server, i, event_name, data) == 0) {
                sent_count++;
            }
        }
    }
    
    return sent_count;
}

int32_t uWebServerCloseSSE(uWebServer_t *server, uSseClient_t client)
{
    if (!server || client < 0 || client >= U_WEBSERVER_MAX_CLIENTS) {
        return -1;
    }
    
    if (server->clients[client].active) {
        uCxSocketClose(server->ucx_handle, server->clients[client].socket);
        server->clients[client].active = false;
        client_release_tx_buffer(&server->clients[client]);
    }
    
    return 0;
}

int32_t uWebServerGetSSEClientCount(uWebServer_t *server)
{
    if (!server) {
        return -1;
    }
    
    int32_t count = 0;
    for (int32_t i = 0; i < U_WEBSERVER_MAX_CLIENTS; i++) {
        if (server->clients[i].active && server->clients[i].is_sse) {
            count++;
        }
    }
    
    return count;
}

/* ========================================
 * External Event Handler API
 * 
 * These functions allow integration with a socket multiplexer.
 * Called by UcxClientManager when socket events occur.
 * ======================================== */

void uWebServerHandleIncomingConnection(uWebServer_t *server, struct uCxHandle *puCxHandle,
                                        int32_t clientSocket, uSockIpAddress_t *remoteIp, 
                                        int32_t listenSocket)
{
    (void)remoteIp;
    (void)listenSocket;
    
    if (!server || !server->running) {
        return;
    }
    
    // Find free client slot
    uWebServerClient_t *client = NULL;
    for (int32_t i = 0; i < U_WEBSERVER_MAX_CLIENTS; i++) {
        if (!server->clients[i].active) {
            client = &server->clients[i];
            break;
        }
    }
    
    if (!client) {
        // All slots full — evict oldest non-SSE client to make room
        uWebServerClient_t *victim = NULL;
        for (int32_t i = 0; i < U_WEBSERVER_MAX_CLIENTS; i++) {
            if (server->clients[i].active && !server->clients[i].is_sse) {
                victim = &server->clients[i];
                break;
            }
        }
        if (!victim) {
            // All clients are SSE — evict first one (stale browser tab)
            for (int32_t i = 0; i < U_WEBSERVER_MAX_CLIENTS; i++) {
                if (server->clients[i].active) {
                    victim = &server->clients[i];
                    break;
                }
            }
        }
        if (victim) {
            U_CX_LOG_LINE(U_CX_LOG_CH_WARN, "WebServer: evicting client socket=%d (is_sse=%d) for new connection",
                          (int)victim->socket, (int)victim->is_sse);
            // Defer eviction close to Process()
            victim->closing = true;
            victim->pending_close = true;
            client = victim;
        } else {
            U_CX_LOG_LINE(U_CX_LOG_CH_WARN, "WebServer: no free client slots, rejecting socket=%d",
                          (int)clientSocket);
            // Cannot defer — no client slot. Must close inline (rare).
            uCxSocketClose(puCxHandle, clientSocket);
            return;
        }
    }
    
    // Initialize client
    client->socket = clientSocket;
    client->active = true;
    client->is_sse = false;
    client->closing = false;
    client->pending_close = false;
    client->rx_length = 0;
    client->request_complete = false;
    memset(client->rx_buffer, 0, sizeof(client->rx_buffer));
    
    U_CX_LOG_LINE(U_CX_LOG_CH_WEB, "Client connected: socket=%d", (int)clientSocket);
}

void uWebServerHandleDataAvailable(uWebServer_t *server, struct uCxHandle *puCxHandle,
                                   int32_t socketHandle, int32_t bytesAvailable)
{
    (void)bytesAvailable;
    
    if (!server || !server->running) {
        return;
    }
    
    uWebServerClient_t *client = find_client_by_socket(server, socketHandle);
    if (!client) {
        return;
    }
    
    // Read available data (max 1000 bytes per NORA-W36 AT command limit)
    int32_t rx_space = U_WEBSERVER_MAX_REQUEST_SIZE - client->rx_length - 1;
    if (rx_space > 1000) rx_space = 1000;  // NORA-W36 max read size
    if (rx_space <= 0) {
        // Buffer full — defer close to Process()
        client->closing = true;
        client->pending_close = true;
        return;
    }
    
    uint8_t *rx_ptr = (uint8_t*)(client->rx_buffer + client->rx_length);
    int32_t result = uCxSocketRead(puCxHandle, socketHandle, rx_space, rx_ptr);
    
    if (result > 0) {
        client->rx_length += result;
        client->rx_buffer[client->rx_length] = '\0';
        
        // Check for complete HTTP request (ends with "\r\n\r\n")
        if (strstr(client->rx_buffer, "\r\n\r\n")) {
            client->request_complete = true;
            handle_client_request(server, client);
        }
    } else if (result < 0) {
        // Read error — defer close to Process()
        client->closing = true;
        client->pending_close = true;
    }
}

// Handle binary data received in DIRECT BINARY mode (USORM=2)
// Data is provided inline - no uCxSocketRead needed!
void uWebServerHandleBinaryData(uWebServer_t *server, struct uCxHandle *puCxHandle,
                                int32_t socketHandle, uint8_t *pData, size_t dataLen)
{
    (void)puCxHandle;  // Not needed - we have the data already
    
    if (!server || !server->running) {
        return;
    }
    
    uWebServerClient_t *client = find_client_by_socket(server, socketHandle);
    if (!client) {
        return;
    }
    
    // Calculate how much we can copy
    int32_t rx_space = U_WEBSERVER_MAX_REQUEST_SIZE - client->rx_length - 1;
    if (rx_space <= 0) {
        // Buffer full — defer close to Process()
        client->closing = true;
        client->pending_close = true;
        return;
    }
    
    // Copy as much as we can fit
    size_t copy_len = (dataLen < (size_t)rx_space) ? dataLen : (size_t)rx_space;
    memcpy(client->rx_buffer + client->rx_length, pData, copy_len);
    client->rx_length += (int32_t)copy_len;
    client->rx_buffer[client->rx_length] = '\0';
    
    // Check for complete HTTP request (ends with "\r\n\r\n")
    if (strstr(client->rx_buffer, "\r\n\r\n")) {
        client->request_complete = true;
        handle_client_request(server, client);
    }
}

void uWebServerHandleSocketClosed(uWebServer_t *server, struct uCxHandle *puCxHandle,
                                  int32_t socketHandle)
{
    (void)puCxHandle;
    
    if (!server) return;
    
    uWebServerClient_t *client = find_client_by_socket(server, socketHandle);
    if (client) {
        // Remote closed first — clear ALL state so Process() does NOT issue
        // a redundant AT+USOCL (which returns ERROR:11 "already closed").
        client->active = false;
        client->pending_close = false;
        client->is_sse = false;
        client->closing = false;
        client->rx_length = 0;
        client->request_complete = false;
        client_release_tx_buffer(client);
    }
}

bool uWebServerOwnsSocket(uWebServer_t *server, int32_t socketHandle)
{
    if (!server || !server->running) return false;
    
    // Check if it's the listen socket
    if (socketHandle == server->listen_socket) {
        return true;
    }
    
    // Check if it's a client socket. Match on socket handle alone — do NOT
    // gate on `active`. A client may still have pending_close=true after the
    // response was sent (active flipped to false by handle_client_request).
    // The URC dispatcher must still recognize this socket as ours so it
    // routes the +UESOCL to us, letting us clear pending_close and avoid a
    // redundant AT+USOCL (which would return ERROR:11).
    for (int32_t i = 0; i < U_WEBSERVER_MAX_CLIENTS; i++) {
        if (server->clients[i].socket == socketHandle &&
            (server->clients[i].active || server->clients[i].pending_close)) {
            return true;
        }
    }
    
    return false;
}

int32_t uWebServerGetListenSocket(uWebServer_t *server)
{
    if (!server) return -1;
    return server->listen_socket;
}
