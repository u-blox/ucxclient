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

#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include "unity.h"
#include "mock_u_cx_log.h"
#include "mock_u_cx_at_config.h"
#include "mock_u_port.h"
#include "u_cx_at_util.h"
#include "u_cx_at_params.h"
#include "u_cx_at_urc_queue.h"
#include "u_cx_at_client.h"

/* ----------------------------------------------------------------
 * COMPILE-TIME MACROS
 * -------------------------------------------------------------- */

#define CONTEXT_VALUE  ((void *)0x11223344)
#define UART_HANDLE    ((uPortUartHandle_t)0x44332211)

#define BIN_HDR(DATA_LENGTH) \
    0x01,(DATA_LENGTH) >> 8,(DATA_LENGTH) & 0xFF

#define TEST_URC "+MYURC:123,\"abc\""

/* ----------------------------------------------------------------
 * TYPES
 * -------------------------------------------------------------- */

/* ----------------------------------------------------------------
 * STATIC VARIABLES
 * -------------------------------------------------------------- */

static uint8_t gRxBuffer[1024];

static uint8_t gTxBuffer[1024];
static size_t gTxBufferPos;

static uint8_t *gPRxDataPtr;
static int32_t gRxDataLen;
static int32_t gRxIoErrorCode;
static size_t gRxMaxReadSize;

static uCxAtClientConfig_t gClientConfig = {
    .pContext = CONTEXT_VALUE,
    .pRxBuffer = gRxBuffer,
    .rxBufferLen = sizeof(gRxBuffer),
    .pUartDevName = "TEST_UART",
    .timeoutMs = 10000
};

static uCxAtClient_t gClient;

/* ----------------------------------------------------------------
 * STATIC FUNCTIONS
 * -------------------------------------------------------------- */

/* Mock UART open function */
uPortUartHandle_t uPortUartOpen(const char *pDeviceName, int32_t baudRate, bool flowControl)
{
    (void)pDeviceName;
    (void)baudRate;
    (void)flowControl;
    return UART_HANDLE;
}

/* Mock UART close function */
void uPortUartClose(uPortUartHandle_t handle)
{
    TEST_ASSERT_EQUAL(UART_HANDLE, handle);
}

/* Mock BgRxTask functions (not used in tests, background task disabled) */
void uPortBgRxTaskCreate(uCxAtClient_t *pClient)
{
    (void)pClient;
}

void uPortBgRxTaskDestroy(uCxAtClient_t *pClient)
{
    (void)pClient;
}

/* Mock UART write function */
int32_t uPortUartWrite(uPortUartHandle_t handle, const void *pData, size_t length)
{
    TEST_ASSERT_EQUAL(UART_HANDLE, handle);
    assert(length < sizeof(gTxBuffer) - gTxBufferPos);
    memcpy(&gTxBuffer[gTxBufferPos], pData, length);
    gTxBufferPos += length;
    return (int32_t)length;
}

/* Mock UART read function */
int32_t uPortUartRead(uPortUartHandle_t handle, void *pData, size_t length, int32_t timeoutMs)
{
    static int zeroCounter = 0;
    (void)timeoutMs;
    TEST_ASSERT_EQUAL(UART_HANDLE, handle);

    if (gRxIoErrorCode != 0) {
        if (++zeroCounter > 10) {
            TEST_FAIL_MESSAGE("Stuck in read loop");
        }
        return gRxIoErrorCode;
    }

    size_t readLimit = length;
    if ((gRxMaxReadSize > 0) && (readLimit > gRxMaxReadSize)) {
        readLimit = gRxMaxReadSize;
    }

    int32_t cpyLen = U_MIN((int32_t)readLimit, gRxDataLen);
    if (cpyLen > 0) {
        memcpy(pData, gPRxDataPtr, cpyLen);
        gPRxDataPtr += cpyLen;
        gRxDataLen -= cpyLen;
        zeroCounter = 0;
    } else {
        if (++zeroCounter > 10) {
            TEST_FAIL_MESSAGE("Stuck in read loop");
        }
    }
    return cpyLen;
}

/* ----------------------------------------------------------------
 * TEST FUNCTIONS
 * -------------------------------------------------------------- */

void setUp(void)
{
    gClientConfig.rxBufferLen = sizeof(gRxBuffer);
    uCxLogPrintTime_Ignore();
    uCxLogIsEnabled_IgnoreAndReturn(false);
    uCxAtClientInit(&gClientConfig, &gClient);
    uCxAtClientOpen(&gClient, 115200, true);
    memset(&gTxBuffer[0], 0xc0, sizeof(gTxBuffer));
    gTxBufferPos = 0;
    gPRxDataPtr = NULL;
    gRxDataLen = -1;
    gRxIoErrorCode = 0;
    gRxMaxReadSize = 0;

    uPortGetTickTimeMs_IgnoreAndReturn(0);
}

