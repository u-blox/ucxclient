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
#include "u_cx_at_util.h"
#include "u_cx_at_params.h"

/* ----------------------------------------------------------------
 * TYPES
 * -------------------------------------------------------------- */

typedef struct {
    const char *pString;
    const uSockIpAddress_t expectedAddr;
} uIpTestEntry_t;

/* ----------------------------------------------------------------
 * STATIC VARIABLES
 * -------------------------------------------------------------- */

/* ----------------------------------------------------------------
 * STATIC FUNCTIONS
 * -------------------------------------------------------------- */

/* ----------------------------------------------------------------
 * TEST FUNCTIONS
 * -------------------------------------------------------------- */

void setUp(void)
{
}

void tearDown(void)
{
}

void test_uCxStringToIpAddress_withValidIpv4Str_expectSuccess(void)
{
    uSockIpAddress_t ipAddress;
    int32_t ret = uCxStringToIpAddress("0.16.32.48", &ipAddress);
    TEST_ASSERT_EQUAL(0, ret);
    TEST_ASSERT_EQUAL(U_SOCK_ADDRESS_TYPE_V4, ipAddress.type);
    TEST_ASSERT_EQUAL(0x00102030, ipAddress.address.ipv4);
}

void test_uCxStringToIpAddress_withValidIpv6Str_expectSuccess(void)
{
    uIpTestEntry_t testEntries[] = {
        {
            .pString = "[0000:1000:2000:3000:4000:5000:6000:7000]",
            .expectedAddr = { .address.ipv6 = { 0x60007000, 0x40005000, 0x20003000, 0x00001000 } }
        },
        {
            .pString = "0000:1000:2000:3000:4000:5000:6000:7000",
            .expectedAddr = { .address.ipv6 = { 0x60007000, 0x40005000, 0x20003000, 0x00001000 } }
        },
        {
            .pString = "0:1000:2000:3000:4000:5000:6000:7000",
            .expectedAddr = { .address.ipv6 = { 0x60007000, 0x40005000, 0x20003000, 0x00001000 } }
        },
        {
            .pString = "::2000:3000:4000:5000:6000:7000",
            .expectedAddr = { .address.ipv6 = { 0x60007000, 0x40005000, 0x20003000, 0x00000000 } }
        },
        {
            .pString = "[::2000:3000:4000:5000:6000:7000]",
            .expectedAddr = { .address.ipv6 = { 0x60007000, 0x40005000, 0x20003000, 0x00000000 } }
        },
        {
            .pString = "[::2000:3000:4000:5000:6000:FFFF]",
            .expectedAddr = { .address.ipv6 = { 0x6000FFFF, 0x40005000, 0x20003000, 0x00000000 } }
        },
        { 0 }
    };

    uIpTestEntry_t *pEntry = &testEntries[0];
    while (pEntry->pString) {
        uSockIpAddress_t ipAddress;
        int32_t ret = uCxStringToIpAddress(pEntry->pString, &ipAddress);
        TEST_ASSERT_EQUAL(0, ret);
        TEST_ASSERT_EQUAL(U_SOCK_ADDRESS_TYPE_V6, ipAddress.type);
        TEST_ASSERT_EQUAL_MEMORY(&pEntry->expectedAddr.address.ipv6,
                                 &ipAddress.address.ipv6,
                                 sizeof(ipAddress.address.ipv6));
        pEntry++;
    }
}

void test_uCxStringToIpAddress_withInvalidIpv4Str_expectError(void)
{
    uSockIpAddress_t ipAddress;
    TEST_ASSERT_LESS_THAN(0, uCxStringToIpAddress("0.16.32", &ipAddress));
    TEST_ASSERT_LESS_THAN(0, uCxStringToIpAddress("0.16.32.48.64", &ipAddress));
    TEST_ASSERT_LESS_THAN(0, uCxStringToIpAddress("0.16.32.48.", &ipAddress));
    TEST_ASSERT_LESS_THAN(0, uCxStringToIpAddress("256.16.32.48", &ipAddress));
    TEST_ASSERT_LESS_THAN(0, uCxStringToIpAddress(".16.32.0", &ipAddress));
    TEST_ASSERT_LESS_THAN(0, uCxStringToIpAddress(" .16.32.0", &ipAddress));
    TEST_ASSERT_LESS_THAN(0, uCxStringToIpAddress("0.16.32.", &ipAddress));
    TEST_ASSERT_LESS_THAN(0, uCxStringToIpAddress("A.16.32.48", &ipAddress));
    TEST_ASSERT_LESS_THAN(0, uCxStringToIpAddress("0,16.32.48", &ipAddress));
}

