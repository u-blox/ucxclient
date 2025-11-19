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

/** @file
 * @brief u-connectXpress AT client
 */

#ifndef U_CX_AT_CLIENT_H
#define U_CX_AT_CLIENT_H

#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stddef.h>

#include "u_cx_at_util.h"
#include "u_cx_at_params.h"
#include "u_cx_at_urc_queue.h"

/* ----------------------------------------------------------------
 * COMPILE-TIME MACROS
 * -------------------------------------------------------------- */

/* ----------------------------------------------------------------
 * TYPES
 * -------------------------------------------------------------- */

struct uCxAtClient;

typedef void (*uUrcCallback_t)(struct uCxAtClient *pClient, void *pTag, char *pLine,
                               size_t lineLength, uint8_t *pBinaryData, size_t binaryDataLen);

typedef enum {
    U_CX_BIN_STATE_BINARY_FLUSH,
    U_CX_BIN_STATE_BINARY_RSP,
    U_CX_BIN_STATE_BINARY_URC,
} uCxAtBinaryState_t;

typedef struct {

    uCxAtBinaryState_t state;
    uint8_t rxHeaderCount;
    uint16_t remainingDataBytes;
    uint8_t *pBuffer;
    uint16_t bufferSize;
    uint16_t bufferPos;
} uCxAtBinaryRx_t;

typedef struct {
    uint8_t *pBuffer;
    uint16_t *pBufferLength;
} uCxAtBinaryResponseBuf_t;

typedef struct uCxAtClient {
    const struct uCxAtClientConfig *pConfig;
    size_t rxBufferPos;
    size_t urcBufferPos;
    volatile bool executingCmd;
    int32_t cmdStartTime;
    int32_t cmdTimeout;
    int32_t cmdTimeoutLastPerm;
    const char *pExpectedRsp;
    size_t pExpectedRspLen;
    char *pRspParams;
    int32_t status;
    int32_t lastIoError;
    uUrcCallback_t urcCallback;
    void *pUrcCallbackTag;
#if U_CX_USE_URC_QUEUE == 1
    uCxAtUrcQueue_t urcQueue;
#endif
    bool isBinaryRx;
    uCxAtBinaryRx_t binaryRx;
    uCxAtBinaryResponseBuf_t rspBinaryBuf;
    U_CX_MUTEX_HANDLE cmdMutex;
    int32_t instance;
} uCxAtClient_t;

typedef struct uCxAtClientConfig {
    void *pRxBuffer;        /**< Pointer to a buffer that the client will use as RX buffer */
    size_t rxBufferLen;     /**< Size of the RX buffer. */
#if U_CX_USE_URC_QUEUE == 1
    void *pUrcBuffer;       /**< Pointer to a separate URC buffer */
    size_t urcBufferLen;    /**< Size of the URC buffer. */
#endif
    void *pStreamHandle;    /**< User pointer associated with the AT interface.
                                 This pointer will be passed to write and read functions below
                                 and can be used to talk to a certain COM port etc.*/

    /* Callback for writing to the AT interface (typically a UART)
     * The function should return the number of actual bytes written or negative number on error.
     */
    int32_t (*write)(uCxAtClient_t *pClient, void *pStreamHandle, const void *pData, size_t length);

    /* Callback for reading from the AT interface (typically a UART)
     * The function should return the number of actual bytes read or negative number on error.
     */
    int32_t (*read)(uCxAtClient_t *pClient, void *pStreamHandle, void *pData, size_t length,
                    int32_t timeoutMs);
    int32_t timeoutMs;
    void *pContext;
} uCxAtClientConfig_t;

/* ----------------------------------------------------------------
 * VARIABLES
 * -------------------------------------------------------------- */

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -------------------------------------------------------------- */

/**
  * @brief  AT client init
  *
  * This function must be called before any other uCxAtClientXxx function is called.
  *
  * @param[in]  pConfig:  the AT client configuration (see \ref uCxAtClientConfig_t)
  * @param[out] pClient:  a pointer to an AT client struct that will be initialized.
  */
void uCxAtClientInit(const uCxAtClientConfig_t *pConfig, uCxAtClient_t *pClient);

/**
  * @brief  AT client deinit
  *
  * Calling this will release any resources allocated by uCxAtClientInit().
  *
  * @param[in]  pClient:      the AT client from uCxAtClientInit().
  *
  */
void uCxAtClientDeinit(uCxAtClient_t *pClient);

/**
  * @brief  Set URC callback
  *
  * @param[in]  pClient:      the AT client from uCxAtClientInit().
  * @param[in]  urcCallback:  the callback to be called when a URC line is received.
  *                           Can be set to NULL for disabling the callback.
  * @param[in]  pTag          a user pointer that will be passed to the callback when called.
  */
