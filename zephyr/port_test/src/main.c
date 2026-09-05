/*
 * Copyright 2025 u-blox
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <inttypes.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/serial/uart_emul.h>
#include <zephyr/ztest.h>

#include "u_port.h"
#include "u_cx_at_client.h"
#include "u_port_uart.h"

/* ----------------------------------------------------------------
 * COMPILE-TIME MACROS
 * -------------------------------------------------------------- */

#define EMUL_UART_NODE	       	DT_NODELABEL(euart0)
#define EMUL_UART_RX_FIFO_SIZE 	DT_PROP(EMUL_UART_NODE, rx_fifo_size)
#define EMUL_UART_TX_FIFO_SIZE 	DT_PROP(EMUL_UART_NODE, tx_fifo_size)

#define U_RINGBUFFER_SIZE       128

#define TEST_DATA_SIZE          (U_RINGBUFFER_SIZE * 2)
#define MODEM_THREAD_STACK_SIZE 4096
#define MODEM_THREAD_PRIORITY   5

#define TIMESTAMP_CREATE()      int64_t __timestamp = k_uptime_get();

#define TIMESTAMP_CHECK_TIME(expectMs)                                          \
    do {                                                                        \
        int64_t deltaMs =  k_uptime_delta(&__timestamp);                        \
        zassert_within(deltaMs, expectMs, 30, "took: %" PRId64 " ms", deltaMs); \
    } while (0);

/* ----------------------------------------------------------------
 * TYPES
 * -------------------------------------------------------------- */

struct u_connect_client_port_fixture {
    const struct device *pDev;
    uint8_t testData[TEST_DATA_SIZE];
    uCxAtClient_t client;
    uCxAtClientConfig_t config;
    uint8_t rxBuffer[TEST_DATA_SIZE];
    uint8_t urcBuffer[TEST_DATA_SIZE];
};

struct modem_response {
    const struct device *pDev;
    const uint8_t *pChunks[5];
    size_t chunkLengths[5];
    size_t chunkCount;
    size_t chunksSent;
    int32_t error;
};

struct urc_capture {
    uCxAtClient_t *pClient;
    char line[32];
    size_t lineLength;
    uint8_t binaryData[16];
    size_t binaryDataLength;
};

/* ----------------------------------------------------------------
 * VARIABLES
 * -------------------------------------------------------------- */

extern bool gDisableRxWorker;

K_SEM_DEFINE(gModemRequest, 0, 1);
K_SEM_DEFINE(gModemDone, 0, 1);
K_SEM_DEFINE(gUrcReceived, 0, 1);
static struct modem_response *gpModemResponse;
static struct urc_capture gUrcCapture;

static void urcCallback(uCxAtClient_t *pClient, void *pTag, char *pLine,
                        size_t lineLength, uint8_t *pBinaryData,
                        size_t binaryDataLength)
{
    ARG_UNUSED(pTag);

    gUrcCapture.pClient = pClient;
    gUrcCapture.lineLength = MIN(lineLength, sizeof(gUrcCapture.line) - 1);
    memcpy(gUrcCapture.line, pLine, gUrcCapture.lineLength);
    gUrcCapture.line[gUrcCapture.lineLength] = '\0';
    gUrcCapture.binaryDataLength = MIN(binaryDataLength,
                                       sizeof(gUrcCapture.binaryData));
    memcpy(gUrcCapture.binaryData, pBinaryData,
           gUrcCapture.binaryDataLength);
    k_sem_give(&gUrcReceived);
}

static void modemResponseThread(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    while (true) {
        k_sem_take(&gModemRequest, K_FOREVER);
        struct modem_response *pResponse = gpModemResponse;

        for (size_t i = 0; i < pResponse->chunkCount; i++) {
            k_sleep(K_MSEC(5));
            int32_t sent = uart_emul_put_rx_data(pResponse->pDev,
                                                 pResponse->pChunks[i],
                                                 pResponse->chunkLengths[i]);
            if (sent != pResponse->chunkLengths[i]) {
                pResponse->error = sent;
                break;
            }
            pResponse->chunksSent++;
        }
        k_sem_give(&gModemDone);
    }
}