void test_uCxStringToIpAddress_withInvalidIpv6Str_expectError(void)
{
    uSockIpAddress_t ipAddress;
    TEST_ASSERT_LESS_THAN(0, uCxStringToIpAddress("", &ipAddress));
    TEST_ASSERT_LESS_THAN(0, uCxStringToIpAddress("[0:0:0:0:0:0:0:0", &ipAddress));
    TEST_ASSERT_LESS_THAN(0, uCxStringToIpAddress("0:0:0:0:0:0:0:0]", &ipAddress));
    TEST_ASSERT_LESS_THAN(0, uCxStringToIpAddress("0:0:0:0:0:0:0:0:0", &ipAddress));
    TEST_ASSERT_LESS_THAN(0, uCxStringToIpAddress("0:0:0:0:0:0:0", &ipAddress));
    TEST_ASSERT_LESS_THAN(0, uCxStringToIpAddress("G:0:0:0:0:0:0:0", &ipAddress));
    TEST_ASSERT_LESS_THAN(0, uCxStringToIpAddress("0.0:0:0:0:0:0:0", &ipAddress));
}

void test_uCxIpAddressToString_withIpv4Addr_expectValidIpString(void)
{
    const uSockIpAddress_t ipAddress = {
        .type = U_SOCK_ADDRESS_TYPE_V4,
        .address.ipv4 = 0x00102030
    };
    char buffer[15 + 1];
    int32_t ret = uCxIpAddressToString(&ipAddress, &buffer[0], sizeof(buffer));
    TEST_ASSERT_EQUAL(10, ret);
    TEST_ASSERT_EQUAL_STRING("0.16.32.48", buffer);
}

void test_uCxIpAddressToString_withIpv6Addr_expectValidIpString(void)
{
    const uSockIpAddress_t ipAddress = {
        .type = U_SOCK_ADDRESS_TYPE_V6,
        .address.ipv6 = {
            0x60007000,
            0x40005000,
            0x20003000,
            0x00001000
        }
    };
    char buffer[2 /* "[]" */ + 32 /* "0000" */ + 7 /* ":" */ + 1 /* null */];
    TEST_ASSERT_EQUAL(U_IP_STRING_MAX_LENGTH_BYTES, sizeof(buffer));
    int32_t ret = uCxIpAddressToString(&ipAddress, &buffer[0], sizeof(buffer));
    TEST_ASSERT_EQUAL(41, ret);
    TEST_ASSERT_EQUAL_STRING("[0000:1000:2000:3000:4000:5000:6000:7000]", buffer);
}

void test_uCxIpAddressToString_withIpv4AddrAndLimitedBuf_expectNoBufferOverflow(void)
{
    const uSockIpAddress_t ipAddress = {
        .type = U_SOCK_ADDRESS_TYPE_V4,
        .address.ipv4 = 0xFFFFFFFF
    };
    char buffer[15 + 1];
    int32_t ret = uCxIpAddressToString(&ipAddress, &buffer[0], sizeof(buffer));
    TEST_ASSERT_EQUAL(15, ret);
    TEST_ASSERT_EQUAL_STRING("255.255.255.255", buffer);
}

void test_uCxIpAddressToString_withTooSmallBuffer_expectError(void)
{
    const uSockIpAddress_t ipAddress = {
        .type = U_SOCK_ADDRESS_TYPE_V4,
        .address.ipv4 = 0xFFFFFFFF
    };
    char buffer[15];
    int32_t ret = uCxIpAddressToString(&ipAddress, &buffer[0], sizeof(buffer));
    TEST_ASSERT_LESS_THAN(0, ret);
}

void test_uCxStringToMacAddress_withValidIpv4Str_expectSuccess(void)
{
    uMacAddress_t macAddress;
    const uint8_t expData[] = {0x00,0x11,0x22,0x33,0x44,0x55};
    int32_t ret = uCxStringToMacAddress("001122334455", &macAddress);
    TEST_ASSERT_EQUAL(0, ret);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expData, macAddress.address, U_MAC_ADDR_LEN);
}

void test_uCxStringToMacAddress_withInvalidMacStr_expectError(void)
{
    uMacAddress_t macAddress;
    TEST_ASSERT_LESS_THAN(0, uCxStringToMacAddress("", &macAddress));
    TEST_ASSERT_LESS_THAN(0, uCxStringToMacAddress("00112233445566", &macAddress));
    TEST_ASSERT_LESS_THAN(0, uCxStringToMacAddress("0011223344", &macAddress));
    TEST_ASSERT_LESS_THAN(0, uCxStringToMacAddress("00112233445", &macAddress));
    TEST_ASSERT_LESS_THAN(0, uCxStringToMacAddress("0G1122334455", &macAddress));
    TEST_ASSERT_LESS_THAN(0, uCxStringToMacAddress(" 01122334455", &macAddress));
    TEST_ASSERT_LESS_THAN(0, uCxStringToMacAddress(" 001122334455", &macAddress));
}

