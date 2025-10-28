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

/** @file
 * @brief u-connectXpress AT client
 */

#include <limits.h>  // For INT_MAX
#include <stddef.h>  // NULL, size_t etc.
#include <stdint.h>  // int32_t etc.
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>  // memcpy(), strcmp(), strcspn(), strspm()
#include <stdio.h>   // snprintf()
#include <ctype.h>   // isprint()

#include "u_cx_at_config.h"

#include "u_cx_log.h"
#include "u_cx_at_util.h"
#include "u_cx_at_client.h"

/* ----------------------------------------------------------------
 * COMPILE-TIME MACROS
 * -------------------------------------------------------------- */

#define NO_STATUS   (INT_MAX)

/* Special character sent for entering binary mode */
#define U_CX_SOH_CHAR    0x01

#define CHECK_READ_ERROR(CLIENT, READ_RET)  \
    if (READ_RET < 0) {                     \
        CLIENT->lastIoError = READ_RET;     \
        CLIENT->status = U_CX_ERROR_IO;     \
        U_CX_LOG_LINE_I(U_CX_LOG_CH_WARN, CLIENT->instance, \
                        "read() failed with return value: %d", READ_RET); \
        return AT_PARSER_ERROR;             \
    }

/* ----------------------------------------------------------------
 * TYPES
 * -------------------------------------------------------------- */

enum uCxAtParserCode {
    AT_PARSER_NOP = 0,
    AT_PARSER_GOT_STATUS,
    AT_PARSER_GOT_RSP,
    AT_PARSER_GOT_URC,
    AT_PARSER_START_BINARY,
    AT_PARSER_ERROR
};

/* ----------------------------------------------------------------
 * STATIC PROTOTYPES
 * -------------------------------------------------------------- */

/* ----------------------------------------------------------------
 * STATIC VARIABLES
 * -------------------------------------------------------------- */

static int32_t gNextInstance = 0;

/* ----------------------------------------------------------------
 * STATIC FUNCTIONS
 * -------------------------------------------------------------- */

// Helper function for setting up the RX binary transfer buffer
static void setupBinaryRxBuffer(uCxAtClient_t *pClient, uCxAtBinaryState_t state,
                                uint8_t *pBuffer, size_t bufferSize, uint16_t remainingBytes)
{
    uCxAtBinaryRx_t *pBinRx = &pClient->binaryRx;
    pBinRx->bufferPos = 0;
    pBinRx->pBuffer = pBuffer;
    pBinRx->bufferSize = bufferSize;
    pBinRx->state = state;
    pBinRx->remainingDataBytes = remainingBytes;
}

