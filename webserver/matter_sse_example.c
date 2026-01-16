/*
 * matter_sse_example.c
 * 
 * Example: Server-Sent Events integration with Matter commissioning
 * Real-time push of commissioning progress, device state, network status
 * 
 * Copyright (c) 2025 u-blox
 * SPDX-License-Identifier: Apache-2.0
 */

#include "u_webserver.h"
#include "u_cx.h"
#include <stdio.h>
#include <string.h>

/* ========================================
 * Global State
 * ======================================== */

static uWebServer_t *g_server = NULL;

/* ========================================
 * SSE Route Handler
 * ======================================== */

/**
 * GET /events - SSE endpoint for real-time updates
 */
static void handle_sse_events(const uHttpRequest_t *request, uHttpResponse_t *response, void *user_data)
{
    (void)user_data;
    
    // Register this connection as SSE
    uSseClient_t client = uWebServerRegisterSSE(g_server, request, response);
    if (client < 0) {
        uWebServerSendError(response, 503, "SSE capacity full");
        return;
    }
    
    // Send initial connected event
    uWebServerSendSSEEvent(g_server, client, "connected", "{\"message\":\"Event stream connected\"}");
    
    printf("[SSE] New client connected (total: %d)\n", uWebServerGetSSEClientCount(g_server));
}

/* ========================================
 * Matter Event Notifications
 * ======================================== */

/**
 * Notify all SSE clients about commissioning progress
 * Call this from Matter commissioning callbacks
 */
void NotifyCommissioningProgress(int step, int total, const char *stage_name)
{
    if (!g_server) return;
    
    char json_buf[256];
    snprintf(json_buf, sizeof(json_buf),
        "{\n"
        "  \"step\": %d,\n"
        "  \"total\": %d,\n"
        "  \"stage\": \"%s\",\n"
        "  \"progress\": %d\n"
        "}",
        step, total, stage_name, (step * 100) / total
    );
    
    int32_t sent = uWebServerBroadcastSSEEvent(g_server, "commissioning", json_buf);
    printf("[SSE] Broadcasted commissioning event to %d clients\n", sent);
}

/**
 * Notify about device state change
 */
void NotifyDeviceState(const char *state, bool is_on)
{
    if (!g_server) return;
    
    char json_buf[128];
    snprintf(json_buf, sizeof(json_buf),
        "{\n"
        "  \"state\": \"%s\",\n"
        "  \"on\": %s\n"
        "}",
        state, is_on ? "true" : "false"
    );
    
    uWebServerBroadcastSSEEvent(g_server, "device_state", json_buf);
}

/**
 * Notify about network status change
 */
void NotifyNetworkStatus(const char *ssid, int32_t rssi, bool connected)
{
    if (!g_server) return;
    
    char json_buf[256];
    snprintf(json_buf, sizeof(json_buf),
        "{\n"
        "  \"ssid\": \"%s\",\n"
        "  \"rssi\": %d,\n"
        "  \"connected\": %s\n"
        "}",
        ssid, rssi, connected ? "true" : "false"
    );
    
    uWebServerBroadcastSSEEvent(g_server, "network", json_buf);
}

/**
 * Notify about errors
 */
void NotifyError(const char *error_message)
{
    if (!g_server) return;
    
    char json_buf[256];
    snprintf(json_buf, sizeof(json_buf),
        "{\n"
        "  \"error\": \"%s\",\n"
        "  \"timestamp\": %llu\n"
        "}",
        error_message, (unsigned long long)uPortGetTickTimeMs()
    );
    
    uWebServerBroadcastSSEEvent(g_server, "error", json_buf);
}

/* ========================================
 * Regular API Endpoints (REST)
 * ======================================== */

/**
 * GET /api/commissioning/status - Current commissioning state
 */
static void handle_commissioning_status(const uHttpRequest_t *request, uHttpResponse_t *response, void *user_data)
{
    (void)request;
    (void)user_data;
    
    // Mock data - replace with real Matter commissioning state
    static char json_buf[256];
    snprintf(json_buf, sizeof(json_buf),
        "{\n"
        "  \"commissioned\": false,\n"
        "  \"pairing_code\": \"34970112332\",\n"
        "  \"discriminator\": 3840,\n"
        "  \"sse_clients\": %d\n"
        "}",
        uWebServerGetSSEClientCount(g_server)
    );
    
    uWebServerSendJSON(response, json_buf);
}

/**
 * POST /api/commissioning/reset - Trigger factory reset
 */
static void handle_commissioning_reset(const uHttpRequest_t *request, uHttpResponse_t *response, void *user_data)
{
    (void)request;
    (void)user_data;
    
    // Notify all clients before reset
    NotifyError("Factory reset initiated");
    
    // Trigger reset (replace with real Matter API)
    // chip::Server::GetInstance().ScheduleFactoryReset();
    
    uWebServerSendJSON(response, "{\"status\":\"Factory reset scheduled\"}");
}

