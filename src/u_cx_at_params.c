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
 * @brief u-connectXpress AT param parser
 */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <limits.h>

#include "u_cx_at_params.h"
#include "u_cx_at_util.h"

/* ----------------------------------------------------------------
 * COMPILE-TIME MACROS
 * -------------------------------------------------------------- */

#define IPV6_DIGIT_COUNT    8

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

// Determine whether the given IP address string is IPV4.
static bool addressStringIsIpv4(const char *pAddressString)
{
    // If it's got a dot in it, must be IPV4
    return (strchr(pAddressString, '.') != NULL);
}

// Convert an IPV4 address string "xxx.yyy.www.zzz" into
// a struct.
static bool ipv4StringToIpAddress(const char *pAddressString,
                                  uSockIpAddress_t *pIpAddress)
{
    char *pStrPtr = (char *)pAddressString;
    uint8_t digits[4];

    pIpAddress->type = U_SOCK_ADDRESS_TYPE_V4;
    pIpAddress->address.ipv4 = 0;

    for (int i = 0; i < 4; i++) {
        if (!isdigit((int) * pStrPtr)) {
            return false;
        }
        long value = strtol(pStrPtr, &pStrPtr, 10);
        if ((value < 0) || (value > 255)) {
            return false;
        }
        if (i < 3) {
            if (*pStrPtr != '.') {
                return false;
            }
        } else if (*pStrPtr != 0) {
            // After last digit there should be a null terminator
            return false;
        }
        digits[i] = (uint8_t)value;
        pStrPtr++;
    }

    pIpAddress->address.ipv4 = (((uint32_t) digits[0]) << 24) |
                               (((uint32_t) digits[1]) << 16) |
                               (((uint32_t) digits[2]) << 8)  |
                               (((uint32_t) digits[3]) << 0);
    return true;
}

// Convert an IPV6 address string "2001:0db8:85a3:0000:0000:8a2e:0370:7334"
// or "[2001:0db8:85a3:0000:0000:8a2e:0370:7334]" into a struct.
static bool ipv6StringToIpAddress(const char *pAddressString,
                                  uSockIpAddress_t *pIpAddress)
{
    char *pStrPtr = (char *)pAddressString;
    uint16_t digits[IPV6_DIGIT_COUNT];
    bool blockChar = false;

    pIpAddress->type = U_SOCK_ADDRESS_TYPE_V6;
    memset(pIpAddress->address.ipv6, 0,
           sizeof(pIpAddress->address.ipv6));

    // See if there's a '[' on the start
    if ((strlen(pStrPtr) > 0) && (*pStrPtr == '[')) {
        blockChar = true;
        pStrPtr++;
    }

    // Get the hex numbers from the IP address part
    for (size_t i = 0; i < IPV6_DIGIT_COUNT; i++) {
        char *pTmp;
        if (*pStrPtr == 0) {
            return false;
        }
        digits[i] = (uint16_t)strtol(pStrPtr, &pTmp, 16);
        // Each digit should be 4 max characters in hex
        if (((pTmp - pStrPtr) > 4) || (pTmp < pStrPtr)) {
            return false;
        }
        pStrPtr = pTmp;

        if (i < IPV6_DIGIT_COUNT - 1) {
            if (*pStrPtr != ':') {
                return false;
            }
            pStrPtr++;
        } else {
            // Last digit
            if (blockChar) {
                // If string started with '[' it must end with ']'
                if (*pStrPtr != ']') {
                    return false;
                }
                pStrPtr++;
            }
            // We should now be at the end of the string
            if (*pStrPtr != 0) {
                return false;
            }
        }
    }
    // Got enough digits, now slot the uint16_t's
    // into the array in network-byte order
    pIpAddress->address.ipv6[3] = (((uint32_t) digits[0]) << 16) | (digits[1]);
    pIpAddress->address.ipv6[2] = (((uint32_t) digits[2]) << 16) | (digits[3]);
    pIpAddress->address.ipv6[1] = (((uint32_t) digits[4]) << 16) | (digits[5]);
    pIpAddress->address.ipv6[0] = (((uint32_t) digits[6]) << 16) | (digits[7]);

    return true;
}