K_THREAD_DEFINE(gModemThread, MODEM_THREAD_STACK_SIZE, modemResponseThread,
                NULL, NULL, NULL, MODEM_THREAD_PRIORITY, 0, 0);

static void startModemResponse(struct modem_response *pResponse)
{
    gpModemResponse = pResponse;
    k_sem_give(&gModemRequest);
}

static void waitForModemResponse(void)
{
    zassert_equal(k_sem_take(&gModemDone, K_SECONDS(1)), 0);
}

/* ----------------------------------------------------------------
 * TEST SETUP
 * -------------------------------------------------------------- */

static void *u_connect_client_port_setup(void)
{
    static struct u_connect_client_port_fixture fixture = {
        .pDev = DEVICE_DT_GET(EMUL_UART_NODE)
    };

    for (size_t i = 0; i < TEST_DATA_SIZE; i++) {
        fixture.testData[i] = i;
    }

    zassert_not_null(fixture.pDev);

    // Initialize config with device name
    fixture.config.pRxBuffer = fixture.rxBuffer;
    fixture.config.rxBufferLen = sizeof(fixture.rxBuffer);
    fixture.config.pUrcBuffer = fixture.urcBuffer;
    fixture.config.urcBufferLen = sizeof(fixture.urcBuffer);
    fixture.config.pUartDevName = fixture.pDev->name;
    fixture.config.timeoutMs = 10;

    // Initialize AT client
    uCxAtClientInit(&fixture.config, &fixture.client);

    return &fixture;
}

static void u_connect_client_port_before(void *f)
{
    struct u_connect_client_port_fixture *fixture = f;

    uart_irq_tx_disable(fixture->pDev);
    uart_irq_rx_disable(fixture->pDev);

    uart_emul_flush_rx_data(fixture->pDev);
    uart_emul_flush_tx_data(fixture->pDev);

    uart_err_check(fixture->pDev);

    memset(&fixture->rxBuffer, 0, sizeof(fixture->rxBuffer));
    memset(&fixture->urcBuffer, 0, sizeof(fixture->urcBuffer));
    memset(&gUrcCapture, 0, sizeof(gUrcCapture));
    k_sem_reset(&gUrcReceived);

    gDisableRxWorker = true;

    // Open the AT client with baudrate and flow control
    int32_t result = uCxAtClientOpen(&fixture->client, 115200, true);
    zassert_equal(result, 0, "uCxAtClientOpen failed: %d", result);
}

static void u_connect_client_port_after(void *f)
{
    struct u_connect_client_port_fixture *fixture = f;

    uCxAtClientClose(&fixture->client);
}

/* ----------------------------------------------------------------
 * TESTS
 * -------------------------------------------------------------- */

ZTEST_F(u_connect_client_port, test_rx_no_data)
{
    TIMESTAMP_CREATE();
    int32_t rc = uPortUartRead(fixture->client.uartHandle, NULL, 1, 0);
    TIMESTAMP_CHECK_TIME(0);
    zassert_equal(rc, 0, "read() returned: %d", rc);
}

ZTEST_F(u_connect_client_port, test_rx_no_data_timeout)
{
    TIMESTAMP_CREATE();
    int32_t rc = uPortUartRead(fixture->client.uartHandle, NULL, 1, 100);
    TIMESTAMP_CHECK_TIME(100);
    zassert_equal(rc, 0, "read() returned: %d", rc);
}

ZTEST_F(u_connect_client_port, test_rx_some_data_timeout)
{
    uart_emul_put_rx_data(fixture->pDev, &fixture->testData[0], 3);

    TIMESTAMP_CREATE();
    int32_t rc = uPortUartRead(fixture->client.uartHandle,
                               &fixture->rxBuffer, 4, 100);
    TIMESTAMP_CHECK_TIME(100);
    zassert_equal(rc, 3, "read() returned: %d", rc);
    zassert_mem_equal__(&fixture->rxBuffer, &fixture->testData, 3);
}

ZTEST_F(u_connect_client_port, test_rx_read_some_data_timeout)
{
    uart_emul_put_rx_data(fixture->pDev, &fixture->testData[0], 3);

    TIMESTAMP_CREATE();
    int32_t rc = uPortUartRead(fixture->client.uartHandle,
                               &fixture->rxBuffer, 2, 100);
    TIMESTAMP_CHECK_TIME(0);
    zassert_equal(rc, 2, "read() returned: %d", rc);
    zassert_mem_equal__(&fixture->rxBuffer, &fixture->testData, 2);
}

