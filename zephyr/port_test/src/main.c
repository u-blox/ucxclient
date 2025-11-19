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

/* ----------------------------------------------------------------
 * COMPILE-TIME MACROS
 * -------------------------------------------------------------- */

#define EMUL_UART_NODE	       	DT_NODELABEL(euart0)
#define EMUL_UART_RX_FIFO_SIZE 	DT_PROP(EMUL_UART_NODE, rx_fifo_size)
#define EMUL_UART_TX_FIFO_SIZE 	DT_PROP(EMUL_UART_NODE, tx_fifo_size)

#define U_RINGBUFFER_SIZE       128

#define TEST_DATA_SIZE          (U_RINGBUFFER_SIZE * 2)

#define TIMESTAMP_CREATE()      int64_t __timestamp = k_uptime_get();

#define TIMESTAMP_CHECK_TIME(expectMs)                                          \
    do {                                                                        \
        int64_t deltaMs =  k_uptime_delta(&__timestamp);                        \
        zassert_within(deltaMs, expectMs, 30, "took: %" PRId64 " ms", deltaMs); \
    } while (0);

/* ----------------------------------------------------------------
 * TYPES
 * -------------------------------------------------------------- */

struct ucxclient_port_fixture {
    const struct device *pDev;
    uint8_t testData[TEST_DATA_SIZE];
    uCxAtClient_t client;
    uint8_t rxBuffer[U_RINGBUFFER_SIZE];
};

/* ----------------------------------------------------------------
 * VARIABLES
 * -------------------------------------------------------------- */

extern bool gDisableRxWorker;

/* ----------------------------------------------------------------
 * TEST SETUP
 * -------------------------------------------------------------- */

static void *ucxclient_port_setup(void)
{
    static struct ucxclient_port_fixture fixture = {
        .pDev = DEVICE_DT_GET(EMUL_UART_NODE)
    };

    for (size_t i = 0; i < TEST_DATA_SIZE; i++) {
        fixture.testData[i] = i;
    }

    zassert_not_null(fixture.pDev);

    uPortAtInit(&fixture.client);

    return &fixture;
}

static void ucxclient_port_before(void *f)
{
    struct ucxclient_port_fixture *fixture = f;

    uart_irq_tx_disable(fixture->pDev);
    uart_irq_rx_disable(fixture->pDev);

    uart_emul_flush_rx_data(fixture->pDev);
    uart_emul_flush_tx_data(fixture->pDev);

    uart_err_check(fixture->pDev);

    memset(&fixture->rxBuffer, 0, sizeof(fixture->rxBuffer));

    gDisableRxWorker = true;
    zassert_true(uPortAtOpen(&fixture->client, fixture->pDev->name, 115200, true));
}

static void ucxclient_port_after(void *f)
{
    struct ucxclient_port_fixture *fixture = f;

    uPortAtClose(&fixture->client);
}

/* ----------------------------------------------------------------
 * TESTS
 * -------------------------------------------------------------- */

ZTEST_F(ucxclient_port, test_rx_no_data)
{
    const struct uCxAtClientConfig *pCfg = fixture->client.pConfig;

    TIMESTAMP_CREATE();
    int32_t rc = pCfg->read(&fixture->client, pCfg->pStreamHandle, NULL, 1, 0);
    TIMESTAMP_CHECK_TIME(0);
    zassert_equal(rc, 0, "read() returned: %d", rc);
}

ZTEST_F(ucxclient_port, test_rx_no_data_timeout)
{
    const struct uCxAtClientConfig *pCfg = fixture->client.pConfig;

    TIMESTAMP_CREATE();
    int32_t rc = pCfg->read(&fixture->client, pCfg->pStreamHandle, NULL, 1, 100);
    TIMESTAMP_CHECK_TIME(100);
    zassert_equal(rc, 0, "read() returned: %d", rc);
}

ZTEST_F(ucxclient_port, test_rx_some_data_timeout)
{
    const struct uCxAtClientConfig *pCfg = fixture->client.pConfig;

    uart_emul_put_rx_data(fixture->pDev, &fixture->testData[0], 3);

    TIMESTAMP_CREATE();
    int32_t rc = pCfg->read(&fixture->client, pCfg->pStreamHandle,
                            &fixture->rxBuffer, 4, 100);
    TIMESTAMP_CHECK_TIME(100);
    zassert_equal(rc, 3, "read() returned: %d", rc);
    zassert_mem_equal__(&fixture->rxBuffer, &fixture->testData, 3);
}