static int32_t findBdAddressType(const char *pStr,
                                 uBdAddressType_t *pBdAddressType)
{
    size_t length = strlen(pStr);

    switch (pStr[length - 1]) {
        case 'r':
            *pBdAddressType = U_BD_ADDRESS_TYPE_RANDOM;
            break;
        case 'p':
            *pBdAddressType = U_BD_ADDRESS_TYPE_PUBLIC;
            break;
        default:
            if (length == 12) {
                //the input address has only 12 hexadecimal characters without any suffix
                //default address type is public
                *pBdAddressType = U_BD_ADDRESS_TYPE_PUBLIC;
            } else {
                //the input address has a suffix other than 'p' or 'r'
                return -1;
            }
            break;
    }

    return 0;
}

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -------------------------------------------------------------- */

int32_t uCxStringToIpAddress(const char *pAddressString,
                             uSockIpAddress_t *pIpAddress)
{
    U_CX_AT_PORT_ASSERT(pAddressString != NULL);
    U_CX_AT_PORT_ASSERT(pIpAddress != NULL);

    if (addressStringIsIpv4(pAddressString)) {
        if (!ipv4StringToIpAddress(pAddressString, pIpAddress)) {
            return -1;
        }
    } else {
        if (!ipv6StringToIpAddress(pAddressString, pIpAddress)) {
            return -1;
        }
    }

    return 0;
}

int32_t uCxIpAddressToString(const uSockIpAddress_t *pIpAddress,
                             char *pBuffer,
                             size_t sizeBytes)
{
    U_CX_AT_PORT_ASSERT(pBuffer != NULL);
    U_CX_AT_PORT_ASSERT(pIpAddress != NULL);

    int32_t ret = -1;
    char *pWritePtr = pBuffer;
    char *pBufEnd = &pBuffer[sizeBytes];

    // Convert the address in network byte order (MSB first);
    switch (pIpAddress->type) {
        case U_SOCK_ADDRESS_TYPE_V4:
            ret = snprintf(pBuffer, sizeBytes, "%u.%u.%u.%u",
                           (unsigned int) ((pIpAddress->address.ipv4 >> 24) & 0xFF),
                           (unsigned int) ((pIpAddress->address.ipv4 >> 16) & 0xFF),
                           (unsigned int) ((pIpAddress->address.ipv4 >> 8)  & 0xFF),
                           (unsigned int) ((pIpAddress->address.ipv4 >> 0)  & 0xFF));
            if (ret >= (int32_t) sizeBytes) {
                return -1;
            }
            break;
        case U_SOCK_ADDRESS_TYPE_V6:
            ret = strlen("[0000:0000:1111:1111:2222:2222:3333:3333]");
            if (ret >= (int32_t) sizeBytes) {
                return -1;
            }
            *pWritePtr = '[';
            pWritePtr++;
            for (int i = 0; i < 4; i++) {
                if (i > 0) {
                    *pWritePtr = ':';
                    pWritePtr++;
                }
                int len = snprintf(pWritePtr, (size_t)(pBufEnd - pWritePtr),
                                   "%04x:%04x",
                                   (unsigned int) ((pIpAddress->address.ipv6[3 - i] >> 16) & 0xFFFF),
                                   (unsigned int) ((pIpAddress->address.ipv6[3 - i] >> 0)  & 0xFFFF));
                pWritePtr += len;
            }
            U_CX_AT_PORT_ASSERT(pBufEnd - pWritePtr >= 2);
            pWritePtr[0] = ']';
            pWritePtr[1] = 0;
            pWritePtr += 2;
            (void)pWritePtr; // Keep clang-analyzer happy
            break;
        default:
            // Invalid address type
            U_CX_AT_PORT_ASSERT(false);
            break;
    }

    return ret;
}

int32_t uCxStringToBdAddress(const char *pBtLeAddrString, uBtLeAddress_t *pBtLeAddr)
{
    int32_t ret = findBdAddressType(pBtLeAddrString, &pBtLeAddr->type);
    if (ret == 0) {
        uint32_t len = uCxAtUtilHexToBinary(pBtLeAddrString, pBtLeAddr->address, U_BD_ADDR_LEN);
        if (len != U_BD_ADDR_LEN) {
            ret = -1;
        }
    }
    return ret;
}

