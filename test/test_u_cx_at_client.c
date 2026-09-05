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
static uint8_t gUrcBuffer[1024];

static uint8_t gTxBuffer[1024];
static size_t gTxBufferPos;

static uint8_t *gPRxDataPtr;
static int32_t gRxDataLen;
static int32_t gRxIoErrorCode;
static int32_t gRxBytesUntilError;
static size_t gRxMaxReadSize;

static uCxAtClientConfig_t gClientConfig = {
    .pContext = CONTEXT_VALUE,
    .pRxBuffer = gRxBuffer,
    .rxBufferLen = sizeof(gRxBuffer),
    .pUrcBuffer = gUrcBuffer,
    .urcBufferLen = sizeof(gUrcBuffer),
    .pUartDevName = "TEST_UART",
    .timeoutMs = 10000
};

static uCxAtClient_t gClient;
static int32_t *gPTickSequence;

/* ----------------------------------------------------------------
 * STATIC FUNCTIONS
 * -------------------------------------------------------------- */

int32_t uPortGetTickTimeMs_CALLBACK(int cmock_num_calls)
{
    // Helper function for returning a sequence of ticks
    // uPortGetTickTimeMs_ExpectAndReturn() cannot be used in some cases as
    // CMock will only check the number of calls after the complete test is
    // completed.
    int32_t ret;
    (void)cmock_num_calls;
    TEST_ASSERT_NOT_NULL(gPTickSequence);
    TEST_ASSERT_NOT_EQUAL_MESSAGE(-1, *gPTickSequence, "Timeout not triggered");
    ret = *gPTickSequence;
    gPTickSequence++;
    return ret;
}

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

    if ((gRxIoErrorCode != 0) && (gRxBytesUntilError == 0)) {
        if (++zeroCounter > 10) {
            TEST_FAIL_MESSAGE("Stuck in read loop");
        }
        return gRxIoErrorCode;
    }

    size_t readLimit = length;
    if ((gRxMaxReadSize > 0) && (readLimit > gRxMaxReadSize)) {
        readLimit = gRxMaxReadSize;
    }
    if ((gRxBytesUntilError > 0) && (readLimit > (size_t)gRxBytesUntilError)) {
        readLimit = (size_t)gRxBytesUntilError;
    }

    int32_t cpyLen = U_MIN((int32_t)readLimit, gRxDataLen);
    if (cpyLen > 0) {
        memcpy(pData, gPRxDataPtr, cpyLen);
        gPRxDataPtr += cpyLen;
        gRxDataLen -= cpyLen;
        if (gRxBytesUntilError > 0) {
            gRxBytesUntilError -= cpyLen;
        }
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
    uCxAtClientOpen(&gClient, 115200, true);
    memset(&gTxBuffer[0], 0xc0, sizeof(gTxBuffer));
    gTxBufferPos = 0;
    gPRxDataPtr = NULL;
    gRxDataLen = -1;
    gRxIoErrorCode = 0;
    gRxBytesUntilError = 0;
    gRxMaxReadSize = 0;
    gPTickSequence = NULL;

    uPortGetTickTimeMs_IgnoreAndReturn(0);
}