void tearDown(void)
{
    uCxAtClientClose(&gClient);
    uCxAtClientDeinit(&gClient);
}

void test_uCxAtClientHandleRx_withStringUrc_expectUrcCallback(void)
{
    char rxData[] = { "\r\n" TEST_URC "\r\n" };
    int callbackCount = 0;
    gPRxDataPtr = (uint8_t *)&rxData[0];
    gRxDataLen = strlen(rxData);

    void urcCallback(struct uCxAtClient *pClient, void *pTag, char *pLine,
                     size_t lineLength, uint8_t *pBinaryData, size_t binaryDataLen)
    {
        TEST_ASSERT_EQUAL(&gClient, pClient);
        TEST_ASSERT_NULL(pTag);
        TEST_ASSERT_EQUAL_STRING(TEST_URC, pLine);
        TEST_ASSERT_EQUAL(strlen(pLine), lineLength);
        TEST_ASSERT_NULL(pBinaryData);
        TEST_ASSERT_EQUAL(0, binaryDataLen);
        callbackCount++;
    }

    uCxAtClientSetUrcCallback(&gClient, urcCallback, NULL);
    uCxAtClientHandleRx(&gClient);
    TEST_ASSERT_EQUAL(1, callbackCount);
}

void test_uCxAtClientHandleRx_withFragmentedBinUrc_expectUrcCallback(void)
{
    char strData[] = { "\r\n" TEST_URC };
    uint8_t binData[] = {BIN_HDR(6),0xa5,0x01,0x0d,0x0a,0x00,0xff};
    uint8_t rxData[strlen(strData) + sizeof(binData)];
    int callbackCount = 0;
    memcpy(&rxData[0], &strData[0], strlen(strData));
    memcpy(&rxData[strlen(strData)], &binData[0], sizeof(binData));
    gPRxDataPtr = &rxData[0];
    gRxDataLen = strlen(strData) + sizeof(binData);
    gRxMaxReadSize = 1;
    gClientConfig.rxBufferLen = strlen(TEST_URC) + 1 + 6;

    void urcCallback(struct uCxAtClient *pClient, void *pTag, char *pLine,
                     size_t lineLength, uint8_t *pBinaryData, size_t binaryDataLen)
    {
        uint8_t expectedBinData[] = {0xa5,0x01,0x0d,0x0a,0x00,0xff};
        TEST_ASSERT_EQUAL(&gClient, pClient);
        TEST_ASSERT_NULL(pTag);
        TEST_ASSERT_EQUAL_STRING(TEST_URC, pLine);
        TEST_ASSERT_EQUAL(strlen(pLine), lineLength);
        TEST_ASSERT_NOT_NULL(pBinaryData);
        TEST_ASSERT_EQUAL(sizeof(expectedBinData), binaryDataLen);
        TEST_ASSERT_EQUAL_MEMORY(expectedBinData, pBinaryData, sizeof(expectedBinData));
        callbackCount++;
    }

    uCxAtClientSetUrcCallback(&gClient, urcCallback, NULL);
    uCxAtClientHandleRx(&gClient);
    TEST_ASSERT_EQUAL(0, callbackCount);
    TEST_ASSERT_TRUE(gClient.isBinaryRx);
    TEST_ASSERT_EQUAL(1, gClient.binaryRx.rxHeaderCount);
    gRxDataLen = 3;
    uCxAtClientHandleRx(&gClient);
    TEST_ASSERT_EQUAL(0, callbackCount);
    TEST_ASSERT_TRUE(gClient.isBinaryRx);
    TEST_ASSERT_EQUAL(2, gClient.binaryRx.bufferPos);
    TEST_ASSERT_EQUAL(4, gClient.binaryRx.remainingDataBytes);
    gRxDataLen = 4;
    uCxAtClientHandleRx(&gClient);
    TEST_ASSERT_EQUAL(1, callbackCount);
    TEST_ASSERT_FALSE(gClient.isBinaryRx);
    TEST_ASSERT_EQUAL(0, gRxDataLen);
}