void uCxAtClientSetUrcCallback(uCxAtClient_t *pClient, uUrcCallback_t urcCallback, void *pTag);

/**
  * @brief  Execute an AT command without any response
  *
  * @param[in]  pClient:  the AT client from uCxAtClientInit().
  * @param[in]  pCmd:     the AT command to execute.
  * @retval               0 on status OK, -1 on status ERROR, negative value on error.
  */
int32_t uCxAtClientExecSimpleCmd(uCxAtClient_t *pClient, const char *pCmd);

/**
  * @brief  Execute an AT command with params but without any response
  *
  * This function uses a param format string (somewhat similar to printf).
  * Defined format characters are:
  *
  * Char   Type                 Desc
  * ---------------------------------------------
  * 'd'    int32_t              Integer
  * 'l'    int16_t *, size_t    Integer list
  *                             Note: Takes two args:
  *                             - int16_t *pValues
  *                             - size_t length
  * 's'    const char *         Null terminated string
  * '$'    const char *, size_t Binary string
  *                             Note: Takes two args:
  *                             - const char *pString
  *                             - size_t stringLength
  * 'i'    uSockIpAddress_t *   IP address
  * 'm'    uMacAddress_t *      MAC address
  * 'b'    uBtLeAddress_t *     Bluetooth address
  * 'h'    uint8_t *, int32_t   Binary data serialized as a hex string.
  *                             Note: Takes two args:
  *                             - uint8_t *pData
  *                             - int32_t dataLength
  * 'B'    uint8_t *, int32_t   Binary data using binary transfer.
  *                             Note: Takes two args:
  *                             - uint8_t *pData
  *                             - int32_t dataLength
  *
  * Each AT parameter is then added as an variadic argument to this function.
  * NOTE: The variadic parameters must always be terminated with U_CX_AT_UTIL_PARAM_LAST
  *
  * Example:
  * uCxAtClientExecSimpleCmdF(pClient,"AT+FOO=","ds",123,"MyString",U_CX_AT_UTIL_PARAM_LAST)
  * Result:
  * "AT+FOO=123,MyString\r"
  *
  * @param[in]  pClient:   the AT client from uCxAtClientInit().
  * @param[in]  pCmd:      the AT command to execute.
  * @param[in]  pParamFmt: format string - see function description.
  * @param      ...:       the AT params. Last param is always U_CX_AT_UTIL_PARAM_LAST!
  * @retval                0 on status OK, -1 on status ERROR, negative value on error.
  */
int32_t uCxAtClientExecSimpleCmdF(uCxAtClient_t *pClient, const char *pCmd, const char *pParamFmt,
                                  ...);

/**
  * @brief  Execute an AT command with params but without any response
  *
  * Same as uCxAtClientExecSimpleCmdF() but using a va_list for AT params instead of variadic arguments.
  *
  * @param[in]  pClient:   the AT client from uCxAtClientInit().
  * @param[in]  pCmd:      the AT command to execute.
  * @param[in]  pParamFmt: format string - see uCxAtClientExecSimpleCmdF().
  * @param      args:      the AT params. Last param is always U_CX_AT_UTIL_PARAM_LAST!
  * @retval                0 on status OK, -1 on status ERROR, negative value on error.
  */
void uCxAtClientSendCmdVaList(uCxAtClient_t *pClient, const char *pCmd, const char *pParamFmt,
                              va_list args);

/**
  * @brief  Begin an AT command with response
  *
  * The sequence of executing an AT command with response are:
  * 1. Call uCxAtClientCmdBeginF() - this will send the AT command
  * 2. Call either uCxAtClientCmdGetRspParamLine() or uCxAtClientCmdGetRspParamsF() for each response
  *    (you may call these functions several times if there are more than one response line)
  * 3. Call uCxAtClientCmdEnd() to wait for the AT status and end the AT command sequence.
  *
  * I.e. each call to uCxAtClientCmdBeginF() must always be terminated with a call to uCxAtClientCmdEnd().
  *
  * @param[in]  pClient:   the AT client from uCxAtClientInit().
  * @param[in]  pCmd:      the AT command to execute.
  * @param[in]  pParamFmt: format string - see uCxAtClientExecSimpleCmdF().
  * @param      args:      the AT params. Last param is always U_CX_AT_UTIL_PARAM_LAST!
  */
void uCxAtClientCmdBeginF(uCxAtClient_t *pClient, const char *pCmd, const char *pParamFmt, ...);

