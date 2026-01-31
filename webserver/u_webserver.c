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
#include "u_port.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

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
    char rx_buffer[U_WEBSERVER_MAX_REQUEST_SIZE];  /**< Receive buffer */
    int32_t rx_length;                             /**< Received bytes */
    bool request_complete;                         /**< Full request received */
} uWebServerClient_t;

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
    printf("[WebServer] build_response: status=%d, body=%p, body_length=%d, content_type='%s'\n",
           response->status_code, (void*)response->body, (int)response->body_length, response->content_type);
    fflush(stdout);
    
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
    
    printf("[WebServer] build_response: body_len=%d, buffer_size=%llu\n", (int)body_len, (unsigned long long)buffer_size);
    fflush(stdout);
    
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
    
    printf("[WebServer] build_response: snprintf returned %d\n", (int)written);
    fflush(stdout);
    
    if (written < 0 || (size_t)written >= buffer_size) {
        printf("[WebServer] build_response: OVERFLOW! written=%d, buffer_size=%llu\n", (int)written, (unsigned long long)buffer_size);
        fflush(stdout);
        return -1;  // Buffer overflow
    }
    
    // Append body
    if (response->body && body_len > 0) {
        if ((size_t)(written + body_len) >= buffer_size) {
            printf("[WebServer] build_response: BODY OVERFLOW! written=%d, body_len=%d, buffer_size=%llu\n",
                   (int)written, (int)body_len, (unsigned long long)buffer_size);
            fflush(stdout);
            return -1;  // Buffer overflow
        }
        memcpy(buffer + written, response->body, (size_t)body_len);
        written += body_len;
    }
    
    printf("[WebServer] build_response: SUCCESS, total=%d bytes\n", (int)written);
    fflush(stdout);
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
    
    if (parsed) {
        // Find route handler
        uRoute_t *route = find_route(server, request.method, request.path);
        if (route) {
            // Call handler
            route->handler(&request, &response, route->user_data);
        }
    }
    
    // Check if this is an SSE connection (based on content-type)
    bool is_sse = (strstr(response.content_type, "text/event-stream") != NULL);
    client->is_sse = is_sse;
    
    // Build and send response
    char tx_buffer[U_WEBSERVER_MAX_RESPONSE_SIZE];
    int32_t response_len = build_response(&response, tx_buffer, sizeof(tx_buffer), is_sse);
    
    printf("[WebServer] Response: status=%d, len=%d\n", response.status_code, (int)response_len);
    fflush(stdout);
    
    if (response_len > 0) {
        printf("[WebServer] Calling uCxSocketWrite(socket=%d, len=%d)...\n", 
               (int)client->socket, (int)response_len);
        printf("[WebServer] Response data (first 200 chars):\n%.200s\n", tx_buffer);
        fflush(stdout);
        int32_t result = uCxSocketWrite(server->ucx_handle, client->socket, 
                                        (const uint8_t*)tx_buffer, response_len);
        printf("[WebServer] uCxSocketWrite returned: %d\n", (int)result);
        fflush(stdout);
        if (result < 0) {
            // Send failed, close connection
            printf("[WebServer] Write failed, closing socket\n");
            fflush(stdout);
            uCxSocketClose(server->ucx_handle, client->socket);
            client->active = false;
            return;
        }
    } else {
        printf("[WebServer] No response to send (response_len=%d)\n", (int)response_len);
        fflush(stdout);
    }
    
    // Close connection unless it's SSE
    if (!is_sse) {
        uCxSocketClose(server->ucx_handle, client->socket);
        client->active = false;
    } else {
        // Reset for next message (SSE can receive multiple events)
        client->rx_length = 0;
        client->request_complete = false;
    }
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
        // No free slots, reject connection
        uCxSocketClose(puCxHandle, client_socket);
        return;
    }
    
    // Initialize client
    client->socket = client_socket;
    client->active = true;
    client->is_sse = false;
    client->rx_length = 0;
    client->request_complete = false;
    memset(client->rx_buffer, 0, sizeof(client->rx_buffer));
}

/**
 * Callback: Data available to read
 */