void test_uCxMacAddressToString_withLimitedBuf_expectNoBufferOverflow(void)
{
    const uMacAddress_t macAddress = {
        .address = {0x00,0x11,0x22,0x33,0x44,0x55}
    };
    char buffer[U_MAC_STRING_MAX_LENGTH_BYTES];
    int32_t ret = uCxMacAddressToString(&macAddress, &buffer[0], sizeof(buffer));
    TEST_ASSERT_EQUAL(12, ret);
    TEST_ASSERT_EQUAL_STRING("001122334455", buffer);
}

void test_uCxMacAddressToString_withTooSmallBuffer_expectError(void)
{
    const uMacAddress_t macAddress = {
        .address = {0}
    };
    char buffer[U_MAC_STRING_MAX_LENGTH_BYTES - 1];
    int32_t ret = uCxMacAddressToString(&macAddress, &buffer[0], sizeof(buffer));
    TEST_ASSERT_LESS_THAN(0, ret);
}

void test_uCxStringToBdAddress_withPublicAddrStr_expectPublicAddr(void)
{
    uBtLeAddress_t btLeAddress;
    const uint8_t expData[] = {0x00,0x11,0x22,0x33,0x44,0x55};

    TEST_ASSERT_EQUAL(0, uCxStringToBdAddress("001122334455p", &btLeAddress));
    TEST_ASSERT_EQUAL(U_BD_ADDRESS_TYPE_PUBLIC, btLeAddress.type);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expData, btLeAddress.address, U_BD_ADDR_LEN);

    memset(&btLeAddress, 0, sizeof(btLeAddress));
    TEST_ASSERT_EQUAL(0, uCxStringToBdAddress("001122334455", &btLeAddress));
    TEST_ASSERT_EQUAL(U_BD_ADDRESS_TYPE_PUBLIC, btLeAddress.type);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expData, btLeAddress.address, U_BD_ADDR_LEN);
}

void test_uCxStringToBdAddress_withRandomAddrStr_expectRandomAddr(void)
{
    uBtLeAddress_t btLeAddress;
    const uint8_t expData[] = {0x00,0x11,0x22,0x33,0x44,0x55};

    TEST_ASSERT_EQUAL(0, uCxStringToBdAddress("001122334455r", &btLeAddress));
    TEST_ASSERT_EQUAL(U_BD_ADDRESS_TYPE_RANDOM, btLeAddress.type);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expData, btLeAddress.address, U_BD_ADDR_LEN);
}

void test_uCxBdAddressToString_withPublicAddr_expectPublicAddrStr(void)
{
    const uBtLeAddress_t btLeAddress = {
        .type = U_BD_ADDRESS_TYPE_PUBLIC,
        .address = {0x00,0x11,0x22,0x33,0x44,0x55}
    };
    char buffer[U_BD_STRING_MAX_LENGTH_BYTES];
    int32_t ret = uCxBdAddressToString(&btLeAddress, &buffer[0], sizeof(buffer));
    TEST_ASSERT_EQUAL(13, ret);
    TEST_ASSERT_EQUAL_STRING("001122334455p", buffer);
}

void test_uCxBdAddressToString_withRandomAddr_expectRandomAddrStr(void)
{
    const uBtLeAddress_t btLeAddress = {
        .type = U_BD_ADDRESS_TYPE_RANDOM,
        .address = {0x00,0x11,0x22,0x33,0x44,0x55}
    };
    char buffer[U_BD_STRING_MAX_LENGTH_BYTES];
    int32_t ret = uCxBdAddressToString(&btLeAddress, &buffer[0], sizeof(buffer));
    TEST_ASSERT_EQUAL(13, ret);
    TEST_ASSERT_EQUAL_STRING("001122334455r", buffer);
}

void test_uCxBdAddressToString_withTooSmallBuffer_expectError(void)
{
    const uBtLeAddress_t btLeAddress = {
        .address = {0}
    };
    char buffer[U_BD_STRING_MAX_LENGTH_BYTES - 1];
    int32_t ret = uCxBdAddressToString(&btLeAddress, &buffer[0], sizeof(buffer));
    TEST_ASSERT_LESS_THAN(0, ret);
}