ZTEST_F(u_connect_client_port, test_rx_read_some_data_no_timeout)
{
    uart_emul_put_rx_data(fixture->pDev, &fixture->testData[0], 3);
    // Need a little sleep here so that the ISR receives all the data before next step
    k_sleep(K_MSEC(10));

    TIMESTAMP_CREATE();
    int32_t rc = uPortUartRead(fixture->client.uartHandle,
                               &fixture->rxBuffer, 2, 0);
    TIMESTAMP_CHECK_TIME(0);
    zassert_equal(rc, 2, "read() returned: %d", rc);
    zassert_mem_equal__(&fixture->rxBuffer, &fixture->testData, 2);
}

ZTEST_F(u_connect_client_port, test_rx_all_data)
{
    uart_emul_put_rx_data(fixture->pDev, &fixture->testData[0], 3);

    TIMESTAMP_CREATE();
    int32_t rc = uPortUartRead(fixture->client.uartHandle,
                               &fixture->rxBuffer, 3, 100);
    TIMESTAMP_CHECK_TIME(0);
    zassert_equal(rc, 3, "read() returned: %d", rc);
    zassert_mem_equal__(&fixture->rxBuffer, &fixture->testData, 3);
}

ZTEST_F(u_connect_client_port, test_rx_ringbuf_full)
{
    // Receive a little more data than can be fitted into the ring buffer
    uart_emul_put_rx_data(fixture->pDev, &fixture->testData[0], U_RINGBUFFER_SIZE + 8);

    // Need a little sleep here so that the ISR receives all the data before next step
    k_sleep(K_MSEC(10));

    TIMESTAMP_CREATE();
    int32_t rc = uPortUartRead(fixture->client.uartHandle, &fixture->rxBuffer, 8, 100);
    TIMESTAMP_CHECK_TIME(0);
    zassert_equal(rc, 8, "read() returned: %d", rc);
    rc = uPortUartRead(fixture->client.uartHandle,
                       &fixture->rxBuffer[8], U_RINGBUFFER_SIZE, 100);
    TIMESTAMP_CHECK_TIME(0);
    zassert_equal(rc, U_RINGBUFFER_SIZE, "read() returned: %d", rc);
    zassert_mem_equal__(&fixture->rxBuffer, &fixture->testData, U_RINGBUFFER_SIZE + 8);

    // Everything should be read now - make sure read returns 0
    rc = uPortUartRead(fixture->client.uartHandle,
                       &fixture->rxBuffer[0], U_RINGBUFFER_SIZE, 100);
    TIMESTAMP_CHECK_TIME(100);
    zassert_equal(rc, 0, "read() returned: %d", rc);
}

ZTEST_F(u_connect_client_port, test_tx_fifo_full)
{
    int32_t rc = uPortUartWrite(fixture->client.uartHandle,
                                &fixture->testData[0], EMUL_UART_TX_FIFO_SIZE + 8);
    zassert_equal(rc, EMUL_UART_TX_FIFO_SIZE, "write() returned: %d", rc);

    rc = uart_emul_get_tx_data(fixture->pDev, &fixture->rxBuffer[0], EMUL_UART_TX_FIFO_SIZE + 8);
    zassert_equal(rc, EMUL_UART_TX_FIFO_SIZE, "uart_emul_get_tx_data() returned: %d", rc);
    zassert_mem_equal__(&fixture->rxBuffer, &fixture->testData, U_RINGBUFFER_SIZE);
}

