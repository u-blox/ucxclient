/** @file
 * @brief AT string utilities
 */

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

#include "u_cx_at_params.h"
#include "u_cx_at_util.h"

/* ----------------------------------------------------------------
 * COMPILE-TIME MACROS
 * -------------------------------------------------------------- */

/* ----------------------------------------------------------------
 * TYPES
 * -------------------------------------------------------------- */

/* ----------------------------------------------------------------
 * STATIC PROTOTYPES
 * -------------------------------------------------------------- */

/* ----------------------------------------------------------------
 * STATIC VARIABLES
 * -------------------------------------------------------------- */

/* ----------------------------------------------------------------
 * STATIC FUNCTIONS
 * -------------------------------------------------------------- */

static inline char nibbleToHex(uint8_t nibble)
{
    return (nibble < 10) ? '0' + nibble : 'A' + nibble - 10;
}


static inline int32_t hexToNibble(char hexChar)
{
    if ((hexChar >= '0') && (hexChar <= '9')) {
        return hexChar - '0';
    }
    if ((hexChar >= 'a') && (hexChar <= 'f')) {
        return hexChar - 'a' + 10;
    }
    if ((hexChar >= 'A') && (hexChar <= 'F')) {
        return hexChar - 'A' + 10;
    }
    return -1;
}

static bool binaryToHex(const uint8_t *pData, size_t dataLen, char *pBuf,
                        size_t bufSize, bool reverse)
{
    uint32_t i;

    U_CX_AT_PORT_ASSERT(pBuf != NULL);
    U_CX_AT_PORT_ASSERT(bufSize > 0);

    // Check that the hex string can be fitted into the buffer (don't forget null termination)
    if (bufSize < (2 * dataLen) + 1) {
        pBuf[0] = 0;
        return false;
    }
    pBuf[2 * dataLen] = 0;

    if (pData != NULL) {
        for (i = 0; i < dataLen; i++) {
            uint32_t dataIndex = reverse ? dataLen - i - 1 : i;
            pBuf[i * 2] = nibbleToHex(pData[dataIndex] >> 4);
            pBuf[(i * 2) + 1] = nibbleToHex(pData[dataIndex] & 0x0F);
        }
    } else {
        strncpy(pBuf, "(null)", bufSize);
    }

    return true;
}

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -------------------------------------------------------------- */

void uCxAtUtilByteToHex(uint8_t byte, char *pOutPtr)
{
    pOutPtr[0] = nibbleToHex(byte >> 4);
    pOutPtr[1] = nibbleToHex(byte & 0xF);
    pOutPtr[2] = 0;
}

int32_t uCxAtUtilHexToByte(const char *pHex, uint8_t *pOutByte)
{
    int32_t highNibble = hexToNibble(pHex[0]);
    int32_t lowNibble = hexToNibble(pHex[1]);
    if ((highNibble < 0) || (lowNibble < 0)) {
        return -1;
    }
    *pOutByte = (highNibble << 4) | lowNibble;
    return 0;
}

uint32_t uCxAtUtilHexToBinary(const char *pHexString, uint8_t *pBuf, size_t bufSize)
{
    uint32_t i = 0;
    uint32_t len;
    uint32_t toIndex = 0;

    len = strlen(pHexString);

    while ((toIndex < bufSize) && ((i + 1) < len)) {
        int32_t ret = uCxAtUtilHexToByte(&pHexString[i], &pBuf[toIndex]);
        if (ret < 0) {
            // Invalid byte array return converted size
            return toIndex;
        }
        toIndex++;
        i += 2;
    }

    return toIndex;
}

bool uCxAtUtilBinaryToHex(const uint8_t *pData, size_t dataLen, char *pBuf, size_t bufSize)
{
    return binaryToHex(pData, dataLen, pBuf, bufSize, false);
}

bool uCxAtUtilReverseBinaryToHex(const uint8_t *pData, size_t dataLen, char *pBuf, size_t bufSize)
{
    return binaryToHex(pData, dataLen, pBuf, bufSize, true);
}