void tearDown(void)
{
    if (gClient.executingCmd) {
        gClient.executingCmd = false;
        U_CX_MUTEX_UNLOCK(gClient.cmdMutex);
    }
    uCxAtClientClose(&gClient);
    uCxAtClientDeinit(&gClient);
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

void test_uCxAtClientSendCmdVaList_withIntList(void)
{
    int16_t values[] = {1, 2, 3};
    uAtClientSendCmdVaList_wrapper(&gClient, "AT+FOO=", "l",
                                   values, (size_t)3, U_CX_AT_UTIL_PARAM_LAST);
    TEST_ASSERT_EQUAL_STRING("AT+FOO=[1,2,3]\r", &gTxBuffer[0]);
}

void test_uCxAtClientSendCmdVaList_withEmptyIntList(void)
{
    int16_t *values = NULL;
    uAtClientSendCmdVaList_wrapper(&gClient, "AT+FOO=", "l",
                                   values, (size_t)0, U_CX_AT_UTIL_PARAM_LAST);
    TEST_ASSERT_EQUAL_STRING("AT+FOO=[]\r", &gTxBuffer[0]);
}

void test_uCxAtClientSendCmdVaList_withNegativeIntList(void)
{
    int16_t values[] = {-1, -100, 50};
    uAtClientSendCmdVaList_wrapper(&gClient, "AT+FOO=", "l",
                                   values, (size_t)3, U_CX_AT_UTIL_PARAM_LAST);
    TEST_ASSERT_EQUAL_STRING("AT+FOO=[-1,-100,50]\r", &gTxBuffer[0]);
}

void test_uCxAtClientSendCmdVaList_withMultipleParams(void)
{
    uAtClientSendCmdVaList_wrapper(&gClient, "AT+FOO=", "ds",
                                   123, "abc", U_CX_AT_UTIL_PARAM_LAST);
    TEST_ASSERT_EQUAL_STRING("AT+FOO=123,\"abc\"\r", &gTxBuffer[0]);
}

void test_uCxAtClientSendCmdVaList_withBinaryString(void)
{
    char str[] = "test";
    uAtClientSendCmdVaList_wrapper(&gClient, "AT+FOO=", "$",
                                   str, (size_t)4, U_CX_AT_UTIL_PARAM_LAST);
    TEST_ASSERT_EQUAL_STRING("AT+FOO=\"test\"\r", &gTxBuffer[0]);
}

void test_uCxAtClientSendCmdVaList_withBinaryStringWithEscapes(void)
{
    char str[] = "te\"st";
    uAtClientSendCmdVaList_wrapper(&gClient, "AT+FOO=", "$",
                                   str, (size_t)5, U_CX_AT_UTIL_PARAM_LAST);
    // Expected: AT+FOO="te\"st"\r where \" is backslash-quote (escaped quote)
    const char expected[] = {'A','T','+','F','O','O','=','"','t','e','\\','\"','s','t','"','\r','\0'};
    TEST_ASSERT_EQUAL_STRING(expected, &gTxBuffer[0]);
}

void test_uCxAtClientSendCmdVaList_withBinaryStringWithNullChar(void)
{
    char str[] = {'t', 'e', '\0', 's', 't'};
    uAtClientSendCmdVaList_wrapper(&gClient, "AT+FOO=", "$",
                                   str, (size_t)5, U_CX_AT_UTIL_PARAM_LAST);
    // Expected: AT+FOO="te\0st"\r where \0 is backslash followed by '0' (escaped null)
    const char expected[] = {'A','T','+','F','O','O','=','"','t','e','\\','0','s','t','"','\r','\0'};
    TEST_ASSERT_EQUAL_MEMORY(expected, &gTxBuffer[0], sizeof(expected) - 1);
}

void test_uCxAtClientSendCmdVaList_withHexSmallData(void)
{
    uint8_t data[] = {0x01, 0x02, 0x03};
    uAtClientSendCmdVaList_wrapper(&gClient, "AT+FOO=", "h",
                                   data, (int32_t)sizeof(data), U_CX_AT_UTIL_PARAM_LAST);
    TEST_ASSERT_EQUAL_STRING("AT+FOO=010203\r", &gTxBuffer[0]);
}

void test_uCxAtClientSendCmdVaList_withHexLargeData(void)
{
    // Test with data larger than chunk size to verify chunking works
    uint8_t data[30];
    for (int i = 0; i < 30; i++) {
        data[i] = (uint8_t)i;
    }
    uAtClientSendCmdVaList_wrapper(&gClient, "AT+FOO=", "h",
                                   data, (int32_t)sizeof(data), U_CX_AT_UTIL_PARAM_LAST);
    TEST_ASSERT_EQUAL_STRING("AT+FOO=000102030405060708090A0B0C0D0E0F101112131415161718191A1B1C1D\r", &gTxBuffer[0]);
}

void test_uCxAtClientSendCmdVaList_withHexEmptyData(void)
{
    uint8_t data[] = {0x00};
    uAtClientSendCmdVaList_wrapper(&gClient, "AT+FOO=", "h",
                                   data, (int32_t)0, U_CX_AT_UTIL_PARAM_LAST);
    TEST_ASSERT_EQUAL_STRING("AT+FOO=\r", &gTxBuffer[0]);
}

void test_uCxAtClientSendCmdVaList_withStringEscapeQuote(void)
{
    char str[] = "te\"st";
    uAtClientSendCmdVaList_wrapper(&gClient, "AT+FOO=", "s",
                                   str, U_CX_AT_UTIL_PARAM_LAST);
    const char expected[] = {'A','T','+','F','O','O','=','"','t','e','\\','\"','s','t','"','\r','\0'};
    TEST_ASSERT_EQUAL_STRING(expected, &gTxBuffer[0]);
}

void test_uCxAtClientSendCmdVaList_withStringEscapeBackslash(void)
{
    char str[] = "te\\\\st";
    uAtClientSendCmdVaList_wrapper(&gClient, "AT+FOO=", "s",
                                   str, U_CX_AT_UTIL_PARAM_LAST);
    const char expected[] = {'A','T','+','F','O','O','=','"','t','e','\\','\\','\\','\\','s','t','"','\r','\0'};
    TEST_ASSERT_EQUAL_STRING(expected, &gTxBuffer[0]);
}

void test_uCxAtClientSendCmdVaList_withStringEscapeNewline(void)
{
    char str[] = "te\nst";
    uAtClientSendCmdVaList_wrapper(&gClient, "AT+FOO=", "s",
                                   str, U_CX_AT_UTIL_PARAM_LAST);
    const char expected[] = {'A','T','+','F','O','O','=','"','t','e','\\','n','s','t','"','\r','\0'};
    TEST_ASSERT_EQUAL_STRING(expected, &gTxBuffer[0]);
}

void test_uCxAtClientSendCmdVaList_withStringEscapeCarriageReturn(void)
{
    char str[] = "te\rst";
    uAtClientSendCmdVaList_wrapper(&gClient, "AT+FOO=", "s",
                                   str, U_CX_AT_UTIL_PARAM_LAST);
    const char expected[] = {'A','T','+','F','O','O','=','"','t','e','\\','r','s','t','"','\r','\0'};
    TEST_ASSERT_EQUAL_STRING(expected, &gTxBuffer[0]);
}

void test_uCxAtClientSendCmdVaList_withStringEscapeTab(void)
{
    char str[] = "te\tst";
    uAtClientSendCmdVaList_wrapper(&gClient, "AT+FOO=", "s",
                                   str, U_CX_AT_UTIL_PARAM_LAST);
    const char expected[] = {'A','T','+','F','O','O','=','"','t','e','\\','t','s','t','"','\r','\0'};
    TEST_ASSERT_EQUAL_STRING(expected, &gTxBuffer[0]);
}

void test_uCxAtClientSendCmdVaList_withStringEscapeBackspace(void)
{
    char str[] = "te\bst";
    uAtClientSendCmdVaList_wrapper(&gClient, "AT+FOO=", "s",
                                   str, U_CX_AT_UTIL_PARAM_LAST);
    const char expected[] = {'A','T','+','F','O','O','=','"','t','e','\\','b','s','t','"','\r','\0'};
    TEST_ASSERT_EQUAL_STRING(expected, &gTxBuffer[0]);
}

void test_uCxAtClientSendCmdVaList_withStringEscapeNonPrintable(void)
{
    char str[] = {'t','e','\x01','s','t','\0'};
    uAtClientSendCmdVaList_wrapper(&gClient, "AT+FOO=", "s",
                                   str, U_CX_AT_UTIL_PARAM_LAST);
    const char expected[] = {'A','T','+','F','O','O','=','"','t','e','\\','x','0','1','s','t','"','\r','\0'};
    TEST_ASSERT_EQUAL_STRING(expected, &gTxBuffer[0]);
}

void test_uCxAtClientExecSimpleCmdF_withStatusOk_expectSuccess(void)
{
    char rxData[] = { "\r\nOK\r\n" };
    gPRxDataPtr = (uint8_t *)&rxData[0];
    gRxDataLen = sizeof(rxData);
    TEST_ASSERT_EQUAL(0, uCxAtClientExecSimpleCmdF(&gClient, "DUMMY", ""));
}

void test_uCxAtClientExecSimpleCmd_withStatusOk_expectSuccess(void)
{
    char rxData[] = { "\r\nOK\r\n" };
    gPRxDataPtr = (uint8_t *)&rxData[0];
    gRxDataLen = sizeof(rxData);

    TEST_ASSERT_EQUAL(0, uCxAtClientExecSimpleCmd(&gClient, "AT"));
    TEST_ASSERT_EQUAL(3, gTxBufferPos);
    TEST_ASSERT_EQUAL_MEMORY("AT\r", gTxBuffer, gTxBufferPos);
}

void test_uCxAtClientExecSimpleCmdF_withControlByte_expectSuccess(void)
{
    uint8_t rxData[] = { '\r','\n',0x02,'O','K','\r','\n' };
    gPRxDataPtr = rxData;
    gRxDataLen = sizeof(rxData);

    TEST_ASSERT_EQUAL(0, uCxAtClientExecSimpleCmdF(&gClient, "DUMMY", ""));
}

void test_uCxAtClientExecSimpleCmdF_afterRxOverflow_expectSuccess(void)
{
    uint8_t rxData[] = { '1','2','3','4','5','6','7','8','O','K','\r','\n' };
    size_t originalBufferLen = gClientConfig.rxBufferLen;
    gClientConfig.rxBufferLen = 8;
    gPRxDataPtr = rxData;
    gRxDataLen = sizeof(rxData);

    TEST_ASSERT_EQUAL(0, uCxAtClientExecSimpleCmdF(&gClient, "DUMMY", ""));

    gClientConfig.rxBufferLen = originalBufferLen;
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
    TEST_ASSERT_EQUAL(-1234, uCxAtClientGetLastIoError(&gClient));
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

void test_uCxAtClientCmdGetRspParamLine_withoutPrefix_expectRsp(void)
{
    char rxData[] = { "NORA-W36\r\nOK\r\n" };
    gPRxDataPtr = (uint8_t *)&rxData[0];
    gRxDataLen = sizeof(rxData);
    uCxAtClientCmdBeginF(&gClient, "", "", U_CX_AT_UTIL_PARAM_LAST);

    char *pRsp = uCxAtClientCmdGetRspParamLine(&gClient, NULL, NULL, NULL);

    TEST_ASSERT_NOT_NULL(pRsp);
    TEST_ASSERT_EQUAL_STRING("NORA-W36", pRsp);
    TEST_ASSERT_EQUAL(0, uCxAtClientCmdEnd(&gClient));
}

void test_uCxAtClientCmdGetRspParamsF_withValidParams_expectParsedValues(void)
{
    char rxData[] = { "+FOO:123,\"abc\"\r\nOK\r\n" };
    int32_t value = 0;
    char *pString = NULL;
    gPRxDataPtr = (uint8_t *)&rxData[0];
    gRxDataLen = sizeof(rxData);
    uCxAtClientCmdBeginF(&gClient, "", "", U_CX_AT_UTIL_PARAM_LAST);

    TEST_ASSERT_EQUAL(2, uCxAtClientCmdGetRspParamsF(&gClient, "+FOO:", NULL, NULL,
                                                    "ds", &value, &pString,
                                                    U_CX_AT_UTIL_PARAM_LAST));
    TEST_ASSERT_EQUAL(123, value);
    TEST_ASSERT_EQUAL_STRING("abc", pString);
    TEST_ASSERT_EQUAL(0, uCxAtClientCmdEnd(&gClient));
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
    uint16_t binaryLen = sizeof(binaryBuf);
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

void test_uCxAtClientCmdGetRspParamLine_withFragmentedBinary_expectCompleteResponse(void)
{
    uint8_t binaryBuf[6] = {0};
    uint16_t binaryLen = sizeof(binaryBuf);
    uint8_t rxData[] = { '+','F','O','O',':',BIN_HDR(6),0x00,0x11,0x22,0x33,0x44,0x55};
    uint8_t expectedBinData[] = {0x00,0x11,0x22,0x33,0x44,0x55};

    uCxAtClientCmdBeginF(&gClient, "", "", U_CX_AT_UTIL_PARAM_LAST);
    gPRxDataPtr = rxData;
    gRxDataLen = sizeof(rxData);
    gRxMaxReadSize = 1;

    char *pRsp = uCxAtClientCmdGetRspParamLine(&gClient, "+FOO:", binaryBuf, &binaryLen);

    TEST_ASSERT_NOT_NULL(pRsp);
    TEST_ASSERT_EQUAL_STRING("", pRsp);
    TEST_ASSERT_EQUAL(sizeof(expectedBinData), binaryLen);
    TEST_ASSERT_EQUAL_MEMORY(expectedBinData, binaryBuf, sizeof(expectedBinData));
}

void test_uCxAtClientCmdGetRspParamLine_withFragmentedZeroLengthBinary_expectCompleteResponse(void)
{
    uint8_t binaryBuf[1] = {0xa5};
    uint16_t binaryLen = sizeof(binaryBuf);
    uint8_t rxData[] = { '+','F','O','O',':',BIN_HDR(0)};

    uCxAtClientCmdBeginF(&gClient, "", "", U_CX_AT_UTIL_PARAM_LAST);
    gPRxDataPtr = rxData;
    gRxDataLen = sizeof(rxData);
    gRxMaxReadSize = 1;

    char *pRsp = uCxAtClientCmdGetRspParamLine(&gClient, "+FOO:", binaryBuf, &binaryLen);

    TEST_ASSERT_NOT_NULL(pRsp);
    TEST_ASSERT_EQUAL_STRING("", pRsp);
    TEST_ASSERT_EQUAL(0, binaryLen);
    TEST_ASSERT_EQUAL_HEX8(0xa5, binaryBuf[0]);
    TEST_ASSERT_FALSE(gClient.isBinaryRx);
}

void test_uCxAtClientCmdGetRspParamLine_withBinaryHeaderReadError_expectIoError(void)
{
    uint8_t binaryBuf[6] = {0};
    uint16_t binaryLen = sizeof(binaryBuf);
    uint8_t rxData[] = { '+','F','O','O',':',BIN_HDR(6),0x00,0x11,0x22,0x33,0x44,0x55};

    uCxAtClientCmdBeginF(&gClient, "", "", U_CX_AT_UTIL_PARAM_LAST);
    gPRxDataPtr = rxData;
    gRxDataLen = sizeof(rxData);
    gRxIoErrorCode = -1234;
    gRxBytesUntilError = 7;

    TEST_ASSERT_NULL(uCxAtClientCmdGetRspParamLine(&gClient, "+FOO:", binaryBuf, &binaryLen));
    TEST_ASSERT_EQUAL(U_CX_ERROR_IO, gClient.status);
    TEST_ASSERT_EQUAL(-1234, uCxAtClientGetLastIoError(&gClient));
    TEST_ASSERT_EQUAL(1, gClient.binaryRx.rxHeaderCount);
}

void test_uCxAtClientCmdGetRspParamLine_withPartialBinaryHeaderTimeout_expectNull(void)
{
    uint8_t binaryBuf[6] = {0};
    uint16_t binaryLen = sizeof(binaryBuf);
    uint8_t rxData[] = { '+','F','O','O',':',0x01,0x00};

    uCxAtClientCmdBeginF(&gClient, "", "", U_CX_AT_UTIL_PARAM_LAST);
    gPRxDataPtr = rxData;
    gRxDataLen = sizeof(rxData);
    uPortGetTickTimeMs_StopIgnore();
    uPortGetTickTimeMs_ExpectAndReturn(20000);

    TEST_ASSERT_NULL(uCxAtClientCmdGetRspParamLine(&gClient, "+FOO:", binaryBuf, &binaryLen));
    TEST_ASSERT_TRUE(gClient.isBinaryRx);
    TEST_ASSERT_EQUAL(1, gClient.binaryRx.rxHeaderCount);
}

void test_uCxAtClientCmdGetRspParamLine_withBinaryPayloadReadError_expectIoError(void)
{
    uint8_t binaryBuf[6] = {0};
    uint16_t binaryLen = sizeof(binaryBuf);
    uint8_t rxData[] = { '+','F','O','O',':',BIN_HDR(6),0x00,0x11,0x22,0x33,0x44,0x55};

    uCxAtClientCmdBeginF(&gClient, "", "", U_CX_AT_UTIL_PARAM_LAST);
    gPRxDataPtr = rxData;
    gRxDataLen = sizeof(rxData);
    gRxIoErrorCode = -1234;
    gRxBytesUntilError = 10;

    TEST_ASSERT_NULL(uCxAtClientCmdGetRspParamLine(&gClient, "+FOO:", binaryBuf, &binaryLen));
    TEST_ASSERT_EQUAL(U_CX_ERROR_IO, gClient.status);
    TEST_ASSERT_EQUAL(-1234, uCxAtClientGetLastIoError(&gClient));
    TEST_ASSERT_EQUAL(4, gClient.binaryRx.remainingDataBytes);
}

void test_uCxAtClientCmdGetRspParamLine_withBinaryPayloadTimeout_expectNull(void)
{
    uint8_t binaryBuf[6] = {0};
    uint16_t binaryLen = sizeof(binaryBuf);
    uint8_t rxData[] = { '+','F','O','O',':',BIN_HDR(6),0x00,0x11};

    uCxAtClientCmdBeginF(&gClient, "", "", U_CX_AT_UTIL_PARAM_LAST);
    gPRxDataPtr = rxData;
    gRxDataLen = sizeof(rxData);
    uPortGetTickTimeMs_StopIgnore();
    uPortGetTickTimeMs_ExpectAndReturn(20000);

    TEST_ASSERT_NULL(uCxAtClientCmdGetRspParamLine(&gClient, "+FOO:", binaryBuf, &binaryLen));
    TEST_ASSERT_TRUE(gClient.isBinaryRx);
    TEST_ASSERT_EQUAL(4, gClient.binaryRx.remainingDataBytes);
}

void test_uCxAtClientCmdGetRspParamLine_withSmallBinaryBuffer_expectTruncatedResponse(void)
{
    uint8_t binaryBuf[3] = {0};
    uint16_t binaryLen = sizeof(binaryBuf);
    uint8_t rxData[] = { '+','F','O','O',':',BIN_HDR(6),0x00,0x11,0x22,0x33,0x44,0x55};
    uint8_t expectedBinData[] = {0x00,0x11,0x22};

    uCxAtClientCmdBeginF(&gClient, "", "", U_CX_AT_UTIL_PARAM_LAST);
    gPRxDataPtr = rxData;
    gRxDataLen = sizeof(rxData);
    gRxMaxReadSize = 1;

    char *pRsp = uCxAtClientCmdGetRspParamLine(&gClient, "+FOO:", binaryBuf, &binaryLen);

    TEST_ASSERT_NOT_NULL(pRsp);
    TEST_ASSERT_EQUAL(sizeof(binaryBuf), binaryLen);
    TEST_ASSERT_EQUAL_MEMORY(expectedBinData, binaryBuf, sizeof(expectedBinData));
    TEST_ASSERT_EQUAL(0, gRxDataLen);
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
    gClient.urcQueue.bufferLen = sizeof(uUrcEntry_t) + strlen(TEST_URC) + 1 + 6;

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
    gClient.urcQueue.bufferLen = sizeof(uUrcEntry_t) + strlen(TEST_URC) + 1 + 5;

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

    gPRxDataPtr = (uint8_t *)nextUrc;
    gRxDataLen = strlen(nextUrc);
    uCxAtClientHandleRx(&gClient);
    TEST_ASSERT_EQUAL(1, callbackCount);
}

void test_uCxAtClientCmd_withInterleavedUrc_expectResponseAndCallback(void)
{
    char rxData[] = { TEST_URC "\r\n+MYRSP:123\r\nOK\r\n" };
    int callbackCount = 0;
    gPRxDataPtr = (uint8_t *)&rxData[0];
    gRxDataLen = strlen(rxData);

    void urcCallback(struct uCxAtClient *pClient, void *pTag, char *pLine,
                     size_t lineLength, uint8_t *pBinaryData, size_t binaryDataLen)
    {
        TEST_ASSERT_EQUAL(&gClient, pClient);
        TEST_ASSERT_EQUAL(CONTEXT_VALUE, pTag);
        TEST_ASSERT_EQUAL_STRING(TEST_URC, pLine);
        TEST_ASSERT_EQUAL(strlen(TEST_URC), lineLength);
        TEST_ASSERT_NULL(pBinaryData);
        TEST_ASSERT_EQUAL(0, binaryDataLen);
        callbackCount++;
    }

    uCxAtClientSetUrcCallback(&gClient, urcCallback, CONTEXT_VALUE);
    uCxAtClientCmdBeginF(&gClient, "", "", U_CX_AT_UTIL_PARAM_LAST);
    TEST_ASSERT_EQUAL_STRING("123",
                             uCxAtClientCmdGetRspParamLine(&gClient, "+MYRSP:", NULL, NULL));
    TEST_ASSERT_EQUAL(0, uCxAtClientCmdEnd(&gClient));
    TEST_ASSERT_EQUAL(1, callbackCount);
}

void test_uCxAtClientHandleRx_withReadError_expectIoError(void)
{
    gRxIoErrorCode = -1234;

    TEST_ASSERT_EQUAL(-1234, uCxAtClientHandleRx(&gClient));
    TEST_ASSERT_EQUAL(-1234, uCxAtClientGetLastIoError(&gClient));
}

void test_uCxAtClientSetCommandTimeout_withNonPermanentTimeout(void)
{
    gRxDataLen = 0;
    int32_t newTimeout = U_CX_DEFAULT_CMD_TIMEOUT_MS + 5000;

    uPortGetTickTimeMs_StopIgnore();
    uPortGetTickTimeMs_StubWithCallback(uPortGetTickTimeMs_CALLBACK);

    // Set a new timeout B that is longer than default timeout A
    // Make sure it doesn't timeout between A and B, only after B
    int32_t ret = uCxAtClientSetCommandTimeout(&gClient, newTimeout, false);
    TEST_ASSERT_EQUAL(U_CX_DEFAULT_CMD_TIMEOUT_MS, ret);
    gPTickSequence = (int32_t []) {
        0, U_CX_DEFAULT_CMD_TIMEOUT_MS + 10, newTimeout + 10, -1
    };
    TEST_ASSERT_EQUAL(U_CX_ERROR_CMD_TIMEOUT, uCxAtClientExecSimpleCmdF(&gClient, "DUMMY", ""));
    TEST_ASSERT_EQUAL_MESSAGE(-1, *gPTickSequence, "Timed out too early");

    // Now the timeout should be back at default timeout again, so make sure it is
    gPTickSequence = (int32_t []) {
        0, U_CX_DEFAULT_CMD_TIMEOUT_MS + 10, -1
    };
    TEST_ASSERT_EQUAL(U_CX_ERROR_CMD_TIMEOUT, uCxAtClientExecSimpleCmdF(&gClient, "DUMMY", ""));
    TEST_ASSERT_EQUAL_MESSAGE(-1, *gPTickSequence, "Timed out too early");
}

void test_uCxAtClientSetCommandTimeout_withPermanentTimeout(void)
{
    gRxDataLen = 0;
    int32_t newTimeout = U_CX_DEFAULT_CMD_TIMEOUT_MS + 5000;

    uPortGetTickTimeMs_StopIgnore();
    uPortGetTickTimeMs_StubWithCallback(uPortGetTickTimeMs_CALLBACK);

    // Set a new timeout B that is longer than default timeout A
    // Make sure it doesn't timeout between A and B, only after B
    int32_t ret = uCxAtClientSetCommandTimeout(&gClient, newTimeout, true);
    TEST_ASSERT_EQUAL(U_CX_DEFAULT_CMD_TIMEOUT_MS, ret);
    gPTickSequence = (int32_t []) {
        0, U_CX_DEFAULT_CMD_TIMEOUT_MS + 10, newTimeout + 10, -1
    };
    TEST_ASSERT_EQUAL(U_CX_ERROR_CMD_TIMEOUT, uCxAtClientExecSimpleCmdF(&gClient, "DUMMY", ""));
    TEST_ASSERT_EQUAL_MESSAGE(-1, *gPTickSequence, "Timed out too early");

    // Since we did set the timeout with permanent=true the new timeout
    // should still be set.
    gPTickSequence = (int32_t []) {
        0, U_CX_DEFAULT_CMD_TIMEOUT_MS + 10, newTimeout + 10, -1
    };
    TEST_ASSERT_EQUAL(U_CX_ERROR_CMD_TIMEOUT, uCxAtClientExecSimpleCmdF(&gClient, "DUMMY", ""));
    TEST_ASSERT_EQUAL_MESSAGE(-1, *gPTickSequence, "Timed out too early");
}
