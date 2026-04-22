/*
 * u_webserver.h
 * 
 * Pure C HTTP server implementation for ucxclient - EVENT-DRIVEN ARCHITECTURE
 * Platform-independent (Windows/Linux/FreeRTOS) via u_port.h abstraction
 * 
 * Uses NORA-W36's u_cx_socket API with async callbacks (not blocking BSD sockets)
 * 
 * Architecture:
 * - All socket operations are event-driven via ucxclient callbacks
 * - No polling required - uWebServerProcess() is optional (can be used for stats)
 * - Fully asynchronous HTTP request/response handling
 * - SSE support with automatic connection keep-alive
 * 
 * Copyright (c) 2025 u-blox
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef U_WEBSERVER_H
#define U_WEBSERVER_H

#include <stdint.h>
#include <stdbool.h>
#include "u_cx.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================
 * Configuration
 * ========================================
 * 
 * Memory Footprint:
 * -----------------
 * RAM (persistent):  ~10 KB (server context + client buffers)
 *   - uWebServerClient_t × 4:  ~8.2 KB (4 × 2048 rx_buffer)
 *   - uRoute_t × 16:           ~2.1 KB (16 × 128 path)
 *   - uWebServer struct:       ~40 bytes
 * 
 * RAM (stack during request): +16 KB (response buffer)
 * 
 * Flash (code + data): ~25-35 KB
 *   - u_webserver.c compiled:  ~10-15 KB
 *   - Dashboard HTML:          ~8 KB (embedded string)
 * 
 * To reduce RAM on constrained devices:
 *   - U_WEBSERVER_MAX_CLIENTS to 2:       saves ~4 KB (minimum for SSE + API)
 *   - U_WEBSERVER_MAX_REQUEST_SIZE to 1024: saves ~2 KB
 *   - U_WEBSERVER_MAX_RESPONSE_SIZE to 8192: saves 24 KB (dashboard won't fit)
 */

/**
 * Max simultaneous client connections.
 *
 * Steady-state usage by the dashboard is 2 (1 SSE + 1 sequential API call),
 * but browsers transiently open extra connections we cannot serialize from JS:
 *   - /favicon.ico (fired by the browser, bypasses our apiCall queue)
 *   - speculative preconnect / parallel HEAD requests
 *
 * Sizing for 4 gives headroom (1 SSE + 1 API + 2 transient) and avoids mid-
 * response eviction during normal browsing. RAM cost is ~2 KB per slot.
 */
#define U_WEBSERVER_MAX_CLIENTS         4      /**< 4: 1 SSE + 1 API + 2 transient (favicon/preconnect) */
#define U_WEBSERVER_MAX_REQUEST_SIZE    2048   /**< Maximum HTTP request size */
#ifndef U_WEBSERVER_MAX_RESPONSE_SIZE
#define U_WEBSERVER_MAX_RESPONSE_SIZE   40960  /**< Maximum HTTP response size (40KB for dashboard HTML+CSS+JS) */
#endif
#define U_WEBSERVER_MAX_ROUTES          24     /**< Maximum route handlers */
#define U_WEBSERVER_SSE_EVENT_SIZE      512    /**< Maximum SSE event size */

/* ========================================
 * HTTP Types
 * ======================================== */

/**
 * HTTP request method
 */
typedef enum {
    U_HTTP_METHOD_GET,
    U_HTTP_METHOD_POST,
    U_HTTP_METHOD_PUT,
    U_HTTP_METHOD_DELETE,
    U_HTTP_METHOD_OPTIONS,
    U_HTTP_METHOD_UNKNOWN
} uHttpMethod_t;

/**
 * HTTP request structure
 */
typedef struct {
    uHttpMethod_t method;                          /**< HTTP method */
    char path[256];                                /**< Request path (e.g., "/api/status") */
    char query[256];                               /**< Query string (e.g., "id=123") */
    char headers[512];                             /**< Raw headers (newline-separated) */
    const char *body;                              /**< Request body (POST/PUT data) */
    int32_t body_length;                           /**< Body length in bytes */
} uHttpRequest_t;

/**
 * HTTP response structure
 */