/* ========================================
 * HTML Dashboard
 * ======================================== */

/**
 * GET / - HTML dashboard with live SSE updates
 */
static void handle_dashboard(const uHttpRequest_t *request, uHttpResponse_t *response, void *user_data)
{
    (void)request;
    (void)user_data;
    
    static const char html[] =
        "<!DOCTYPE html>\n"
        "<html>\n"
        "<head>\n"
        "  <title>Matter Device Dashboard</title>\n"
        "  <style>\n"
        "    body { font-family: Arial, sans-serif; margin: 20px; }\n"
        "    .status { padding: 10px; margin: 10px 0; border-radius: 5px; }\n"
        "    .online { background: #d4edda; }\n"
        "    .offline { background: #f8d7da; }\n"
        "    #log { border: 1px solid #ccc; padding: 10px; height: 300px; overflow-y: auto; }\n"
        "    .event { margin: 5px 0; padding: 5px; background: #f0f0f0; }\n"
        "  </style>\n"
        "</head>\n"
        "<body>\n"
        "  <h1>Matter Device Dashboard</h1>\n"
        "  <div id=\"status\" class=\"status offline\">Disconnected from event stream</div>\n"
        "  <h2>Real-Time Events</h2>\n"
        "  <div id=\"log\"></div>\n"
        "  <script>\n"
        "    const log = document.getElementById('log');\n"
        "    const status = document.getElementById('status');\n"
        "    \n"
        "    // Connect to SSE endpoint\n"
        "    const eventSource = new EventSource('/events');\n"
        "    \n"
        "    eventSource.addEventListener('connected', (e) => {\n"
        "      status.textContent = 'Connected to event stream';\n"
        "      status.className = 'status online';\n"
        "      addLog('connected', e.data);\n"
        "    });\n"
        "    \n"
        "    eventSource.addEventListener('commissioning', (e) => {\n"
        "      addLog('commissioning', e.data);\n"
        "    });\n"
        "    \n"
        "    eventSource.addEventListener('device_state', (e) => {\n"
        "      addLog('device_state', e.data);\n"
        "    });\n"
        "    \n"
        "    eventSource.addEventListener('network', (e) => {\n"
        "      addLog('network', e.data);\n"
        "    });\n"
        "    \n"
        "    eventSource.addEventListener('error', (e) => {\n"
        "      addLog('error', e.data);\n"
        "    });\n"
        "    \n"
        "    eventSource.onerror = () => {\n"
        "      status.textContent = 'Disconnected from event stream';\n"
        "      status.className = 'status offline';\n"
        "    };\n"
        "    \n"
        "    function addLog(type, data) {\n"
        "      const entry = document.createElement('div');\n"
        "      entry.className = 'event';\n"
        "      entry.innerHTML = `<strong>${type}</strong>: ${data}`;\n"
        "      log.insertBefore(entry, log.firstChild);\n"
        "      // Keep only last 50 events\n"
        "      while (log.children.length > 50) log.removeChild(log.lastChild);\n"
        "    }\n"
        "  </script>\n"
        "</body>\n"
        "</html>";
    
    uWebServerSendHTML(response, html);
}

/* ========================================
 * Main Example
 * ======================================== */

int main(void)
{
    // Initialize ucxclient
    uPortInit();
    uCxHandle_t ucx_handle;
    // ... (initialize ucx_handle) ...
    
    // Create web server
    g_server = uWebServerInit(&ucx_handle, 80);
    
    // Register HTML dashboard
    uWebServerAddRoute(g_server, U_HTTP_METHOD_GET, "/", handle_dashboard, NULL);
    
    // Register SSE endpoint
    uWebServerAddRoute(g_server, U_HTTP_METHOD_GET, "/events", handle_sse_events, NULL);
    
    // Register REST API endpoints
    uWebServerAddRoute(g_server, U_HTTP_METHOD_GET, "/api/commissioning/status", 
                       handle_commissioning_status, NULL);
    uWebServerAddRoute(g_server, U_HTTP_METHOD_POST, "/api/commissioning/reset", 
                       handle_commissioning_reset, NULL);
    
    // Start server
    uWebServerStart(g_server);
    printf("Web server started on port 80\n");
    printf("Open http://<device-ip>/ for live dashboard\n");
    
    // Simulate Matter events (replace with real Matter callbacks)
    int step = 0;
    while (1) {
        // Process HTTP requests
        uWebServerProcess(g_server);
        
        // Simulate commissioning progress (every 5 seconds)
        if (step < 12) {
            const char *stages[] = {
                "Secure Pairing", "Reading Device Info", "Arming Failsafe",
                "Device Attestation", "CSR", "NOC Chain", "Sending Certs",
                "WiFi Setup", "WiFi Enable", "Operational Session",
                "Complete", "Cleanup"
            };
            NotifyCommissioningProgress(step + 1, 12, stages[step]);
            step++;
        }
        
        uPortTaskBlock(5000);  // 5 seconds
    }
    
    return 0;
}