static void on_data_available(struct uCxHandle *puCxHandle, int32_t socket, int32_t bytes_available)
{
    printf("[WebServer] on_data_available: socket=%d, bytes=%d\n", (int)socket, (int)bytes_available);
    
    if (!g_server_instance) {
        printf("[WebServer] ERROR: g_server_instance is NULL\n");
        return;
    }
    
    uWebServerClient_t *client = find_client_by_socket(g_server_instance, socket);
    if (!client) {
        printf("[WebServer] ERROR: No client found for socket %d\n", (int)socket);
        return;
    }
    
    // Read available data (max 1000 bytes per NORA-W36 AT command limit)
    int32_t rx_space = U_WEBSERVER_MAX_REQUEST_SIZE - client->rx_length - 1;
    if (rx_space > 1000) rx_space = 1000;  // NORA-W36 max read size
    if (rx_space <= 0) {
        // Buffer full, close connection
        printf("[WebServer] Buffer full, closing connection\n");
        uCxSocketClose(puCxHandle, socket);
        client->active = false;
        return;
    }
    
    uint8_t *rx_ptr = (uint8_t*)(client->rx_buffer + client->rx_length);
    printf("[WebServer] Calling uCxSocketRead(socket=%d, len=%d)...\n", (int)socket, (int)rx_space);
    int32_t result = uCxSocketRead(puCxHandle, socket, rx_space, rx_ptr);
    printf("[WebServer] uCxSocketRead returned: %d\n", (int)result);
    
    if (result > 0) {
        client->rx_length += result;
        client->rx_buffer[client->rx_length] = '\0';
        printf("[WebServer] Received %d bytes, total %d. Request so far:\n%s\n", 
               (int)result, (int)client->rx_length, client->rx_buffer);
        
        // Check for complete HTTP request (ends with "\r\n\r\n")
        if (strstr(client->rx_buffer, "\r\n\r\n")) {
            printf("[WebServer] Complete request received, processing...\n");
            client->request_complete = true;
            handle_client_request(g_server_instance, client);
        } else {
            printf("[WebServer] Request incomplete, waiting for more data...\n");
        }
    } else if (result < 0) {
        // Read error, close connection
        printf("[WebServer] Read error (%d), closing connection\n", (int)result);
        uCxSocketClose(puCxHandle, socket);
        client->active = false;
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
    
    // Create TCP socket
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
    
    // Event-driven architecture - nothing to do here!
    // All processing happens in callbacks:
    // - on_incoming_connection() handles new clients
    // - on_data_available() reads data and processes requests
    // - on_socket_closed() cleans up disconnected clients
    
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
    
    // Unregister callbacks
    uCxSocketRegisterIncomingConnection(server->ucx_handle, NULL);
    uCxSocketRegisterDataAvailable(server->ucx_handle, NULL);
    uCxSocketRegisterClosed(server->ucx_handle, NULL);
    
    // Close all client connections
    for (int32_t i = 0; i < U_WEBSERVER_MAX_CLIENTS; i++) {
        if (server->clients[i].active) {
            uCxSocketClose(server->ucx_handle, server->clients[i].socket);
            server->clients[i].active = false;
        }
    }
    
    // Close listening socket
    if (server->listen_socket >= 0) {
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
    response->body = "\n";  // Initial keepalive
    response->body_length = 1;
    
    // Find which client slot this is (will be marked as SSE after response sent)
    // Return the client index as the SSE client handle
    for (int32_t i = 0; i < U_WEBSERVER_MAX_CLIENTS; i++) {
        if (server->clients[i].active && !server->clients[i].is_sse) {
            // This will be marked as SSE when response is sent
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
    
    // Send via NORA-W36 socket
    int32_t result = uCxSocketWrite(server->ucx_handle, server->clients[client].socket, 
                                    (const uint8_t*)event_buffer, len);
    
    if (result < 0) {
        // Send failed, close connection
        uCxSocketClose(server->ucx_handle, server->clients[client].socket);
        server->clients[client].active = false;
        return -3;
    }
    
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
    
    if (!server || !server->running) return;
    
    // Find free client slot
    uWebServerClient_t *client = NULL;
    for (int32_t i = 0; i < U_WEBSERVER_MAX_CLIENTS; i++) {
        if (!server->clients[i].active) {
            client = &server->clients[i];
            break;
        }
    }
    
    if (!client) {
        // No free slots, reject connection
        uCxSocketClose(puCxHandle, clientSocket);
        return;
    }
    
    // Initialize client
    client->socket = clientSocket;
    client->active = true;
    client->is_sse = false;
    client->rx_length = 0;
    client->request_complete = false;
    memset(client->rx_buffer, 0, sizeof(client->rx_buffer));
}

void uWebServerHandleDataAvailable(uWebServer_t *server, struct uCxHandle *puCxHandle,
                                   int32_t socketHandle, int32_t bytesAvailable)
{
    printf("[WebServer] uWebServerHandleDataAvailable: socket=%d, bytes=%d\n", 
           (int)socketHandle, (int)bytesAvailable);
    fflush(stdout);
    
    if (!server || !server->running) {
        printf("[WebServer] ERROR: server not running\n");
        fflush(stdout);
        return;
    }
    
    uWebServerClient_t *client = find_client_by_socket(server, socketHandle);
    if (!client) {
        printf("[WebServer] ERROR: No client found for socket %d\n", (int)socketHandle);
        fflush(stdout);
        return;
    }
    
    // Read available data (max 1000 bytes per NORA-W36 AT command limit)
    int32_t rx_space = U_WEBSERVER_MAX_REQUEST_SIZE - client->rx_length - 1;
    if (rx_space > 1000) rx_space = 1000;  // NORA-W36 max read size
    if (rx_space <= 0) {
        // Buffer full, close connection
        printf("[WebServer] Buffer full, closing\n");
        fflush(stdout);
        uCxSocketClose(puCxHandle, socketHandle);
        client->active = false;
        return;
    }
    
    printf("[WebServer] Calling uCxSocketRead(socket=%d, len=%d)...\n", (int)socketHandle, (int)rx_space);
    fflush(stdout);
    
    uint8_t *rx_ptr = (uint8_t*)(client->rx_buffer + client->rx_length);
    int32_t result = uCxSocketRead(puCxHandle, socketHandle, rx_space, rx_ptr);
    
    printf("[WebServer] uCxSocketRead returned: %d\n", (int)result);
    fflush(stdout);
    
    if (result > 0) {
        client->rx_length += result;
        client->rx_buffer[client->rx_length] = '\0';
        
        printf("[WebServer] Total received: %d bytes\n", (int)client->rx_length);
        fflush(stdout);
        
        // Check for complete HTTP request (ends with "\r\n\r\n")
        if (strstr(client->rx_buffer, "\r\n\r\n")) {
            printf("[WebServer] Complete request, calling handle_client_request()\n");
            fflush(stdout);
            client->request_complete = true;
            handle_client_request(server, client);
        }
    } else if (result < 0) {
        // Read error, close connection
        printf("[WebServer] Read error (%d), closing\n", (int)result);
        fflush(stdout);
        uCxSocketClose(puCxHandle, socketHandle);
        client->active = false;
    }
}

// Handle binary data received in DIRECT BINARY mode (USORM=2)
// Data is provided inline - no uCxSocketRead needed!
void uWebServerHandleBinaryData(uWebServer_t *server, struct uCxHandle *puCxHandle,
                                int32_t socketHandle, uint8_t *pData, size_t dataLen)
{
    (void)puCxHandle;  // Not needed - we have the data already
    
    if (!server || !server->running) return;
    
    uWebServerClient_t *client = find_client_by_socket(server, socketHandle);
    if (!client) return;
    
    // Calculate how much we can copy
    int32_t rx_space = U_WEBSERVER_MAX_REQUEST_SIZE - client->rx_length - 1;
    if (rx_space <= 0) {
        // Buffer full, close connection
        uCxSocketClose(puCxHandle, socketHandle);
        client->active = false;
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
        client->active = false;
    }
}

bool uWebServerOwnsSocket(uWebServer_t *server, int32_t socketHandle)
{
    if (!server || !server->running) return false;
    
    // Check if it's the listen socket
    if (socketHandle == server->listen_socket) {
        return true;
    }
    
    // Check if it's a client socket
    for (int32_t i = 0; i < U_WEBSERVER_MAX_CLIENTS; i++) {
        if (server->clients[i].active && server->clients[i].socket == socketHandle) {
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
