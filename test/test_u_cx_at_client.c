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
static uint8_t gUrcBuffer[1024];

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
    .pUrcBuffer = gUrcBuffer,
    .urcBufferLen = sizeof(gUrcBuffer),
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

static void uAtClientSendCmdVaList_wrapper(uCxAtClient_t *pClient, const char *pCmd,
                                           const char *pParamFmt, ...)
{
    va_list args;
    va_start(args, pParamFmt);
    uCxAtClientSendCmdVaList(pClient, pCmd, pParamFmt, args);
    va_end(args);

    gTxBuffer[gTxBufferPos] = 0;
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


void test_uCxAtClientSendCmdVaList_withIpAddress(void)
{
    uSockIpAddress_t ipAddr = {
        .type = U_SOCK_ADDRESS_TYPE_V4,
        .address.ipv4 = 0x00102030
    };
    uAtClientSendCmdVaList_wrapper(&gClient, "AT+FOO=", "i",
                                   &ipAddr, U_CX_AT_UTIL_PARAM_LAST);
    TEST_ASSERT_EQUAL_STRING("AT+FOO=0.16.32.48\r", &gTxBuffer[0]);
}


void test_uCxAtClientSendCmdVaList_withMacAddress(void)
{
    uMacAddress_t macAddr = {
        .address = {0x00,0x11,0x22,0x33,0x44,0x55}
    };
    uAtClientSendCmdVaList_wrapper(&gClient, "AT+FOO=", "m",
                                   &macAddr, U_CX_AT_UTIL_PARAM_LAST);
    TEST_ASSERT_EQUAL_STRING("AT+FOO=001122334455\r", &gTxBuffer[0]);
}

void test_uCxAtClientSendCmdVaList_withBdAddress(void)
{
    uBtLeAddress_t btLeAddr = {
        .type = U_BD_ADDRESS_TYPE_PUBLIC,
        .address = {0x00,0x11,0x22,0x33,0x44,0x55}
    };
    uAtClientSendCmdVaList_wrapper(&gClient, "AT+FOO=", "b",
                                   &btLeAddr, U_CX_AT_UTIL_PARAM_LAST);
    TEST_ASSERT_EQUAL_STRING("AT+FOO=001122334455p\r", &gTxBuffer[0]);
}

void test_uCxAtClientSendCmdVaList_withByteArray(void)
{
    uint8_t data[] = {0x00,0x11,0x22,0x33,0x44,0x55};
    uAtClientSendCmdVaList_wrapper(&gClient, "AT+FOO=", "h",
                                   &data[0], sizeof(data), U_CX_AT_UTIL_PARAM_LAST);
    TEST_ASSERT_EQUAL_STRING("AT+FOO=001122334455\r", &gTxBuffer[0]);
}

void test_uCxAtClientSendCmdVaList_withBinary(void)
{
    uint8_t data[] = {0x00,0x11,0x22,0x33,0x44,0x55};
    uint8_t expected[] = { 'A','T','+','F','O','O','=',BIN_HDR(6),0x00,0x11,0x22,0x33,0x44,0x55};
    uAtClientSendCmdVaList_wrapper(&gClient, "AT+FOO=", "B",
                                   &data[0], sizeof(data), U_CX_AT_UTIL_PARAM_LAST);
    TEST_ASSERT_EQUAL_MEMORY(expected, &gTxBuffer[0], sizeof(expected));
    TEST_ASSERT_EQUAL(sizeof(expected), gTxBufferPos);
}

void test_uCxAtClientExecSimpleCmdF_withStatusOk_expectSuccess(void)
{
    char rxData[] = { "\r\nOK\r\n" };
    gPRxDataPtr = (uint8_t *)&rxData[0];
    gRxDataLen = sizeof(rxData);
    TEST_ASSERT_EQUAL(0, uCxAtClientExecSimpleCmdF(&gClient, "DUMMY", ""));
}

void test_uCxAtClientExecSimpleCmdF_withStatusError_expectError(void)
{
    char rxData[] = { "\r\nERROR\r\n" };
    gPRxDataPtr = (uint8_t *)&rxData[0];
    gRxDataLen = sizeof(rxData);
    TEST_ASSERT_EQUAL(U_CX_ERROR_STATUS_ERROR, uCxAtClientExecSimpleCmdF(&gClient, "DUMMY", ""));
}

void test_uCxAtClientExecSimpleCmdF_withStatusExtendedError_expectErrorCode(void)
{
    char rxData[] = { "\r\nERROR:123\r\n" };
    gPRxDataPtr = (uint8_t *)&rxData[0];
    gRxDataLen = sizeof(rxData);
    TEST_ASSERT_EQUAL(U_CX_EXTENDED_ERROR_OFFSET - 123, uCxAtClientExecSimpleCmdF(&gClient, "DUMMY", ""));
}

void test_uCxAtClientExecSimpleCmdF_withInvalidStatusExtendedError_expectTimeout(void)
{
    char rxData[] = { "\r\nERROR:1a23\r\n" };
    gPRxDataPtr = (uint8_t *)&rxData[0];
    gRxDataLen = sizeof(rxData);
    uPortGetTickTimeMs_StopIgnore();
    uPortGetTickTimeMs_ExpectAndReturn(0);
    uPortGetTickTimeMs_ExpectAndReturn(20000);
    TEST_ASSERT_EQUAL(U_CX_ERROR_CMD_TIMEOUT, uCxAtClientExecSimpleCmdF(&gClient, "DUMMY", ""));
    TEST_ASSERT_EQUAL_MESSAGE(0, gRxDataLen, "Test didn't read all data");
}

void test_uCxAtClientExecSimpleCmdF_withReadError_expectIoError(void)
{
    gRxIoErrorCode = -1234;
    TEST_ASSERT_EQUAL(U_CX_ERROR_IO, uCxAtClientExecSimpleCmdF(&gClient, "DUMMY", ""));
    TEST_ASSERT_EQUAL(-1234, uCxAtGetLastIoError(&gClient));
}

void test_uCxAtClientCmdGetRspParamLine_withTimeout_expectNull(void)
{
    // Start by putting the client in command state
    uCxAtClientCmdBeginF(&gClient, "", "", U_CX_AT_UTIL_PARAM_LAST);

    uPortGetTickTimeMs_StopIgnore();
    uPortGetTickTimeMs_ExpectAndReturn(0);
    uPortGetTickTimeMs_ExpectAndReturn(20000);
    char rxData[] = { "+UNMATCHED_RSP\r\n" };
    gPRxDataPtr = (uint8_t *)&rxData[0];
    gRxDataLen = sizeof(rxData);
    TEST_ASSERT_EQUAL(NULL, uCxAtClientCmdGetRspParamLine(&gClient, "DUMMY", NULL, NULL));
}

void test_uCxAtClientCmdGetRspParamLine_withCmdEchoAndRsp_expectRsp(void)
{
    // Start by putting the client in command state
    uCxAtClientCmdBeginF(&gClient, "", "", U_CX_AT_UTIL_PARAM_LAST);

    char rxData[] = { "AT+FOO\r\n+MYRSP:123\r\n" };
    gPRxDataPtr = (uint8_t *)&rxData[0];
    gRxDataLen = sizeof(rxData);
    char *pRsp = uCxAtClientCmdGetRspParamLine(&gClient, "+MYRSP:", NULL, NULL);
    TEST_ASSERT_NOT_NULL(pRsp);
    TEST_ASSERT_EQUAL_STRING("123", pRsp);
}

void test_uCxAtClientCmdGetRspParamLine_withReadError_expectNull(void)
{
    // Start by putting the client in command state
    uCxAtClientCmdBeginF(&gClient, "", "", U_CX_AT_UTIL_PARAM_LAST);

    gRxIoErrorCode = -1234;
    TEST_ASSERT_EQUAL(NULL, uCxAtClientCmdGetRspParamLine(&gClient, "DUMMY", NULL, NULL));
}

void test_uCxAtClientCmdGetRspParamLine_withBinary(void)
{
    uint8_t binaryBuf[6] = {0};
    size_t binaryLen = sizeof(binaryBuf);
    uint8_t rxData[] = { '+','F','O','O',':','\"','f','o','o','\"',BIN_HDR(6),0x00,0x11,0x22,0x33,0x44,0x55};
    uint8_t expectedBinData[] = {0x00,0x11,0x22,0x33,0x44,0x55};

    // Start by putting the client in command state
    uCxAtClientCmdBeginF(&gClient, "", "", U_CX_AT_UTIL_PARAM_LAST);

    gPRxDataPtr = &rxData[0];
    gRxDataLen = sizeof(rxData);
    char *pRsp = uCxAtClientCmdGetRspParamLine(&gClient, "+FOO:", binaryBuf, &binaryLen);
    TEST_ASSERT_EQUAL_MEMORY(expectedBinData, binaryBuf, sizeof(binaryBuf));
    TEST_ASSERT_EQUAL(sizeof(binaryBuf), binaryLen);
    TEST_ASSERT_EQUAL_STRING("\"foo\"", pRsp);
}

void test_uCxAtClientCmdGetRspParamLine_withUnexpectedBinaryResponse(void)
{
    uint8_t rxData[] = { '+','F','O','O',':','\"','f','o','o','\"',BIN_HDR(6),0x00,0x11,0x22,0x33,0x44,0x55};

    // Start by putting the client in command state
    uCxAtClientCmdBeginF(&gClient, "", "", U_CX_AT_UTIL_PARAM_LAST);

    gPRxDataPtr = &rxData[0];
    gRxDataLen = sizeof(rxData);
    char *pRsp = uCxAtClientCmdGetRspParamLine(&gClient, "+FOO:", NULL, NULL);
    TEST_ASSERT_EQUAL_STRING("\"foo\"", pRsp);
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
    gRxDataLen = strlen(strData) + sizeof(binData);

    void urcCallback(struct uCxAtClient *pClient, void *pTag, char *pLine,
                     size_t lineLength, uint8_t *pBinaryData, size_t binaryDataLen)
    {
        uint8_t expectedBinData[] = {0x00,0x11,0x22,0x33,0x44,0x55};
        TEST_ASSERT_EQUAL(&gClient, pClient);
        TEST_ASSERT_NULL(pTag);
        TEST_ASSERT_EQUAL_STRING(TEST_URC, pLine);
        TEST_ASSERT_EQUAL(strlen(pLine), lineLength);
        TEST_ASSERT_NOT_NULL(pBinaryData);
        TEST_ASSERT_EQUAL(sizeof(expectedBinData), binaryDataLen);
        TEST_ASSERT_EQUAL_MEMORY(expectedBinData, pBinaryData, sizeof(expectedBinData));
    }

    uCxAtClientSetUrcCallback(&gClient, urcCallback, NULL);
    uCxAtClientHandleRx(&gClient);
}