static int32_t parseLine(uCxAtClient_t *pClient, char *pLine, size_t lineLength)
{
    int32_t ret = AT_PARSER_NOP;

    char *pPtr = pLine;
    bool emptyLine = true;
    while (pPtr < &pLine[lineLength]) {
        if ((*pPtr != '\n') && (*pPtr != '\r')) {
            emptyLine = false;
            break;
        }
        pPtr++;
    }
    if (emptyLine) {
        return AT_PARSER_NOP;
    }

    U_CX_LOG_LINE_I(U_CX_LOG_CH_RX, pClient->instance, "%s", pLine);

    if (pClient->executingCmd) {
        if ((pClient->pExpectedRsp != NULL) &&
            (*pClient->pExpectedRsp != 0) &&
            (strncmp(pLine, pClient->pExpectedRsp, pClient->pExpectedRspLen) == 0)) {
            pClient->pRspParams = &pLine[pClient->pExpectedRspLen];
            ret = AT_PARSER_GOT_RSP;
        } else if (strcmp(pLine, "OK") == 0) {
            pClient->status = 0;
            ret = AT_PARSER_GOT_STATUS;
        } else if (strncmp(pLine, "ERROR", 5) == 0) {
            if (pLine[5] == 0) {
                pClient->status = U_CX_ERROR_STATUS_ERROR;
                U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pClient->instance, "Command failed");
                ret = AT_PARSER_GOT_STATUS;
            } else if (pLine[5] == ':') {
                // Extended error code
                char *pEnd;
                char *pCodeStr = &pLine[6];
                int code = strtol(pCodeStr, &pEnd, 10);
                if (isdigit((int) * pCodeStr) && (*pEnd == 0)) {
                    pClient->status = U_CX_EXTENDED_ERROR_OFFSET - code;
                    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pClient->instance, "Command failed with error code: %d", code);
                    ret = AT_PARSER_GOT_STATUS;
                }
            }
        } else if ((pLine[0] != '+') && (pLine[0] != '*') && (strncmp(pLine, "AT", 2) != 0)) {
            pClient->pRspParams = &pLine[0];
            ret = AT_PARSER_GOT_RSP;
        }
    }

    if (ret == AT_PARSER_NOP) {
        // Check if this is URC data
        if ((pLine[0] == '+') || (pLine[0] == '*')) {
#if U_CX_USE_URC_QUEUE == 1
            if (uCxAtUrcQueueEnqueueBegin(&pClient->urcQueue, pLine, lineLength)) {
                ret = AT_PARSER_GOT_URC;
            } else {
                // Urc queue full
                U_CX_LOG_LINE_I(U_CX_LOG_CH_WARN, pClient->instance, "URC queue full - dropping URC");
            }
#else
            const struct uCxAtClientConfig *pConfig = pClient->pConfig;
            if (pClient->urcCallback) {
                pClient->urcCallback(pClient, pClient->pUrcCallbackTag, pConfig->pRxBuffer,
                                     pClient->rxBufferPos, NULL, 0);
            }
#endif
        } else {
            // Received unexpected data
            // TODO: Handle
            U_CX_LOG_LINE_I(U_CX_LOG_CH_WARN, pClient->instance, "Unexpected data");
        }
    }

    return ret;
}

static int32_t parseIncomingChar(uCxAtClient_t *pClient, char ch)
{
    int32_t ret = AT_PARSER_NOP;
    char *pRxBuffer = (char *)pClient->pConfig->pRxBuffer;

    if (ch == U_CX_SOH_CHAR) {
        pRxBuffer[pClient->rxBufferPos] = 0;
        ret = AT_PARSER_START_BINARY;
    } else if ((ch == '\r') || (ch == '\n')) {
        pRxBuffer[pClient->rxBufferPos] = 0;
        ret = parseLine(pClient, pRxBuffer, pClient->rxBufferPos);
        pClient->rxBufferPos = 0;
#if U_CX_USE_URC_QUEUE == 1
        if (ret == AT_PARSER_GOT_URC) {
            // We got URC in character mode so no binary transfer is needed
            // hence we can complete the URC enqueueing
            uCxAtUrcQueueEnqueueEnd(&pClient->urcQueue, 0);
            // Make sure we continue calling parseIncomingChar() as the
            // URC will be handled after the command has completed
            ret = AT_PARSER_NOP;
        }
#endif
    } else if (isprint(ch)) {
        pRxBuffer[pClient->rxBufferPos++] = ch;
        if (pClient->rxBufferPos == pClient->pConfig->rxBufferLen) {
            // Overflow - discard everything and start over
            pClient->rxBufferPos = 0;
        }
    }

    return ret;
}