ZTEST_F(ucxclient_port, test_rx_read_some_data_timeout)
{
    const struct uCxAtClientConfig *pCfg = fixture->client.pConfig;

    uart_emul_put_rx_data(fixture->pDev, &fixture->testData[0], 3);

    TIMESTAMP_CREATE();
    int32_t rc = pCfg->read(&fixture->client, pCfg->pStreamHandle,
                            &fixture->rxBuffer, 2, 100);
    TIMESTAMP_CHECK_TIME(0);
    zassert_equal(rc, 2, "read() returned: %d", rc);
    zassert_mem_equal__(&fixture->rxBuffer, &fixture->testData, 2);
}

ZTEST_F(ucxclient_port, test_rx_read_some_data_no_timeout)
{
    const struct uCxAtClientConfig *pCfg = fixture->client.pConfig;

    uart_emul_put_rx_data(fixture->pDev, &fixture->testData[0], 3);
    // Need a little sleep here so that the ISR receives all the data before next step
    k_sleep(K_MSEC(10));

    TIMESTAMP_CREATE();
    int32_t rc = pCfg->read(&fixture->client, pCfg->pStreamHandle,
                            &fixture->rxBuffer, 2, 0);
    TIMESTAMP_CHECK_TIME(0);
    zassert_equal(rc, 2, "read() returned: %d", rc);
    zassert_mem_equal__(&fixture->rxBuffer, &fixture->testData, 2);
}

ZTEST_F(ucxclient_port, test_rx_all_data)
{
    const struct uCxAtClientConfig *pCfg = fixture->client.pConfig;

    uart_emul_put_rx_data(fixture->pDev, &fixture->testData[0], 3);

    TIMESTAMP_CREATE();
    int32_t rc = pCfg->read(&fixture->client, pCfg->pStreamHandle,
                            &fixture->rxBuffer, 3, 100);
    TIMESTAMP_CHECK_TIME(0);
    zassert_equal(rc, 3, "read() returned: %d", rc);
    zassert_mem_equal__(&fixture->rxBuffer, &fixture->testData, 3);
}

ZTEST_F(ucxclient_port, test_rx_ringbuf_full)
{
    const struct uCxAtClientConfig *pCfg = fixture->client.pConfig;

    // Receive a little more data than can be fitted into the ring buffer
    uart_emul_put_rx_data(fixture->pDev, &fixture->testData[0], U_RINGBUFFER_SIZE + 8);

    // Need a little sleep here so that the ISR receives all the data before next step
    k_sleep(K_MSEC(10));

    TIMESTAMP_CREATE();
    int32_t rc = pCfg->read(&fixture->client, pCfg->pStreamHandle, &fixture->rxBuffer, 8, 100);
    TIMESTAMP_CHECK_TIME(0);
    zassert_equal(rc, 8, "read() returned: %d", rc);
    rc = pCfg->read(&fixture->client, pCfg->pStreamHandle,
                    &fixture->rxBuffer[8], U_RINGBUFFER_SIZE, 100);
    TIMESTAMP_CHECK_TIME(0);
    zassert_equal(rc, U_RINGBUFFER_SIZE, "read() returned: %d", rc);
    zassert_mem_equal__(&fixture->rxBuffer, &fixture->testData, U_RINGBUFFER_SIZE + 8);

    // Everything should be read now - make sure read returns 0
    rc = pCfg->read(&fixture->client, pCfg->pStreamHandle,
                    &fixture->rxBuffer[0], U_RINGBUFFER_SIZE, 100);
    TIMESTAMP_CHECK_TIME(100);
    zassert_equal(rc, 0, "read() returned: %d", rc);
}

ZTEST_F(ucxclient_port, test_tx_fifo_full)
{
    const struct uCxAtClientConfig *pCfg = fixture->client.pConfig;

    int32_t rc = pCfg->write(&fixture->client, pCfg->pStreamHandle,
                             &fixture->testData[0], EMUL_UART_TX_FIFO_SIZE + 8);
    zassert_equal(rc, EMUL_UART_TX_FIFO_SIZE, "write() returned: %d", rc);

    rc = uart_emul_get_tx_data(fixture->pDev, &fixture->rxBuffer[0], EMUL_UART_TX_FIFO_SIZE + 8);
    zassert_equal(rc, EMUL_UART_TX_FIFO_SIZE, "uart_emul_get_tx_data() returned: %d", rc);
    zassert_mem_equal__(&fixture->rxBuffer, &fixture->testData, U_RINGBUFFER_SIZE);
}

ZTEST_SUITE(ucxclient_port, NULL, ucxclient_port_setup, ucxclient_port_before, ucxclient_port_after, NULL);
