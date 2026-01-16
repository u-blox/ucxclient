# ucxclient Pure C Web Server

Lightweight HTTP server implementation for ucxclient platform.

## Features

- ✅ **Pure C** - No C++ dependencies, portable across platforms
- ✅ **Platform-independent** - Works on Windows, Linux, FreeRTOS via `u_port.h`
- ✅ **NORA-W36 integration** - Uses `u_cx_socket` API directly
- ✅ **RESTful API support** - GET, POST, PUT, DELETE methods
- ✅ **Route handlers** - Clean callback-based routing
- ✅ **JSON/HTML/Text** - Helper functions for common response types
- ✅ **Small footprint** - ~500 lines of C code, minimal RAM

## Architecture

```
┌─────────────────────────────────────────┐
│   Application (Matter Device)           │
│   - Registers route handlers            │
│   - Calls uWebServerProcess() in loop   │
└─────────────────┬───────────────────────┘
                  │
┌─────────────────▼───────────────────────┐
│   u_webserver.c (Pure C HTTP Server)    │
│   - Request parsing                     │
│   - Route matching                      │
│   - Response building                   │
└─────────────────┬───────────────────────┘
                  │
┌─────────────────▼───────────────────────┐
│   u_cx_socket.h (NORA-W36 Socket API)   │
│   - TCP socket create/bind/listen       │
│   - Accept connections                  │
│   - Send/receive data                   │
└─────────────────┬───────────────────────┘
                  │
┌─────────────────▼───────────────────────┐
│   NORA-W36 Firmware (Built-in LWIP)     │
│   - TCP/IP stack                        │
│   - WiFi connectivity                   │
└─────────────────────────────────────────┘
```

## Usage Example

```c
#include "u_webserver.h"

// Define a route handler
void handle_status(const uHttpRequest_t *req, uHttpResponse_t *res, void *data)
{
    uWebServerSendJSON(res, "{\"status\":\"online\"}");
}

int main(void)
{
    // Initialize ucxclient
    uCxHandle_t ucx_handle;
    // ... (initialize ucx_handle) ...
    
    // Create web server on port 80
    uWebServer_t *server = uWebServerInit(&ucx_handle, 80);
    
    // Register routes
    uWebServerAddRoute(server, U_HTTP_METHOD_GET, "/api/status", handle_status, NULL);
    
    // Start server
    uWebServerStart(server);
    
    // Main loop
    while (1) {
        uWebServerProcess(server);  // Process HTTP requests
        uPortTaskBlock(100);        // Sleep 100ms
    }
}
```

## API Reference

### Initialization

```c
uWebServer_t *uWebServerInit(uCxHandle_t *ucx_handle, uint16_t port);
```
Creates web server context.

### Route Registration

```c
int32_t uWebServerAddRoute(uWebServer_t *server, uHttpMethod_t method, 
                           const char *path, uRouteHandler_t handler, void *user_data);
```
Registers a route handler for specific HTTP method and path.

**Example:**
```c
uWebServerAddRoute(server, U_HTTP_METHOD_GET, "/api/device", get_device_info, NULL);
uWebServerAddRoute(server, U_HTTP_METHOD_POST, "/api/control", control_device, NULL);
```

### Server Control

```c
int32_t uWebServerStart(uWebServer_t *server);   // Start listening
int32_t uWebServerProcess(uWebServer_t *server); // Process requests (call in loop)
int32_t uWebServerStop(uWebServer_t *server);    // Stop server
void uWebServerFree(uWebServer_t *server);       // Free resources
```

### Response Helpers

```c
void uWebServerSendJSON(uHttpResponse_t *response, const char *json);
void uWebServerSendText(uHttpResponse_t *response, const char *text);
void uWebServerSendHTML(uHttpResponse_t *response, const char *html);
void uWebServerSendError(uHttpResponse_t *response, int32_t status, const char *msg);
```

### Query Parameters

```c
int32_t uWebServerGetQueryParam(const uHttpRequest_t *req, const char *param, 
                                 char *value, size_t max_len);
```

**Example:**
```c
char action[64];
if (uWebServerGetQueryParam(request, "action", action, sizeof(action)) == 0) {
    if (strcmp(action, "on") == 0) {
        // Turn device on
    }
}
```

## Route Handler Pattern

```c
void my_handler(const uHttpRequest_t *req, uHttpResponse_t *res, void *user_data)
{
    // Access request data
    printf("Method: %d\n", req->method);
    printf("Path: %s\n", req->path);
    printf("Query: %s\n", req->query);
    printf("Body: %s\n", req->body);
    
    // Build response
    res->status_code = 200;
    strcpy(res->content_type, "application/json");
    res->body = "{\"result\":\"success\"}";
    res->body_length = -1;  // Auto-calculate from strlen
    
    // Or use helpers:
    // uWebServerSendJSON(res, "{\"result\":\"success\"}");
}
```

## Integration with Matter Device

```c
// In app/main.cpp (Matter application)
extern "C" {
    #include "ucxclient/webserver/u_webserver.h"
}

// Global web server instance
static uWebServer_t *g_webserver = nullptr;

// Initialize in main()
void InitWebServer()
{
    auto& ucx = DeviceLayer::UcxClientManager::GetInstance();
    uCxHandle_t *ucx_handle = ucx.GetUcxHandle();
    
    g_webserver = uWebServerInit(ucx_handle, 80);
    
    // Register Matter diagnostic endpoints
    uWebServerAddRoute(g_webserver, U_HTTP_METHOD_GET, "/api/matter/status", 
                       handle_matter_status, nullptr);
    uWebServerAddRoute(g_webserver, U_HTTP_METHOD_GET, "/api/network/info", 
                       handle_network_info, nullptr);
    
    uWebServerStart(g_webserver);
}

// Call in main loop
void ProcessWebServer()
{
    if (g_webserver) {
        uWebServerProcess(g_webserver);
    }
}
```

## Memory Configuration

```c
// In u_webserver.h - adjust as needed:
#define U_WEBSERVER_MAX_CLIENTS         4      // Concurrent connections
#define U_WEBSERVER_MAX_REQUEST_SIZE    2048   // Max HTTP request
#define U_WEBSERVER_MAX_RESPONSE_SIZE   4096   // Max HTTP response
#define U_WEBSERVER_MAX_ROUTES          16     // Max route handlers
```

## Limitations

- **Blocking I/O**: Server processes one request at a time per client
- **No HTTPS**: TLS not yet implemented (can be added via `uCxSocketSetTLS`)
- **No WebSockets**: HTTP/1.1 only
- **No chunked encoding**: Responses must fit in buffer

## Future Enhancements

- [ ] TLS/HTTPS support via NORA-W36's built-in TLS
- [ ] Chunked transfer encoding for large responses
- [ ] WebSocket support
- [ ] Static file serving (from filesystem or embedded)
- [ ] CORS headers for cross-origin requests
- [ ] Authentication middleware (Basic Auth, Bearer tokens)

## License

Apache-2.0 (same as ucxclient and Matter SDK)

## See Also

- `webserver_example.c` - Complete working example
- `u_cx_socket.h` - NORA-W36 socket API reference
- `u_port.h` - Platform abstraction layer