static void setupBinaryTransfer(uCxAtClient_t *pClient, int32_t parserRet, uint16_t binLength)
{
    const struct uCxAtClientConfig *pConfig = pClient->pConfig;

    U_CX_LOG_LINE_I(U_CX_LOG_CH_RX, pClient->instance, "[%d bytes]", binLength);
    switch (parserRet) {
        case AT_PARSER_GOT_RSP: {
            // We are receiving an AT response with binary data
            // Setup the binary buffer, but don't return AT_PARSER_GOT_RSP
            // until transfer is done.
            uCxAtBinaryResponseBuf_t *pRspBuf = &pClient->rspBinaryBuf;
            size_t length = 0;
            if (pRspBuf->pBufferLength) {
                length = *pRspBuf->pBufferLength;
                *pRspBuf->pBufferLength = 0;
            }
            setupBinaryRxBuffer(pClient, U_CX_BIN_STATE_BINARY_RSP,
                                pRspBuf->pBuffer, length, binLength);
            break;
        }
        case AT_PARSER_GOT_URC: {
#if U_CX_USE_URC_QUEUE == 1
            // Place the binary data directly after the URC string
            uint8_t *pPtr = pConfig->pUrcBuffer;
            size_t len = uCxAtUrcQueueEnqueueGetPayloadPtr(&pClient->urcQueue, &pPtr);
            if (len > binLength) {
                setupBinaryRxBuffer(pClient, U_CX_BIN_STATE_BINARY_URC, pPtr, len, binLength);
            } else {
                // The binary data can't be fitted into the queue so we need to drop it
                U_CX_LOG_LINE_I(U_CX_LOG_CH_WARN, pClient->instance, "Not enough space for URC binary data");
                uCxAtUrcQueueEnqueueAbort(&pClient->urcQueue);
                setupBinaryRxBuffer(pClient, U_CX_BIN_STATE_BINARY_FLUSH, NULL, 0, binLength);
            }
#else
            size_t bufPos = pClient->rxBufferPos;
            uint8_t *pPtr = pConfig->pRxBuffer;
            size_t len = pConfig->rxBufferLen - bufPos;
            if (len > binLength) {
                setupBinaryRxBuffer(pClient, U_CX_BIN_STATE_BINARY_URC,
                                    &pPtr[bufPos], len, binLength);
            } else {
                // The binary data can't be fitted into the queue so we need to drop it
                U_CX_LOG_LINE_I(U_CX_LOG_CH_WARN, pClient->instance,  "Not enough space for URC binary data");
                setupBinaryRxBuffer(pClient, U_CX_BIN_STATE_BINARY_FLUSH, NULL, 0, binLength);
            }
#endif
            break;
        }
        default:
            // Unexpected data - just flush it
            U_CX_LOG_LINE_I(U_CX_LOG_CH_WARN, pClient->instance, "Unexpected binary data");
            setupBinaryRxBuffer(pClient, U_CX_BIN_STATE_BINARY_FLUSH, NULL, 0, binLength);
            break;
    }
}