char *uCxAtUtilFindParamEnd(char *pStr)
{
    bool insideString = false;
    bool escape = false;

    char *pIter = pStr;
    while (*pIter != 0) {
        if (escape) {
            escape = false;
        } else if (*pIter == '\\') {
            escape = true;
        } else if (insideString) {
            if (*pIter == '"') {
                insideString = false;
            }
        } else if (*pIter == '"') {
            insideString = true;
        } else if (*pIter == ',') {
            break;
        }
        pIter++;
    }

    if (insideString || escape) {
        return NULL;
    }

    return pIter;
}

int32_t uCxAtUtilParseParamsVaList(char *pParams, const char *pParamFmt, va_list args)
{
    const char *pFmtCh = pParamFmt;
    char *pParam = pParams;
    bool last = false;
    int32_t ret = 0;

    while (*pFmtCh != 0) {
        ret++;
        char *pParamEnd = uCxAtUtilFindParamEnd(pParam);
        if (pParamEnd == NULL) {
            return -ret;
        }
        if (*pParamEnd == 0) {
            last = true;
        } else {
            *pParamEnd = 0;
        }

        switch (*pFmtCh) {
            case '-':
                // Ignore
                break;
            case 'd': {
                char * pEnd;
                int32_t *pI = va_arg(args, int32_t *);
                U_CX_AT_PORT_ASSERT(pI != U_CX_AT_UTIL_PARAM_LAST);
                *pI = strtol(pParam, &pEnd, 10);
                if (((*pParam != '-') && !isdigit((int)*pParam)) || (*pEnd != 0)) {
                    // Not a valid integer
                    return -ret;
                }
            }
            break;
            case 's': {
                char **ppStr = va_arg(args, char **);
                U_CX_AT_PORT_ASSERT(ppStr != U_CX_AT_UTIL_PARAM_LAST);
                if (*pParam == '"') {
                    pParam++;
                    pParamEnd[-1] = 0;
                }
                *ppStr = pParam;
            }
            break;
            case 'i': {
                uSockIpAddress_t *pIpAddr = va_arg(args, uSockIpAddress_t *);
                U_CX_AT_PORT_ASSERT(pIpAddr != U_CX_AT_UTIL_PARAM_LAST);
                if (uCxStringToIpAddress(pParam, pIpAddr) < 0) {
                    return -ret;
                }
            }
            break;
            case 'm': {
                uMacAddress_t *pMacAddr = va_arg(args, uMacAddress_t *);
                U_CX_AT_PORT_ASSERT(pMacAddr != U_CX_AT_UTIL_PARAM_LAST);
                if (uCxStringToMacAddress(pParam, pMacAddr) < 0) {
                    return -ret;
                }
            }
            break;
            case 'b': {
                uBtLeAddress_t *pBtLeAddr = va_arg(args, uBtLeAddress_t *);
                U_CX_AT_PORT_ASSERT(pBtLeAddr != U_CX_AT_UTIL_PARAM_LAST);
                if (uCxStringToBdAddress(pParam, pBtLeAddr) < 0) {
                    return -ret;
                }
            }
            break;
            case 'h': {
                uByteArray_t *pByteArray = va_arg(args, uByteArray_t *);
                U_CX_AT_PORT_ASSERT(pByteArray != U_CX_AT_UTIL_PARAM_LAST);
                uint8_t *pBytes;
                size_t len = strlen(pParam);
                if ((len % 2) != 0) {
                    return -ret;
                }
                pByteArray->length = len / 2;
                pBytes = (uint8_t *)pParam;
                pByteArray->pData = pBytes;
                for (size_t i = 0; i < pByteArray->length; i++) {
                    if (uCxAtUtilHexToByte(&pParam[i * 2], pBytes) < 0) {
                        return -ret;
                    }
                    pBytes++;
                }
            }
            break;
        }
        if (last) {
            break;
        }
        pFmtCh++;
        pParam = &pParamEnd[1];
    }

    return ret;
}

int32_t uCxAtUtilParseParamsF(char *pParams, const char *pParamFmt, ...)
{
    va_list args;

    va_start(args, pParamFmt);
    int32_t ret = uCxAtUtilParseParamsVaList(pParams, pParamFmt, args);
    va_end(args);

    return ret;
}
