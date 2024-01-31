/*
 * Copyright 2024 u-blox
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
#include "u_cx_at_util.h"
#include "u_cx_at_params.h"
#include "u_cx_at_urc_queue.h"
#include "u_cx_at_client.h"

/* ----------------------------------------------------------------
 * COMPILE-TIME MACROS
 * -------------------------------------------------------------- */

#define CONTEXT_VALUE  ((void *)0x11223344)
#define STREAM_HANDLER ((void *)0x44332211)

#define BIN_HDR(DATA_LENGTH) \
    0x01,(DATA_LENGTH) >> 8,(DATA_LENGTH) & 0xFF

#define TEST_URC "+MYURC:123,\"abc\""

/* ----------------------------------------------------------------
 * TYPES
 * -------------------------------------------------------------- */

static int32_t write(uCxAtClient_t *pClient, void *pStreamHandle,
                     const void *pData, size_t length);

static int32_t read(uCxAtClient_t *pClient, void *pStreamHandle,
                    void *pData, size_t length, int32_t timeoutMs);


/* ----------------------------------------------------------------
 * STATIC VARIABLES
 * -------------------------------------------------------------- */

static uint8_t gRxBuffer[1024];

static uint8_t gTxBuffer[1024];
static size_t gTxBufferPos;

static uint8_t *gPRxDataPtr;
static int32_t gRxDataLen;
static int32_t gRxIoErrorCode;

static uCxAtClientConfig_t gClientConfig = {
    .pContext = CONTEXT_VALUE,
    .pRxBuffer = gRxBuffer,
    .rxBufferLen = sizeof(gRxBuffer),
    .pStreamHandle = STREAM_HANDLER,
    .read = read,
    .write = write,
};

static uCxAtClient_t gClient;

/* ----------------------------------------------------------------
 * STATIC FUNCTIONS
 * -------------------------------------------------------------- */

static int32_t write(uCxAtClient_t *pClient, void *pStreamHandle,
                     const void *pData, size_t length)
{
    TEST_ASSERT_EQUAL(&gClient, pClient);
    TEST_ASSERT_EQUAL(STREAM_HANDLER, pStreamHandle);
    assert(length < sizeof(gTxBuffer) - gTxBufferPos);
    memcpy(&gTxBuffer[gTxBufferPos], pData, length);
    gTxBufferPos += length;
    return length;
}

static int32_t read(uCxAtClient_t *pClient, void *pStreamHandle,
                    void *pData, size_t length, int32_t timeoutMs)
{
    static int zeroCounter = 0;
    (void)timeoutMs;
    TEST_ASSERT_EQUAL(&gClient, pClient);
    TEST_ASSERT_EQUAL(STREAM_HANDLER, pStreamHandle);

    if (gRxIoErrorCode != 0) {
        if (++zeroCounter > 10) {
            TEST_FAIL_MESSAGE("Stuck in read loop");
        }
        return gRxIoErrorCode;
    }

    int32_t cpyLen = U_MIN((int32_t)length, gRxDataLen);
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
    uCxLogPrintTime_Ignore();
    uCxLogIsEnabled_IgnoreAndReturn(false);
    uCxAtClientInit(&gClientConfig, &gClient);
    memset(&gTxBuffer[0], 0xc0, sizeof(gTxBuffer));
    gTxBufferPos = 0;
    gPRxDataPtr = NULL;
    gRxDataLen = -1;
    gRxIoErrorCode = 0;

    uPortGetTickTimeMs_IgnoreAndReturn(0);
}

void tearDown(void)
{
}

void test_uCxAtClientHandleRx_withStringUrc_expectUrcCallback(void)
{
    char rxData[] = { "\r\n" TEST_URC "\r\n" };
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
    }

    uCxAtClientSetUrcCallback(&gClient, urcCallback, NULL);
    uCxAtClientHandleRx(&gClient);
}

void test_uCxAtClientHandleRx_withBinUrc_expectUrcCallback(void)
{
    char strData[] = { "\r\n" TEST_URC };
    uint8_t binData[] = {BIN_HDR(6),0x00,0x11,0x22,0x33,0x44,0x55};
    uint8_t rxData[strlen(strData) + sizeof(binData)];
    memcpy(&rxData[0], &strData[0], strlen(strData));
    memcpy(&rxData[strlen(strData)], &binData[0], sizeof(binData));
    gPRxDataPtr = &rxData[0];
    gRxDataLen += strlen(strData) + sizeof(binData);

    void urcCallback(struct uCxAtClient *pClient, void *pTag, char *pLine,
                     size_t lineLength, uint8_t *pBinaryData, size_t binaryDataLen)
    {
        TEST_ASSERT_EQUAL(&gClient, pClient);
        TEST_ASSERT_NULL(pTag);
        TEST_ASSERT_EQUAL_STRING(TEST_URC, pLine);
        TEST_ASSERT_EQUAL(strlen(pLine), lineLength);
        TEST_ASSERT_NULL(pBinaryData);
        TEST_ASSERT_EQUAL(0, binaryDataLen);
    }

    uCxAtClientSetUrcCallback(&gClient, urcCallback, NULL);
    uCxAtClientHandleRx(&gClient);
}