static int32_t handleBinaryRx(uCxAtClient_t *pClient)
{
    int32_t ret = AT_PARSER_NOP;

    // Loop for receiving binary data
    uCxAtBinaryRx_t *pBinRx = &pClient->binaryRx;
    int32_t readStatus;

    static uint8_t lengthBuf[2];
    if (pBinRx->rxHeaderCount < 2) {
        size_t readLen = sizeof(lengthBuf) - pBinRx->rxHeaderCount;
        readStatus = pClient->pConfig->read(pClient, pClient->pConfig->pStreamHandle,
                                            &lengthBuf[pBinRx->rxHeaderCount], readLen,
                                            pClient->pConfig->timeoutMs);
        CHECK_READ_ERROR(pClient, readStatus);
        if (readStatus > 0) {
            pBinRx->rxHeaderCount += readStatus;
        }
        if (readStatus < (int32_t)readLen) {
            return ret;
        } else {
            // The two length bytes have now been received
            int32_t parse_code;
            uint16_t length = (uint16_t)(lengthBuf[0] << 8) | lengthBuf[1];
            char *pRxBuffer = (char *)pClient->pConfig->pRxBuffer;
            parse_code = parseLine(pClient, pRxBuffer, pClient->rxBufferPos);
            setupBinaryTransfer(pClient, parse_code, length);
            pClient->rxBufferPos = 0;
        }
    }

    while (pBinRx->remainingDataBytes > 0) {
        size_t remainingBuf = pBinRx->bufferSize - pBinRx->bufferPos;
        if (remainingBuf > 0) {
            // There are buffer left, continue to read
            size_t readLen = U_MIN(remainingBuf, pBinRx->remainingDataBytes);
            readStatus = pClient->pConfig->read(pClient, pClient->pConfig->pStreamHandle,
                                                &pBinRx->pBuffer[pBinRx->bufferPos], readLen,
                                                pClient->pConfig->timeoutMs);
            CHECK_READ_ERROR(pClient, readStatus);
            if (readStatus > 0) {
                pBinRx->bufferPos += readStatus;
            }
        } else {
            // There are no buffer space - just throw away all data until binary transfer is done
            uint8_t buf[64];
            size_t readLen = U_MIN(sizeof(buf), pBinRx->remainingDataBytes);
            readStatus = pClient->pConfig->read(pClient, pClient->pConfig->pStreamHandle,
                                                &buf[0], readLen,
                                                pClient->pConfig->timeoutMs);
            CHECK_READ_ERROR(pClient, readStatus);
        }

        if (readStatus > 0) {
            pBinRx->remainingDataBytes -= readStatus;
        } else {
            break;
        }
    }

    if (pBinRx->remainingDataBytes == 0) {
        // Binary transfer done
        uCxAtBinaryState_t binState = pClient->binaryRx.state;
        pClient->binaryRx.state = U_CX_BIN_STATE_BINARY_FLUSH;
        pClient->binaryRx.rxHeaderCount = 0;
        pClient->isBinaryRx = false;

        switch (binState) {
            case U_CX_BIN_STATE_BINARY_RSP: {
                uCxAtBinaryResponseBuf_t *pRspBuf = &pClient->rspBinaryBuf;
                // Report back how much data were received
                if (pRspBuf->pBufferLength != NULL) {
                    *pRspBuf->pBufferLength = pClient->binaryRx.bufferPos;
                }
                ret = AT_PARSER_GOT_RSP;
                break;
            }
            case U_CX_BIN_STATE_BINARY_URC: {
#if U_CX_USE_URC_QUEUE == 1
                uCxAtUrcQueueEnqueueEnd(&pClient->urcQueue, pClient->binaryRx.bufferPos);
#else
                const struct uCxAtClientConfig *pConfig = pClient->pConfig;
                if (pClient->urcCallback) {
                    pClient->urcCallback(pClient, pClient->pUrcCallbackTag, pConfig->pRxBuffer,
                                         pClient->rxBufferPos, pClient->binaryRx.pBuffer,
                                         pClient->binaryRx.bufferPos);
                }
#endif
                break;
            }
            default:
                break;
        }
    }

    return ret;
}

static int32_t handleRxData(uCxAtClient_t *pClient)
{
    int32_t ret = AT_PARSER_NOP;

    do {
        int32_t readStatus;

        if (!pClient->isBinaryRx) {
            // Loop for receiving string data
            do {
                char ch;
                readStatus = pClient->pConfig->read(pClient, pClient->pConfig->pStreamHandle, &ch, 1,
                                                    pClient->pConfig->timeoutMs);
                CHECK_READ_ERROR(pClient, readStatus);
                if (readStatus != 1) {
                    break;
                }
                ret = parseIncomingChar(pClient, ch);
            } while (ret == AT_PARSER_NOP);
        } else {
            ret = handleBinaryRx(pClient);
        }

        if (ret == AT_PARSER_START_BINARY) {
            pClient->isBinaryRx = true;
        }
    } while (ret == AT_PARSER_START_BINARY);

    return ret;
}

