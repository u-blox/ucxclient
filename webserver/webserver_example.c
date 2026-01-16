/*
 * webserver_example.c
 * 
 * Example usage of pure C web server for ucxclient
 * 
 * Copyright (c) 2025 u-blox
 * SPDX-License-Identifier: Apache-2.0
 */

#include "u_webserver.h"
#include "u_cx.h"
#include "u_port.h"
#include <stdio.h>
#include <string.h>

/* ========================================
 * Route Handlers
 * ======================================== */

/**
 * GET /api/status - Device status endpoint
 */
static void handle_status(const uHttpRequest_t *request, uHttpResponse_t *response, void *user_data)
{
    (void)request;  // Unused
    (void)user_data;
    
    // Build JSON response
    static char json_buf[512];
    snprintf(json_buf, sizeof(json_buf),
        "{\n"
        "  \"device\": \"NORA-W36\",\n"
        "  \"status\": \"online\",\n"
        "  \"uptime_ms\": %llu\n"
        "}",
        (unsigned long long)uPortGetTickTimeMs()
    );
    
    uWebServerSendJSON(response, json_buf);
}

/**
 * GET /api/wifi - WiFi status
 */
static void handle_wifi(const uHttpRequest_t *request, uHttpResponse_t *response, void *user_data)
{
    (void)request;
    (void)user_data;
    
    static char json_buf[256];
    snprintf(json_buf, sizeof(json_buf),
        "{\n"
        "  \"connected\": true,\n"
        "  \"ssid\": \"MyNetwork\",\n"
        "  \"rssi\": -45\n"
        "}"
    );
    
    uWebServerSendJSON(response, json_buf);
}

/**
 * POST /api/control - Device control endpoint
 */
static void handle_control(const uHttpRequest_t *request, uHttpResponse_t *response, void *user_data)
{
    (void)user_data;
    
    // Parse action from query string
    char action[64];
    if (uWebServerGetQueryParam(request, "action", action, sizeof(action)) == 0) {
        if (strcmp(action, "on") == 0) {
            uWebServerSendText(response, "Device turned ON");
        } else if (strcmp(action, "off") == 0) {
            uWebServerSendText(response, "Device turned OFF");
        } else {
            uWebServerSendError(response, 400, "Invalid action");
        }
    } else {
        uWebServerSendError(response, 400, "Missing 'action' parameter");
    }
}

/**
 * GET / - HTML homepage
 */
static void handle_homepage(const uHttpRequest_t *request, uHttpResponse_t *response, void *user_data)
{
    (void)request;
    (void)user_data;
    
    static const char html[] =
        "<!DOCTYPE html>\n"
        "<html>\n"
        "<head><title>NORA-W36 Matter Device</title></head>\n"
        "<body>\n"
        "  <h1>NORA-W36 Web Server</h1>\n"
        "  <p>Pure C HTTP server running on ucxclient</p>\n"
        "  <h2>API Endpoints:</h2>\n"
        "  <ul>\n"
        "    <li><a href=\"/api/status\">/api/status</a> - Device status (JSON)</li>\n"
        "    <li><a href=\"/api/wifi\">/api/wifi</a> - WiFi status (JSON)</li>\n"
        "    <li>/api/control?action=on - Turn device ON</li>\n"
        "    <li>/api/control?action=off - Turn device OFF</li>\n"
        "  </ul>\n"
        "</body>\n"
        "</html>";
    
    uWebServerSendHTML(response, html);
}

/* ========================================
 * Main Example
 * ======================================== */

int main(void)
{
    // Initialize u_port layer (Windows/Linux/FreeRTOS)
    uPortInit();
    
    // Initialize ucxclient AT client
    uCxAtClient_t at_client;
    uCxAtClientConfig_t config;
    memset(&config, 0, sizeof(config));
    // ... configure COM port, buffers, etc. ...
    
    uCxAtClientInit(&config, &at_client);
    uCxAtClientOpen(&at_client, 115200, true);
    
    // Initialize uCX handle
    uCxHandle_t ucx_handle;
    uCxInit(&at_client, &ucx_handle);
    
    // Connect to WiFi
    printf("Connecting to WiFi...\n");
    // ... WiFi connection code ...
    
    // Initialize web server on port 80
    uWebServer_t *server = uWebServerInit(&ucx_handle, 80);
    if (!server) {
        printf("ERROR: Failed to initialize web server\n");
        return -1;
    }
    
    // Register route handlers
    uWebServerAddRoute(server, U_HTTP_METHOD_GET, "/", handle_homepage, NULL);
    uWebServerAddRoute(server, U_HTTP_METHOD_GET, "/api/status", handle_status, NULL);
    uWebServerAddRoute(server, U_HTTP_METHOD_GET, "/api/wifi", handle_wifi, NULL);
    uWebServerAddRoute(server, U_HTTP_METHOD_POST, "/api/control", handle_control, NULL);
    
    // Start web server
    if (uWebServerStart(server) < 0) {
        printf("ERROR: Failed to start web server\n");
        uWebServerFree(server);
        return -1;
    }
    
    printf("Web server running on port 80\n");
    printf("Access http://<device-ip>/ for homepage\n");
    
    // Main loop
    while (1) {
        // Process HTTP requests
        int32_t processed = uWebServerProcess(server);
        if (processed > 0) {
            printf("Processed %d request(s)\n", processed);
        }
        
        // Sleep to avoid busy-wait (platform-specific)
        uPortTaskBlock(100);  // 100ms
    }
    
    // Cleanup (unreachable in this example)
    uWebServerStop(server);
    uWebServerFree(server);
    
    return 0;
}