ZTEST_F(u_connect_client_port, test_invalid_arguments)
{
    zassert_is_null(uPortUartOpen(NULL, 115200, false));
    zassert_is_null(uPortUartOpen(fixture->pDev->name, 115200, false));

    uPortUartClose(NULL);

    zassert_equal(uPortUartWrite(NULL, fixture->testData, 1), -1);
    zassert_equal(uPortUartWrite(fixture->client.uartHandle, NULL, 1), -1);
    zassert_equal(uPortUartWrite(fixture->client.uartHandle, fixture->testData, 0), -1);

    zassert_equal(uPortUartRead(NULL, fixture->rxBuffer, 1, 0), -1);
    zassert_equal(uPortUartRead(fixture->client.uartHandle, fixture->rxBuffer, 0, 0), -1);

    uart_emul_put_rx_data(fixture->pDev, fixture->testData, 1);
    k_sleep(K_MSEC(10));

    zassert_equal(uPortUartRead(fixture->client.uartHandle, NULL, 1, 100), 0);
    zassert_equal(uPortUartRead(fixture->client.uartHandle, NULL, 1, 0), 0);
    zassert_equal(uPortUartRead(fixture->client.uartHandle, fixture->rxBuffer, 1, 0), 1);
    zassert_mem_equal__(fixture->rxBuffer, fixture->testData, 1);
}

ZTEST_F(u_connect_client_port, test_reopen_without_flow_control)
{
    uPortUartClose(fixture->client.uartHandle);
    fixture->client.uartHandle = NULL;

    zassert_is_null(uPortUartOpen("missing-uart-device", 115200, false));

    fixture->client.uartHandle = uPortUartOpen(fixture->pDev->name, 9600, false);
    zassert_not_null(fixture->client.uartHandle);

    struct uart_config config;
    zassert_equal(uart_config_get(fixture->pDev, &config), 0);
    zassert_equal(config.baudrate, 9600);
    zassert_equal(config.flow_ctrl, UART_CFG_FLOW_CTRL_NONE);
}

ZTEST_F(u_connect_client_port, test_os_port_and_rx_worker)
{
    uPortInit();

    int32_t startTimeMs = U_CX_PORT_GET_TIME_MS();
    k_sleep(K_MSEC(1));
    zassert_true(U_CX_PORT_GET_TIME_MS() >= startTimeMs);

    gDisableRxWorker = false;
    uart_emul_put_rx_data(fixture->pDev, &fixture->testData[1], 1);
    k_sleep(K_MSEC(10));
    gDisableRxWorker = true;

    uPortBgRxTaskDestroy(&fixture->client);
    uPortBgRxTaskCreate(&fixture->client);
    uPortDeinit();
}

ZTEST_F(u_connect_client_port, test_fragmented_command_response)
{
    static const uint8_t responseStart[] = "\r\nO";
    static const uint8_t responseEnd[] = "K\r\n";
    struct modem_response response = {
        .pDev = fixture->pDev,
        .pChunks = {responseStart, responseEnd},
        .chunkLengths = {sizeof(responseStart) - 1, sizeof(responseEnd) - 1},
        .chunkCount = 2
    };

    gDisableRxWorker = false;
    startModemResponse(&response);
    int32_t result = uCxAtClientExecSimpleCmd(&fixture->client, "AT");
    waitForModemResponse();
    gDisableRxWorker = true;
    zassert_equal(response.error, 0);
    zassert_equal(response.chunksSent, response.chunkCount);
    zassert_equal(result, 0);

    uint8_t txData[3];
    zassert_equal(uart_emul_get_tx_data(fixture->pDev, txData, sizeof(txData)),
                  sizeof(txData));
    zassert_mem_equal__(txData, "AT\r", sizeof(txData));
}