#if U_CX_USE_URC_QUEUE == 1
static void processUrcs(uCxAtClient_t *pClient)
{
    while (true) {
        uUrcEntry_t *pEntry = uCxAtUrcQueueDequeueBegin(&pClient->urcQueue);
        if (pEntry == NULL) {
            break;
        }
        if (pClient->urcCallback) {
            char *pUrcLine = (char *)&pEntry->data[0];
            uint8_t *pPayload = NULL;
            if (pEntry->payloadSize > 0) {
                pPayload = &pEntry->data[pEntry->strLineLen + 1];
            }
            pClient->urcCallback(pClient, pClient->pUrcCallbackTag, pUrcLine,
                                 pEntry->strLineLen, pPayload, pEntry->payloadSize);
        }
        uCxAtUrcQueueDequeueEnd(&pClient->urcQueue, pEntry);
    }
}
#endif

static void cmdBeginF(uCxAtClient_t *pClient, const char *pCmd, const char *pParamFmt, va_list args)
{
    U_CX_MUTEX_LOCK(pClient->cmdMutex);

    // Check that previous command has completed
    // If this assert fails you have probably forgotten to call uCxAtClientCmdEnd()
    U_CX_AT_PORT_ASSERT(!pClient->executingCmd);

    pClient->pRspParams = NULL;
    pClient->executingCmd = true;
    pClient->status = NO_STATUS;
    pClient->cmdStartTime = U_CX_PORT_GET_TIME_MS();
    uCxAtClientSendCmdVaList(pClient, pCmd, pParamFmt, args);
}

static int32_t cmdEnd(uCxAtClient_t *pClient)
{
    while (pClient->status == NO_STATUS) {
        handleRxData(pClient);

        int32_t now = U_CX_PORT_GET_TIME_MS();
        if ((now - pClient->cmdStartTime) > pClient->cmdTimeout) {
            pClient->status = U_CX_ERROR_CMD_TIMEOUT;
            U_CX_LOG_LINE_I(U_CX_LOG_CH_WARN, pClient->instance, "Command timeout");
            break;
        }
    }

    // cmdEnd() must be preceeded by a cmdBeginF()
    U_CX_AT_PORT_ASSERT(pClient->executingCmd);

    // Restore command timeout to last permanent timeout
    pClient->cmdTimeout = pClient->cmdTimeoutLastPerm;

    pClient->executingCmd = false;

    U_CX_MUTEX_UNLOCK(pClient->cmdMutex);

#if U_CX_USE_URC_QUEUE == 1
    // We may have received URCs during command execution
    processUrcs(pClient);
#endif

    return pClient->status;
}

static inline int32_t writeAndLog(uCxAtClient_t *pClient, const void *pData, size_t dataLen)
{
    const struct uCxAtClientConfig *pConfig = pClient->pConfig;
    U_CX_LOG(U_CX_LOG_CH_TX, "%s", (char *)pData);
    return pConfig->write(pClient, pConfig->pStreamHandle, pData, dataLen);
}

static inline int32_t writeNoLog(uCxAtClient_t *pClient, const void *pData, size_t dataLen)
{
    const struct uCxAtClientConfig *pConfig = pClient->pConfig;
    return pConfig->write(pClient, pConfig->pStreamHandle, pData, dataLen);
}

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -------------------------------------------------------------- */

void uCxAtClientInit(const uCxAtClientConfig_t *pConfig, uCxAtClient_t *pClient)
{
    memset(pClient, 0, sizeof(uCxAtClient_t));
    pClient->pConfig = pConfig;
    pClient->cmdTimeoutLastPerm = U_CX_DEFAULT_CMD_TIMEOUT_MS;
    pClient->cmdTimeout = pClient->cmdTimeoutLastPerm;
    pClient->instance = gNextInstance++;

#if U_CX_USE_URC_QUEUE == 1
    uCxAtUrcQueueInit(&pClient->urcQueue, pConfig->pUrcBuffer, pConfig->urcBufferLen);
#endif
    U_CX_MUTEX_CREATE(pClient->cmdMutex);
}

void uCxAtClientDeinit(uCxAtClient_t *pClient)
{
#if U_CX_USE_URC_QUEUE == 1
    uCxAtUrcQueueDeInit(&pClient->urcQueue);
#endif
    U_CX_MUTEX_DELETE(pClient->cmdMutex);
}

