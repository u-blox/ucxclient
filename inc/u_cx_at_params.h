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
 * @brief uConnectXpress AT param parser
 */

#ifndef U_CX_AT_PARAMS_H
#define U_CX_AT_PARAMS_H

#include <stdint.h>
#include <stdbool.h>

#include "u_cx_at_config.h"

/* ----------------------------------------------------------------
 * COMPILE-TIME MACROS
 * -------------------------------------------------------------- */

#define U_IP_STRING_MAX_LENGTH_BYTES  (41 + 1)
#define U_MAC_STRING_MAX_LENGTH_BYTES (12 + 1)
#define U_BD_STRING_MAX_LENGTH_BYTES  (12 + 1 + 1)

#define U_MAC_ADDR_LEN  6
#define U_BD_ADDR_LEN   6

/* ----------------------------------------------------------------
 * TYPES
 * -------------------------------------------------------------- */

/* U_UCONNECT_GEN2 is set when compiling for ubxlib which already
 * defines these types.
 */
#ifndef U_UCONNECT_GEN2

typedef enum {
    U_SOCK_ADDRESS_TYPE_V4 = 0,
    U_SOCK_ADDRESS_TYPE_V6 = 6
} uSockIpAddressType_t;

typedef struct {
    uSockIpAddressType_t type;
    union {
        uint32_t ipv4;
        uint32_t ipv6[4];
    } address;
} uSockIpAddress_t;

#endif // U_UCONNECT_GEN2

typedef enum {
    U_BD_ADDRESS_TYPE_RANDOM,
    U_BD_ADDRESS_TYPE_PUBLIC,
    U_BD_ADDRESS_TYPE_UNKNOWN,
} uBdAddressType_t;

typedef struct {
    uint8_t address[U_BD_ADDR_LEN];
    uBdAddressType_t type;
} uBtLeAddress_t;

typedef struct {
    uint8_t address[U_MAC_ADDR_LEN];
} uMacAddress_t;

typedef struct {
    uint8_t *pData;
    size_t length;
} uByteArray_t;

/* ----------------------------------------------------------------
 * VARIABLES
 * -------------------------------------------------------------- */

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -------------------------------------------------------------- */

/** Convert an IP address string into a struct.
 *
 * @param pAddressString the null terminated string to convert.
 *                       The string can be a IPv4 and IPv6 address.
 * @param[out] pAddress  a pointer to a place to put the address.
 * @return               zero on success else negative error code.
 */
int32_t uCxStringToIpAddress(const char *pAddressString,
                             uSockIpAddress_t *pAddress);

/** Convert an IP address struct into a string.
 *
 * @param pIpAddress    a pointer to the IP address to convert.
 * @param[out] pBuffer  a buffer in which to place the string.
 *                      Allow U_IP_STRING_MAX_LENGTH_BYTES
 *                      for a full IPv6 address and terminator.
 * @param sizeBytes     the amount of memory pointed to by
 *                      pBuffer.
 * @return              on success the length of the string, not
 *                      including the terminator (i.e. what
 *                      strlen() would return) else negative
 *                      error code.
 */
int32_t uCxIpAddressToString(const uSockIpAddress_t *pIpAddress,
                             char *pBuffer, size_t sizeBytes);

/** Convert a Bluetooth Device address string into a struct.
 *
 * @param pBtLeAddrString   the null terminated string to convert.
 * @param[out] pBtLeAddr    a pointer to a place to put the address.
 * @return                  zero on success else negative error code.
 */
int32_t uCxStringToBdAddress(const char *pBtLeAddrString, uBtLeAddress_t *pBtLeAddr);

/** Convert a Bluetooth Device address struct into a string.
 *
 * @param pBtLeAddr     a pointer to the Bluetooth address to convert.
 * @param[out] pBuffer  a buffer in which to place the string.
 *                      Allow U_BD_STRING_MAX_LENGTH_BYTES
 *                      for a Bluetooth address and terminator.
 * @param sizeBytes     the amount of memory pointed to by
 *                      pBuffer.
 * @return              on success the length of the string, not
 *                      including the terminator (i.e. what
 *                      strlen() would return) else negative
 *                      error code.
 */
int32_t uCxBdAddressToString(const uBtLeAddress_t *pBtLeAddr, char *pBuffer, size_t sizeBytes);

/** Convert a MAC address string into a struct.
 *
 * @param pMacString     the null terminated string to convert.
 * @param[out] pMac      a pointer to a place to put the address.
 * @return               zero on success else negative error code.
 */
int32_t uCxStringToMacAddress(const char *pMacString, uMacAddress_t *pMac);

/** Convert a MAC address struct into a string.
 *
 * @param pMac          a pointer to the MAC address to convert.
 * @param[out] pBuffer  a buffer in which to place the string.
 *                      Allow U_MAC_STRING_MAX_LENGTH_BYTES
 *                      for a MAC address and terminator.
 * @param sizeBytes     the amount of memory pointed to by
 *                      pBuffer.
 * @return              on success the length of the string, not
 *                      including the terminator (i.e. what
 *                      strlen() would return) else negative
 *                      error code.
 */
int32_t uCxMacAddressToString(const uMacAddress_t *pMac, char *pBuffer, size_t sizeBytes);

#endif // U_CX_AT_PARAMS_H