typedef struct {
    int32_t status_code;                           /**< HTTP status code (200, 404, etc.) */
    char content_type[64];                         /**< Content-Type header */
    const char *body;                              /**< Response body */
    int32_t body_length;                           /**< Body length (-1 = use strlen) */
    char custom_headers[256];                      /**< Additional headers (newline-separated) */
} uHttpResponse_t;

/**
 * Route handler callback
 * 
 * @param request   HTTP request
 * @param response  HTTP response (fill in this structure)
 * @param user_data User data passed to uWebServerAddRoute()
 */
typedef void (*uRouteHandler_t)(const uHttpRequest_t *request, uHttpResponse_t *response, void *user_data);

/**
 * SSE (Server-Sent Events) client handle
 */
typedef int32_t uSseClient_t;

/**
 * Web server context (opaque)
 */
typedef struct uWebServer uWebServer_t;

/* ========================================
 * API Functions
 * ======================================== */

/**
 * Initialize web server
 * 
 * @param ucx_handle  uCX handle for NORA-W36 socket API
 * @param port        TCP port to listen on (e.g., 80)
 * @return            Web server context, or NULL on error
 */
uWebServer_t *uWebServerInit(uCxHandle_t *ucx_handle, uint16_t port);

/**
 * Add a route handler
 * 
 * @param server     Web server context
 * @param method     HTTP method (U_HTTP_METHOD_GET, etc.)
 * @param path       URL path (e.g., "/api/status")
 * @param handler    Callback function
 * @param user_data  User data passed to handler
 * @return           0 on success, negative on error
 */
int32_t uWebServerAddRoute(uWebServer_t *server, uHttpMethod_t method, const char *path, 
                           uRouteHandler_t handler, void *user_data);

/**
 * Start web server (begins listening for connections)
 * 
 * @param server  Web server context
 * @return        0 on success, negative on error
 */
int32_t uWebServerStart(uWebServer_t *server);

/**
 * Process pending HTTP requests
 * 
 * NOTE: In event-driven architecture, this function is OPTIONAL.
 * All HTTP processing happens automatically via callbacks.
 * This function can be called to get statistics (returns active client count).
 * 
 * @param server  Web server context
 * @return        Number of active connections, negative on error
 */
int32_t uWebServerProcess(uWebServer_t *server);

/**
 * Stop web server
 * 
 * @param server  Web server context
 * @return        0 on success, negative on error
 */
int32_t uWebServerStop(uWebServer_t *server);

/**
 * Free web server resources
 * 
 * @param server  Web server context
 */
void uWebServerFree(uWebServer_t *server);

/* ========================================
 * Helper Functions
 * ======================================== */

/**
 * Send JSON response
 * 
 * @param response    HTTP response structure
 * @param json_string JSON payload (null-terminated)
 */
void uWebServerSendJSON(uHttpResponse_t *response, const char *json_string);

/**
 * Send plain text response
 * 
 * @param response  HTTP response structure
 * @param text      Text payload (null-terminated)
 */
void uWebServerSendText(uHttpResponse_t *response, const char *text);

/**
 * Send HTML response
 * 
 * @param response  HTTP response structure
 * @param html      HTML payload (null-terminated)
 */
void uWebServerSendHTML(uHttpResponse_t *response, const char *html);

/**
 * Send error response
 * 
 * @param response     HTTP response structure
 * @param status_code  HTTP status code (404, 500, etc.)
 * @param message      Error message
 */
void uWebServerSendError(uHttpResponse_t *response, int32_t status_code, const char *message);

/**
 * Get query parameter value
 * 
 * @param request  HTTP request
 * @param param    Parameter name (e.g., "id")
 * @param value    Output buffer for value
 * @param max_len  Maximum length of output buffer
 * @return         0 on success, negative if not found
 */
int32_t uWebServerGetQueryParam(const uHttpRequest_t *request, const char *param, char *value, size_t max_len);

/* ========================================
 * Server-Sent Events (SSE) API
 * ======================================== */

/**
 * Register SSE endpoint (creates long-lived connection)
 * Call this from a route handler for SSE routes (e.g., GET /events)
 * 
 * @param server     Web server context
 * @param request    HTTP request
 * @param response   HTTP response (will be modified for SSE)
 * @return           SSE client handle (>= 0), or negative on error
 */