void uCxAtClientSetUrcCallback(uCxAtClient_t *pClient, uUrcCallback_t urcCallback, void *pTag)
{
    pClient->urcCallback = urcCallback;
    pClient->pUrcCallbackTag = pTag;
}

void uCxAtClientSendCmdVaList(uCxAtClient_t *pClient, const char *pCmd, const char *pParamFmt,
                              va_list args)
{
    bool binaryTransfer = false;
    char buf[U_IP_STRING_MAX_LENGTH_BYTES];
    const struct uCxAtClientConfig *pConfig = pClient->pConfig;

    U_CX_LOG_BEGIN_I(U_CX_LOG_CH_TX, pClient->instance);

    writeAndLog(pClient, pCmd, strlen(pCmd));
    const char *pCh = pParamFmt;
    while (*pCh != 0) {
        if ((pCh != pParamFmt) && (*pCh != 'B')) { // Don't add ',' for Binary transfer
            writeAndLog(pClient, ",", 1);
        }

        memset(&buf, 0, sizeof(buf));
        switch (*pCh) {
            case 'd': {
                // Digit (integer)
                int i = va_arg(args, int);
                int32_t len = snprintf(buf, sizeof(buf), "%d", i);
                writeAndLog(pClient, buf, len);
            }
            break;
            case 's': {
                // String
                char *pStr = va_arg(args, char *);
                writeAndLog(pClient, "\"", 1);
                writeAndLog(pClient, pStr, strlen(pStr));
                writeAndLog(pClient, "\"", 1);
            }
            break;
            case 'i': {
                // IP address
                uSockIpAddress_t *pIpAddr = va_arg(args, uSockIpAddress_t *);
                int32_t len = uCxIpAddressToString(pIpAddr, buf, sizeof(buf));
                U_CX_AT_PORT_ASSERT(len > 0);
                writeAndLog(pClient, buf, len);
            }
            break;
            case 'm': {
                // MAC address
                uMacAddress_t *pMacAddr = va_arg(args, uMacAddress_t *);
                int32_t len = uCxMacAddressToString(pMacAddr, buf, sizeof(buf));
                U_CX_AT_PORT_ASSERT(len > 0);
                writeAndLog(pClient, buf, len);
            }
            break;
            case 'b': {
                // Bluetooth LE address
                uBtLeAddress_t *pBtLeAddr = va_arg(args, uBtLeAddress_t *);
                int32_t len = uCxBdAddressToString(pBtLeAddr, buf, sizeof(buf));
                U_CX_AT_PORT_ASSERT(len > 0);
                writeAndLog(pClient, buf, len);
            }
            break;
            case 'B': {
                // Binary data transfer
                uint8_t *pData = va_arg(args, uint8_t *);
                int32_t len = va_arg(args, int32_t);
                char binHeader[3] = {
                    U_CX_SOH_CHAR,
                    (uint8_t)(len >> 8),
                    (uint8_t)(len & 0xFF),
                };
                U_CX_AT_PORT_ASSERT(len > 0);
                writeNoLog(pClient, binHeader, sizeof(binHeader));
                writeNoLog(pClient, pData, len);
                U_CX_LOG(U_CX_LOG_CH_TX, "[%d bytes]", len);

                // Binary transfer must always be last param
                U_CX_AT_PORT_ASSERT(pCh[1] == 0);
                binaryTransfer = true;
            }
            break;
            case 'h': {
                // Binary data transferred as hex string
                uint8_t *pData = va_arg(args, uint8_t *);
                int32_t len = va_arg(args, int32_t);
                for (int32_t i = 0; i < len; i++) {
                    uCxAtUtilByteToHex(pData[i], buf);
                    writeAndLog(pClient, buf, 2);
                }
            }
            break;
        }
        pCh++;
    }

    if (!binaryTransfer) {
        pConfig->write(pClient, pConfig->pStreamHandle, "\r", 1);
    }
    U_CX_LOG_END(U_CX_LOG_CH_TX);
}