/**
  * @brief  Get AT response parameter line for AT command started with uCxAtClientCmdBeginF()
  *
  * @param[in]  pClient:            the AT client from uCxAtClientInit().
  * @param[in]  pExpectedRsp:       the expected AT response suffix. Typically an AT command AT+FOO will respond
  *                                 with something like '+FOO:123,"foo"'. In this case you set pExpectedRsp to '+FOO:'.
  *                                 For some AT commands (such as AT+GMM) there are no suffix. In this case you can
  *                                 set pExpectedRsp to either "" or NULL.
  * @param[in]  pBinaryBuf:         if the AT response is expected to respond with binary transfer then you must
  *                                 set this pointer to a buffer that can receive all that data.
  *                                 May be set to NULL if no binary transfer is expected.
  * @param[inout] pBinaryBufLength: a pointer to the length of the binary buffer. After transmission
  *                                 is completed the pointer will be updated with the amount of bytes received.
  *                                 May be set to NULL if no binary transfer is expected.
  * @retval                         a null terminated AT parameter line on success, otherwise NULL.
  *                                 NOTE: The pExpectedRsp part will not be included in this string.
  */
char *uCxAtClientCmdGetRspParamLine(uCxAtClient_t *pClient, const char *pExpectedRsp,
                                    uint8_t *pBinaryBuf, uint16_t *pBinaryBufLength);

/**
  * @brief  Get parsed AT response parameters for AT command started with uCxAtClientCmdBeginF()
  *
  * This function will read a response line like uCxAtClientCmdGetRspParamLine(), but will also try to
  * parse each AT parameter.
  *
  * @param[in]  pClient:            the AT client from uCxAtClientInit().
  * @param[in]  pExpectedRsp:       the expected AT response suffix. Typically an AT command AT+FOO will respond
  *                                 with something like '+FOO:123,"foo"'. In this case you set pExpectedRsp to '+FOO:'.
  *                                 For some AT commands (such as AT+GMM) there are no suffix. In this case you can
  *                                 set pExpectedRsp to either "" or NULL.
  * @param[in]  pBinaryBuf:         if the AT response is expected to respond with binary transfer then you must
  *                                 set this pointer to a buffer that can receive all that data.
  *                                 May be set to NULL if no binary transfer is expected.
  * @param[inout] pBinaryBufLength: a pointer to the length of the binary buffer. After transmission
  *                                 is completed the pointer will be updated with the amount of bytes received.
  *                                 May be set to NULL if no binary transfer is expected.
  * @param[in]  pParamFmt:          format string - see uCxAtClientExecSimpleCmdF().
  * @param      ...:                the AT params. Last param is always U_CX_AT_UTIL_PARAM_LAST!
  * @retval                         the number of parsed parameters on success otherwise negative value.
  */
int32_t uCxAtClientCmdGetRspParamsF(uCxAtClient_t *pClient, const char *pExpectedRsp,
                                    uint8_t *pBinaryBuf, uint16_t *pBinaryBufLength,
                                    const char *pParamFmt, ...);

/**
  * @brief  End AT command started with uCxAtClientCmdBeginF() and get AT status
  *
  * Each call to uCxAtClientCmdBeginF() must be terminated with a call to this function.
  * The function will wait for the AT status ("OK" or "ERROR").
  *
  * @param[in]  pClient:   the AT client from uCxAtClientInit().
  * @retval                0 on status OK, -1 on status ERROR, negative value on error.
  */
int32_t uCxAtClientCmdEnd(uCxAtClient_t *pClient);

/**
  * @brief  Handle AT RX data
  *
  * Call this function when data has been received by the AT interface (UART) while no
  * AT command is being executed.
  *
  * This is needed for handling URCs that can be sent by the AT server at any time.
  * If this function is called when AT client is currently handling an AT command it
  * will do nothing.
  *
  * @param[in]  pClient:   the AT client from uCxAtClientInit().
  */
void uCxAtClientHandleRx(uCxAtClient_t *pClient);

/**
  * @brief  Get last I/O error code
  *
  * If the AT client returns U_CX_ERROR_IO you can call this
  * function to get the underlying IO error code.
  *
  * @param[in]  pClient:   the AT client from uCxAtClientInit().
  * @retval                IO error code.
  */
int32_t uCxAtClientGetLastIoError(uCxAtClient_t *pClient);

/**
  * @brief  Set command timeout
  *
  *
  * @param[in]  pClient:   the AT client from uCxAtClientInit().
  * @param      timeoutMs: timeout in millisec.
  * @param      permanent: when set to true the timeout will be applied
  *                        permanently for all succeeding AT commands.
  *                        When set to false it will be used only for next
  *                        AT command. After that the timeout will be restored
  *                        to last permanent timeout (or default timeout).
  * @retval                the command timeout before call.
  */
int32_t uCxAtClientSetCommandTimeout(uCxAtClient_t *pClient, int32_t timeoutMs,
                                     bool permanent);

#endif // U_CX_AT_CLIENT_H