uSseClient_t uWebServerRegisterSSE(uWebServer_t *server, const uHttpRequest_t *request, uHttpResponse_t *response);

/**
 * Send SSE event to specific client
 * 
 * @param server      Web server context
 * @param client      SSE client handle
 * @param event_name  Event name (e.g., "commissioning", "state")
 * @param data        Event data (JSON string or plain text)
 * @return            0 on success, negative on error
 */
int32_t uWebServerSendSSEEvent(uWebServer_t *server, uSseClient_t client, const char *event_name, const char *data);

/**
 * Broadcast SSE event to all connected clients
 * 
 * @param server      Web server context
 * @param event_name  Event name
 * @param data        Event data
 * @return            Number of clients notified
 */
int32_t uWebServerBroadcastSSEEvent(uWebServer_t *server, const char *event_name, const char *data);

/**
 * Close SSE connection
 * 
 * @param server  Web server context
 * @param client  SSE client handle
 * @return        0 on success, negative on error
 */
int32_t uWebServerCloseSSE(uWebServer_t *server, uSseClient_t client);

/**
 * Get number of active SSE clients
 * 
 * @param server  Web server context
 * @return        Number of active SSE connections
 */
int32_t uWebServerGetSSEClientCount(uWebServer_t *server);

/* ========================================
 * External Event Handler API
 * 
 * These functions allow integration with a socket multiplexer.
 * Instead of registering its own callbacks, the web server can receive
 * events from an external dispatcher (e.g., UcxClientManager).
 * ======================================== */

/**
 * Handle incoming TCP connection event (from external dispatcher).
 * 
 * @param server          Web server context
 * @param puCxHandle      uCX handle
 * @param clientSocket    New client socket handle
 * @param remoteIp        Remote IP address (may be NULL)
 * @param listenSocket    Listening socket that received the connection
 */
void uWebServerHandleIncomingConnection(uWebServer_t *server, struct uCxHandle *puCxHandle,
                                        int32_t clientSocket, uSockIpAddress_t *remoteIp, 
                                        int32_t listenSocket);

/**
 * Handle data available event from BUFFERED mode (USORM=0/1).
 * Must call uCxSocketRead to retrieve the data.
 * 
 * @param server          Web server context
 * @param puCxHandle      uCX handle
 * @param socketHandle    Socket with data available
 * @param bytesAvailable  Number of bytes available to read (-1 if unknown)
 */
void uWebServerHandleDataAvailable(uWebServer_t *server, struct uCxHandle *puCxHandle,
                                   int32_t socketHandle, int32_t bytesAvailable);

/**
 * Handle binary data received in DIRECT BINARY mode (USORM=2).
 * Data is provided inline, no uCxSocketRead needed.
 * 
 * @param server          Web server context
 * @param puCxHandle      uCX handle
 * @param socketHandle    Socket that received data
 * @param pData           Pointer to binary data
 * @param dataLen         Length of binary data
 */
void uWebServerHandleBinaryData(uWebServer_t *server, struct uCxHandle *puCxHandle,
                                int32_t socketHandle, uint8_t *pData, size_t dataLen);

/**
 * Handle socket closed event (from external dispatcher).
 * 
 * @param server          Web server context
 * @param puCxHandle      uCX handle
 * @param socketHandle    Socket that was closed
 */
void uWebServerHandleSocketClosed(uWebServer_t *server, struct uCxHandle *puCxHandle,
                                  int32_t socketHandle);

/**
 * Check if a socket is owned by this web server.
 * 
 * @param server          Web server context
 * @param socketHandle    Socket to check
 * @return                true if owned by this server (listen socket or client socket)
 */
bool uWebServerOwnsSocket(uWebServer_t *server, int32_t socketHandle);

/**
 * Get the listening socket handle.
 * 
 * @param server  Web server context
 * @return        Listen socket handle, or -1 if not listening
 */
int32_t uWebServerGetListenSocket(uWebServer_t *server);

#ifdef __cplusplus
}
#endif

#endif /* U_WEBSERVER_H */