ZTEST_F(u_connect_client_port, test_fragmented_binary_response)
{
    static const uint8_t responseLine[] = {'+', 'F', 'O', 'O', ':', 0x01};
    static const uint8_t lengthHigh[] = {0x00};
    static const uint8_t lengthLowAndData[] = {0x04, 0x00, 0x11};
    static const uint8_t remainingData[] = {0x22, 0xff};
    static const uint8_t responseStatus[] = "\r\nOK\r\n";
    static const uint8_t expectedData[] = {0x00, 0x11, 0x22, 0xff};
    struct modem_response response = {
        .pDev = fixture->pDev,
        .pChunks = {responseLine, lengthHigh, lengthLowAndData,
                    remainingData, responseStatus},
        .chunkLengths = {sizeof(responseLine), sizeof(lengthHigh),
                         sizeof(lengthLowAndData), sizeof(remainingData),
                         sizeof(responseStatus) - 1},
        .chunkCount = 5
    };
    uint8_t binaryData[sizeof(expectedData)] = {0};
    uint16_t binaryLength = sizeof(binaryData);

    startModemResponse(&response);
    uCxAtClientCmdBeginF(&fixture->client, "AT+FOO", "",
                         U_CX_AT_UTIL_PARAM_LAST);
    char *pParams = uCxAtClientCmdGetRspParamLine(&fixture->client, "+FOO:",
                                                  binaryData, &binaryLength);
    waitForModemResponse();
    zassert_equal(response.error, 0);
    zassert_equal(response.chunksSent, response.chunkCount);
    zassert_not_null(pParams);
    zassert_equal(pParams[0], '\0');
    zassert_equal(binaryLength, sizeof(expectedData));
    zassert_mem_equal__(binaryData, expectedData, sizeof(expectedData));
    zassert_equal(uCxAtClientCmdEnd(&fixture->client), 0);

    uint8_t txData[7];
    zassert_equal(uart_emul_get_tx_data(fixture->pDev, txData, sizeof(txData)),
                  sizeof(txData));
    zassert_mem_equal__(txData, "AT+FOO\r", sizeof(txData));
}

ZTEST_F(u_connect_client_port, test_fragmented_binary_urc)
{
    static const uint8_t urcLine[] = "\r\n+MYURC:123";
    static const uint8_t lengthHigh[] = {0x01, 0x00};
    static const uint8_t lengthLowAndData[] = {0x04, 0x00, 0x11};
    static const uint8_t remainingData[] = {0x22, 0xff};
    static const uint8_t expectedData[] = {0x00, 0x11, 0x22, 0xff};
    struct modem_response response = {
        .pDev = fixture->pDev,
        .pChunks = {urcLine, lengthHigh, lengthLowAndData, remainingData},
        .chunkLengths = {sizeof(urcLine) - 1, sizeof(lengthHigh),
                         sizeof(lengthLowAndData), sizeof(remainingData)},
        .chunkCount = 4
    };

    uCxAtClientSetUrcCallback(&fixture->client, urcCallback, NULL);
    gDisableRxWorker = false;
    startModemResponse(&response);
    waitForModemResponse();
    zassert_equal(k_sem_take(&gUrcReceived, K_SECONDS(1)), 0);
    gDisableRxWorker = true;

    zassert_equal(response.error, 0);
    zassert_equal(response.chunksSent, response.chunkCount);
    zassert_equal(gUrcCapture.pClient, &fixture->client);
    zassert_equal(gUrcCapture.lineLength, strlen("+MYURC:123"));
    zassert_equal(strcmp(gUrcCapture.line, "+MYURC:123"), 0);
    zassert_equal(gUrcCapture.binaryDataLength, sizeof(expectedData));
    zassert_mem_equal__(gUrcCapture.binaryData, expectedData,
                        sizeof(expectedData));
}

ZTEST_F(u_connect_client_port, test_command_recovers_after_timeout)
{
    static const uint8_t response[] = "\r\nOK\r\n";
    struct modem_response modemResponse = {
        .pDev = fixture->pDev,
        .pChunks = {response},
        .chunkLengths = {sizeof(response) - 1},
        .chunkCount = 1
    };

    uCxAtClientSetCommandTimeout(&fixture->client, 25, false);
    zassert_equal(uCxAtClientExecSimpleCmd(&fixture->client, "AT"),
                  U_CX_ERROR_CMD_TIMEOUT);

    startModemResponse(&modemResponse);
    zassert_equal(uCxAtClientExecSimpleCmd(&fixture->client, "AT"), 0);
    waitForModemResponse();
    zassert_equal(modemResponse.error, 0);
    zassert_equal(modemResponse.chunksSent, modemResponse.chunkCount);

    uint8_t txData[6];
    zassert_equal(uart_emul_get_tx_data(fixture->pDev, txData, sizeof(txData)),
                  sizeof(txData));
    zassert_mem_equal__(txData, "AT\rAT\r", sizeof(txData));
}

ZTEST_SUITE(u_connect_client_port, NULL, u_connect_client_port_setup, u_connect_client_port_before, u_connect_client_port_after, NULL);