int32_t uCxAtClientExecSimpleCmdF(uCxAtClient_t *pClient, const char *pCmd, const char *pParamFmt,
                                  ...)
{
    va_list args;

    va_start(args, pParamFmt);
    cmdBeginF(pClient, pCmd, pParamFmt, args);
    va_end(args);

    return cmdEnd(pClient);
}

int32_t uCxAtClientExecSimpleCmd(uCxAtClient_t *pClient, const char *pCmd)
{
    return uCxAtClientExecSimpleCmdF(pClient, pCmd, "", NULL);
}

void uCxAtClientCmdBeginF(uCxAtClient_t *pClient, const char *pCmd, const char *pParamFmt, ...)
{
    va_list args;

    va_start(args, pParamFmt);
    cmdBeginF(pClient, pCmd, pParamFmt, args);
    va_end(args);
}

char *uCxAtClientCmdGetRspParamLine(uCxAtClient_t *pClient, const char *pExpectedRsp,
                                    uint8_t *pBinaryBuf, size_t *pBinaryBufLength)
{
    char *pRet = NULL;
    pClient->rspBinaryBuf.pBuffer = pBinaryBuf;
    pClient->rspBinaryBuf.pBufferLength = pBinaryBufLength;
    pClient->pRspParams = NULL;
    pClient->pExpectedRsp = pExpectedRsp;
    if (pExpectedRsp) {
        pClient->pExpectedRspLen = strlen(pExpectedRsp);
    } else {
        pClient->pExpectedRspLen = 0;
    }

    while (pClient->status == NO_STATUS) {
        if (handleRxData(pClient) == AT_PARSER_GOT_RSP) {
            pRet = pClient->pRspParams;
            break;
        }
        // Check for timeout
        int32_t now = U_CX_PORT_GET_TIME_MS();
        if ((now - pClient->cmdStartTime) > pClient->cmdTimeout) {
            U_CX_LOG_LINE_I(U_CX_LOG_CH_WARN, pClient->instance, "Command timeout");
            return NULL;
        }
    }

    return pRet;
}

int32_t uCxAtClientCmdGetRspParamsF(uCxAtClient_t *pClient, const char *pExpectedRsp,
                                    uint8_t *pBinaryBuf, size_t *pBinaryBufLength,
                                    const char *pParamFmt, ...)
{
    va_list args;
    char *pRspParams = uCxAtClientCmdGetRspParamLine(pClient, pExpectedRsp,
                                                     pBinaryBuf, pBinaryBufLength);
    if (pRspParams == NULL) {
        return U_CX_ERROR_CMD_TIMEOUT;
    }
    va_start(args, pParamFmt);
    int32_t ret = uCxAtUtilParseParamsVaList(pRspParams, pParamFmt, args);
    va_end(args);

    return ret;
}

int32_t uCxAtClientCmdEnd(uCxAtClient_t *pClient)
{
    return cmdEnd(pClient);
}

void uCxAtClientHandleRx(uCxAtClient_t *pClient)
{
    U_CX_MUTEX_LOCK(pClient->cmdMutex);

    if (!pClient->executingCmd) {
        handleRxData(pClient);
    }

    U_CX_MUTEX_UNLOCK(pClient->cmdMutex);

#if U_CX_USE_URC_QUEUE == 1
    processUrcs(pClient);
#endif
}

int32_t uCxAtClientGetLastIoError(uCxAtClient_t *pClient)
{
    return pClient->lastIoError;
}

int32_t uCxAtClientSetCommandTimeout(uCxAtClient_t *pClient, int32_t timeoutMs,
                                  bool permanent)
{
    int32_t ret = pClient->cmdTimeout;
    pClient->cmdTimeout = timeoutMs;
    if (permanent) {
        pClient->cmdTimeoutLastPerm = timeoutMs;
    }
    return ret;
}