void test_uCxAtClientHandleRx_withOversizedBinUrc_expectDiscardAndRecovery(void)
{
    char strData[] = { "\r\n" TEST_URC };
    uint8_t binData[] = {BIN_HDR(6),0xa5,0x01,0x0d,0x0a,0x00,0xff};
    uint8_t rxData[strlen(strData) + sizeof(binData)];
    char nextUrc[] = { "\r\n" TEST_URC "\r\n" };
    int callbackCount = 0;

    memcpy(rxData, strData, strlen(strData));
    memcpy(&rxData[strlen(strData)], binData, sizeof(binData));
    gClientConfig.rxBufferLen = strlen(TEST_URC) + 1 + 5;

    void urcCallback(struct uCxAtClient *pClient, void *pTag, char *pLine,
                     size_t lineLength, uint8_t *pBinaryData, size_t binaryDataLen)
    {
        TEST_ASSERT_EQUAL(&gClient, pClient);
        TEST_ASSERT_NULL(pTag);
        TEST_ASSERT_EQUAL_STRING(TEST_URC, pLine);
        TEST_ASSERT_EQUAL(strlen(TEST_URC), lineLength);
        TEST_ASSERT_NULL(pBinaryData);
        TEST_ASSERT_EQUAL(0, binaryDataLen);
        callbackCount++;
    }

    uCxAtClientSetUrcCallback(&gClient, urcCallback, NULL);
    gPRxDataPtr = rxData;
    gRxDataLen = sizeof(rxData);
    uCxAtClientHandleRx(&gClient);
    TEST_ASSERT_EQUAL(0, callbackCount);
    TEST_ASSERT_FALSE(gClient.isBinaryRx);
    TEST_ASSERT_EQUAL(0, gRxDataLen);

    gClientConfig.rxBufferLen = sizeof(gRxBuffer);
    gPRxDataPtr = (uint8_t *)nextUrc;
    gRxDataLen = strlen(nextUrc);
    uCxAtClientHandleRx(&gClient);
    TEST_ASSERT_EQUAL(1, callbackCount);
}

void test_uCxAtClientHandleRx_withInterleavedBinaryHeaders_expectPerClientLength(void)
{
    uint8_t secondRxBuffer[1024];
    uCxAtClientConfig_t secondConfig = gClientConfig;
    uCxAtClient_t secondClient;
    char strData[] = { "\r\n" TEST_URC };
    uint8_t firstHeader[strlen(strData) + 2];
    uint8_t secondHeader[strlen(strData) + 2];
    uint8_t firstTail[258];
    int callbackCount = 0;

    secondConfig.pRxBuffer = secondRxBuffer;
    uCxAtClientInit(&secondConfig, &secondClient);
    TEST_ASSERT_EQUAL(0, uCxAtClientOpen(&secondClient, 115200, true));

    memcpy(firstHeader, strData, strlen(strData));
    firstHeader[strlen(strData)] = 0x01;
    firstHeader[strlen(strData) + 1] = 0x01;
    memcpy(secondHeader, strData, strlen(strData));
    secondHeader[strlen(strData)] = 0x01;
    secondHeader[strlen(strData) + 1] = 0x02;
    firstTail[0] = 0x01;
    for (size_t index = 0; index < sizeof(firstTail) - 1; index++) {
        firstTail[index + 1] = (uint8_t)index;
    }

    void urcCallback(struct uCxAtClient *pClient, void *pTag, char *pLine,
                     size_t lineLength, uint8_t *pBinaryData, size_t binaryDataLen)
    {
        TEST_ASSERT_EQUAL(&gClient, pClient);
        TEST_ASSERT_NULL(pTag);
        TEST_ASSERT_EQUAL_STRING(TEST_URC, pLine);
        TEST_ASSERT_EQUAL(strlen(TEST_URC), lineLength);
        TEST_ASSERT_EQUAL(257, binaryDataLen);
        for (size_t index = 0; index < binaryDataLen; index++) {
            TEST_ASSERT_EQUAL_UINT8((uint8_t)index, pBinaryData[index]);
        }
        callbackCount++;
    }

    uCxAtClientSetUrcCallback(&gClient, urcCallback, NULL);
    gPRxDataPtr = firstHeader;
    gRxDataLen = sizeof(firstHeader);
    uCxAtClientHandleRx(&gClient);
    TEST_ASSERT_EQUAL(1, gClient.binaryRx.rxHeaderCount);

    gPRxDataPtr = secondHeader;
    gRxDataLen = sizeof(secondHeader);
    uCxAtClientHandleRx(&secondClient);
    TEST_ASSERT_EQUAL(1, secondClient.binaryRx.rxHeaderCount);

    gPRxDataPtr = firstTail;
    gRxDataLen = sizeof(firstTail);
    uCxAtClientHandleRx(&gClient);
    TEST_ASSERT_EQUAL(1, callbackCount);
    TEST_ASSERT_FALSE(gClient.isBinaryRx);
    TEST_ASSERT_EQUAL(0, gRxDataLen);

    uCxAtClientClose(&secondClient);
    uCxAtClientDeinit(&secondClient);
}