int32_t uCxBdAddressToString(const uBtLeAddress_t *pBtLeAddr, char *pBuffer, size_t sizeBytes)
{
    if (sizeBytes < U_BD_STRING_MAX_LENGTH_BYTES) {
        return -1;
    }
    if (!uCxAtUtilBinaryToHex(&pBtLeAddr->address[0], U_BD_ADDR_LEN, pBuffer, sizeBytes)) {
        return -1;
    }
    size_t pos = strlen(pBuffer);
    pBuffer[pos++] = (pBtLeAddr->type == U_BD_ADDRESS_TYPE_RANDOM) ? 'r' : 'p';
    pBuffer[pos] = 0;
    return (int32_t)pos;
}

int32_t uCxStringToMacAddress(const char *pMacString, uMacAddress_t *pMac)
{
    if (strlen(pMacString) != U_MAC_ADDR_LEN * 2) {
        return -1;
    }
    if (uCxAtUtilHexToBinary(pMacString, &pMac->address[0], U_MAC_ADDR_LEN) != U_MAC_ADDR_LEN) {
        return -1;
    }
    return 0;
}

int32_t uCxMacAddressToString(const uMacAddress_t *pMac, char *pBuffer, size_t sizeBytes)
{
    if (!uCxAtUtilBinaryToHex(&pMac->address[0], U_MAC_ADDR_LEN, pBuffer, sizeBytes)) {
        return -1;
    }
    return (int32_t)strlen(pBuffer);
}

int32_t uCxStringToIntList(const char *pIntListString, uIntList_t *pIntList)
{
    U_CX_AT_PORT_ASSERT(pIntListString != NULL);
    U_CX_AT_PORT_ASSERT(pIntList != NULL);

    const char *pStr = pIntListString;
    size_t len = strlen(pStr);

    // Check for empty list []
    if (len >= 2 && pStr[0] == '[' && pStr[1] == ']') {
        pIntList->length = 0;
        pIntList->pIntValues = NULL;
        return 0;
    }

    // Must start with '['
    if (len < 3 || pStr[0] != '[') {
        return -1;
    }

    // Count commas to determine list size
    size_t count = 1;
    for (size_t i = 1; i < len - 1; i++) {
        if (pStr[i] == ',') {
            count++;
        }
    }

    // Allocate space for values in the original string buffer
    // We'll reuse the string memory for storing the parsed integers
    pIntList->pIntValues = (int16_t *)pStr;
    pIntList->length = count;

    // Parse the integers
    char *pParse = (char *)&pStr[1]; // Skip '['
    for (size_t i = 0; i < count; i++) {
        char *pEnd;
        long value = strtol(pParse, &pEnd, 10);
        if (pParse == pEnd || value < INT16_MIN || value > INT16_MAX) {
            return -1;
        }
        pIntList->pIntValues[i] = (int16_t)value;
        pParse = pEnd;
        if (i < count - 1) {
            if (*pParse != ',') {
                return -1;
            }
            pParse++; // Skip comma
        }
    }

    // Must end with ']'
    if (*pParse != ']') {
        return -1;
    }

    return 0;
}

int32_t uCxIntListToString(const uIntList_t *pIntList, char *pBuffer, size_t sizeBytes)
{
    U_CX_AT_PORT_ASSERT(pIntList != NULL);
    U_CX_AT_PORT_ASSERT(pBuffer != NULL);

    if (sizeBytes < 3) {
        return -1;
    }

    if (pIntList->length == 0) {
        pBuffer[0] = '[';
        pBuffer[1] = ']';
        pBuffer[2] = '\0';
        return 2;
    }

    char *pWrite = pBuffer;
    char *pEnd = pBuffer + sizeBytes;

    // Start with '['
    *pWrite++ = '[';

    for (size_t i = 0; i < pIntList->length; i++) {
        if (i > 0) {
            if (pWrite >= pEnd - 1) {
                return -1;
            }
            *pWrite++ = ',';
        }
        int written = snprintf(pWrite, (size_t)(pEnd - pWrite), "%d", pIntList->pIntValues[i]);
        if (written < 0 || pWrite + written >= pEnd - 1) {
            return -1;
        }
        pWrite += written;
    }

    // End with ']'
    if (pWrite >= pEnd - 1) {
        return -1;
    }
    *pWrite++ = ']';
    *pWrite = '\0';

    return (int32_t)(pWrite - pBuffer);
}
