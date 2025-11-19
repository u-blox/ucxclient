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

#include "unity.h"
#include "u_cx_at_params.h"
#include "u_cx_at_util.h"

/* ----------------------------------------------------------------
 * STATIC VARIABLES
 * -------------------------------------------------------------- */

static uint8_t gBinarySeq[32];
static char gHexSeq[(sizeof(gBinarySeq) * 2) + 1] = {0};

/* ----------------------------------------------------------------
 * STATIC FUNCTIONS
 * -------------------------------------------------------------- */

static int uCxAtUtilParseParamsVaList_wrapper(char *pParamsLine, const char *pParamFmt, ...)
{
    char data[strlen(pParamsLine) + 1];
    strcpy(data, pParamsLine);
    va_list args;
    va_start(args, pParamFmt);
    int ret = uCxAtUtilParseParamsVaList(data, pParamFmt, args);
    va_end(args);
    return ret;
}

/* ----------------------------------------------------------------
 * TEST FUNCTIONS
 * -------------------------------------------------------------- */

void setUp(void)
{
    if (gHexSeq[0] == 0) {
        for (unsigned int i = 0; i < sizeof(gBinarySeq); i++ ) {
            char buf[16];
            sprintf(buf, "%02X", i);
            strcat(gHexSeq, buf);
            gBinarySeq[i] = i;
        }
    }
}

void tearDown(void)
{
}

void test_uCxAtUtilByteToHex_expectedOutput(void)
{
    char buf[3];

    uCxAtUtilByteToHex(0x00, &buf[0]);
    TEST_ASSERT_EQUAL_STRING("00", buf);
    uCxAtUtilByteToHex(0x3F, &buf[0]);
    TEST_ASSERT_EQUAL_STRING("3F", buf);
    uCxAtUtilByteToHex(0xFF, &buf[0]);
    TEST_ASSERT_EQUAL_STRING("FF", buf);
}

void test_uCxAtUtilHexToByte_withValidInput_expectOutput(void)
{
    uint8_t byte = 11;
    TEST_ASSERT_EQUAL(0, uCxAtUtilHexToByte("00", &byte));
    TEST_ASSERT_EQUAL(0x00, byte);
    TEST_ASSERT_EQUAL(0, uCxAtUtilHexToByte("0000", &byte));
    TEST_ASSERT_EQUAL(0x00, byte);
    TEST_ASSERT_EQUAL(0, uCxAtUtilHexToByte("003F", &byte));
    TEST_ASSERT_EQUAL(0x00, byte);
    TEST_ASSERT_EQUAL(0, uCxAtUtilHexToByte("3F", &byte));
    TEST_ASSERT_EQUAL(0x3f, byte);
    TEST_ASSERT_EQUAL(0, uCxAtUtilHexToByte("FF", &byte));
    TEST_ASSERT_EQUAL(0xff, byte);
}

void test_uCxAtUtilHexToByte_withInvalidInput_expectError(void)
{
    uint8_t byte;
    TEST_ASSERT_LESS_THAN(0, uCxAtUtilHexToByte("0", &byte));
    TEST_ASSERT_LESS_THAN(0, uCxAtUtilHexToByte(" ", &byte));
    TEST_ASSERT_LESS_THAN(0, uCxAtUtilHexToByte("0x00", &byte));
    TEST_ASSERT_LESS_THAN(0, uCxAtUtilHexToByte("x", &byte));
}

void test_uCxAtUtilFindParamEnd_withValidInput_expectOutput(void)
{
    char testData[64];

    strcpy(testData, "123");
    TEST_ASSERT_EQUAL(&testData[0] + strlen("123"), uCxAtUtilFindParamEnd(testData));

    strcpy(testData, "123,");
    TEST_ASSERT_EQUAL(&testData[0] + strlen("123"), uCxAtUtilFindParamEnd(testData));

    strcpy(testData, "\"stringparam\",123");
    TEST_ASSERT_EQUAL(&testData[0] + strlen("\"stringparam\""), uCxAtUtilFindParamEnd(testData));

    strcpy(testData, "\"string, with comma\",123");
    TEST_ASSERT_EQUAL(&testData[0] + strlen("\"string, with comma\""), uCxAtUtilFindParamEnd(testData));

    strcpy(testData, "\"string, with comma\"");
    TEST_ASSERT_EQUAL(&testData[0] + strlen("\"string, with comma\""), uCxAtUtilFindParamEnd(testData));

    strcpy(testData, "\"string with esc \\\" \",123");
    TEST_ASSERT_EQUAL(&testData[0] + strlen("\"string with esc \\\" \""), uCxAtUtilFindParamEnd(testData));

    strcpy(testData, "string with escaped comma \\, ,123");
    TEST_ASSERT_EQUAL(&testData[0] + strlen("string with escaped comma \\, "), uCxAtUtilFindParamEnd(testData));

    strcpy(testData, ",123");
    TEST_ASSERT_EQUAL(&testData[0], uCxAtUtilFindParamEnd(testData));
}

void test_uCxAtUtilFindParamEnd_withInvalidInput_expectNull(void)
{
    TEST_ASSERT_EQUAL(NULL, uCxAtUtilFindParamEnd("\"missing end quote"));
    TEST_ASSERT_EQUAL(NULL, uCxAtUtilFindParamEnd("missing escape char\\"));
}


void test_uCxAtUtilParseParamsF_withValidInput_expectParsedParams(void)
{
    char *pStr;
    int32_t digit;
    char testData[64];

    strcpy(testData, "abc,123");
    TEST_ASSERT_EQUAL(2, uCxAtUtilParseParamsF(testData, "sd", &pStr, &digit, U_CX_AT_UTIL_PARAM_LAST));
    TEST_ASSERT_EQUAL_STRING("abc", pStr);
    TEST_ASSERT_EQUAL(123, digit);

    strcpy(testData, "\"def\",456");
    TEST_ASSERT_EQUAL(2, uCxAtUtilParseParamsF(testData, "sd", &pStr, &digit, U_CX_AT_UTIL_PARAM_LAST));
    TEST_ASSERT_EQUAL_STRING("def", pStr);
    TEST_ASSERT_EQUAL(456, digit);

    strcpy(testData, "\"\",789");
    TEST_ASSERT_EQUAL(2, uCxAtUtilParseParamsF(testData, "sd", &pStr, &digit, U_CX_AT_UTIL_PARAM_LAST));
    TEST_ASSERT_EQUAL_STRING("", pStr);
    TEST_ASSERT_EQUAL(789, digit);

    strcpy(testData, ",123");
    TEST_ASSERT_EQUAL(2, uCxAtUtilParseParamsF(testData, "sd", &pStr, &digit, U_CX_AT_UTIL_PARAM_LAST));
    TEST_ASSERT_EQUAL_STRING("", pStr);
    TEST_ASSERT_EQUAL(123, digit);

    strcpy(testData, "");
    TEST_ASSERT_EQUAL(1, uCxAtUtilParseParamsF(testData, "s", &pStr, &digit, U_CX_AT_UTIL_PARAM_LAST));
    TEST_ASSERT_EQUAL_STRING("", pStr);
}

void test_uCxAtUtilParseParamsF_withInvalidInput_expectNegativeReturnValue(void)
{
    char *pStr;
    int32_t digit;
    char testData[64];

    strcpy(testData, "123,\"missing quote");
    TEST_ASSERT_LESS_THAN(0, uCxAtUtilParseParamsF(testData, "ds", &digit, &pStr, U_CX_AT_UTIL_PARAM_LAST));

    strcpy(testData, "123,missing esc char\\");
    TEST_ASSERT_LESS_THAN(0, uCxAtUtilParseParamsF(testData, "ds", &digit, &pStr, U_CX_AT_UTIL_PARAM_LAST));

    strcpy(testData, ",int param not allowed to be empty");
    TEST_ASSERT_LESS_THAN(0, uCxAtUtilParseParamsF(testData, "ds", &digit, &pStr, U_CX_AT_UTIL_PARAM_LAST));
}

void test_uCxAtUtilParseParamsF_withIpAddr_expectParsedParam(void)
{
    uSockIpAddress_t ipAddr;
    char testData[64];

    strcpy(testData, "0.16.32.48");
    TEST_ASSERT_EQUAL(1, uCxAtUtilParseParamsF(testData, "i", &ipAddr, U_CX_AT_UTIL_PARAM_LAST));
    TEST_ASSERT_EQUAL(U_SOCK_ADDRESS_TYPE_V4, ipAddr.type);
    TEST_ASSERT_EQUAL(0x00102030, ipAddr.address.ipv4);
}

void test_uCxAtUtilParseParamsF_withIgnoredParam(void)
{
    int32_t digit = -1;
    char testData[64];

    strcpy(testData, "1,2,3");
    TEST_ASSERT_EQUAL(3, uCxAtUtilParseParamsF(testData, "--d", &digit, U_CX_AT_UTIL_PARAM_LAST));
    TEST_ASSERT_EQUAL(3, digit);
}

void test_uCxAtUtilHexToBinary_withValidValues_expectSuccess(void)
{
    uint8_t buffer[32];
    const char *seq1[] = {
        "",
        "00",
        "000102030405060708090A0B0C0D0E0F",
        "000102030405060708090a0b0c0d0e0f",
        NULL
    };
    const char *seq2[] = {
        "00102030405060708090A0B0C0D0E0F0",
        "00102030405060708090a0b0c0d0e0f0",
        NULL
    };

    for (int i = 0; seq1[i] != NULL; i++) {
        memset(buffer, 0xFF, sizeof(buffer));
        uint32_t ret = uCxAtUtilHexToBinary(seq1[i], buffer, sizeof(buffer));
        TEST_ASSERT_EQUAL_UINT(strlen(seq1[i]) / 2, ret);
        for (uint32_t j = 0; j < ret; j++) {
            TEST_ASSERT_EQUAL_INT(j, buffer[j]);
        }
    }

    for (int i = 0; seq2[i] != NULL; i++) {
        memset(buffer, 0xFF, sizeof(buffer));
        uint32_t ret = uCxAtUtilHexToBinary(seq2[i], buffer, sizeof(buffer));
        TEST_ASSERT_EQUAL_UINT(strlen(seq2[i]) / 2, ret);
        for (uint32_t j = 0; j < ret; j++) {
            TEST_ASSERT_EQUAL_INT(j * 16, buffer[j]);
        }
    }
}

void test_uCxAtUtilHexToBinary_withInvalidValues_expectFailure(void)
{
    uint8_t buffer[32];
    const char *seq1[] = { "0", "0G", "0x", "0+", NULL };
    const char *seq2[] = { "+0", "00+0", "0001+", "0001020+", NULL };

    // seq1 should return 0 bytes
    for (int i = 0; seq1[i] != NULL; i++) {
        memset(buffer, 0xFF, sizeof(buffer));
        uint32_t ret = uCxAtUtilHexToBinary(seq1[i], buffer, sizeof(buffer));
        TEST_ASSERT_EQUAL_UINT(0, ret);
        TEST_ASSERT_EQUAL_UINT8(0xFF, buffer[0]);
    }

    for (int i = 0; seq2[i] != NULL; i++) {
        memset(buffer, 0xFF, sizeof(buffer));
        uint32_t ret = uCxAtUtilHexToBinary(seq2[i], buffer, sizeof(buffer));
        TEST_ASSERT_EQUAL_UINT(i, ret);
        for (uint32_t j = 0; j < ret; j++) {
            TEST_ASSERT_EQUAL_INT(j, buffer[j]);
        }
    }
}

void test_uCxAtUtilHexToBinary_withTightBuffer_expectSuccess(void)
{
    // uStringHexToBinary() should only output bufSize number of bytes
    uint8_t buffer[32];
    const char seq[] = "000102030405060708090A0B0C0D0E0F";
    for (int i = 0; i < 16; i++) {
        memset(buffer, 0xFF, sizeof(buffer));
        uint32_t ret = uCxAtUtilHexToBinary(seq, buffer, i);
        TEST_ASSERT_EQUAL_UINT(i, ret);
        for (uint32_t j = 0; j < ret; j++) {
            TEST_ASSERT_EQUAL_INT(j, buffer[j]);
        }
        TEST_ASSERT_EQUAL_UINT8(0xFF, buffer[ret]);
    }
}

void test_uCxAtUtilBinaryToHex_withValidData_expectSuccess(void)
{
    char buf[(sizeof(gBinarySeq) * 2) + 1] = { 0 };
    TEST_ASSERT(uCxAtUtilBinaryToHex(gBinarySeq, sizeof(gBinarySeq), buf, sizeof(buf)));
    TEST_ASSERT_EQUAL_STRING(gHexSeq, buf);

}

void test_uCxAtUtilBinaryToHex_withNull_expectNullString(void)
{
    char buf[16];
    TEST_ASSERT(uCxAtUtilBinaryToHex(NULL, 1, buf, sizeof(buf)));
    TEST_ASSERT_EQUAL_STRING("(null)", buf);

    TEST_ASSERT_FALSE(uCxAtUtilBinaryToHex(NULL, 1, buf, 2));
}

void test_uCxAtUtilBinaryToHex_withTooSmallBuffer_expectFailure(void)
{
    char buf[sizeof(gBinarySeq)];
    TEST_ASSERT_FALSE(uCxAtUtilBinaryToHex(gBinarySeq, sizeof(gBinarySeq), buf, sizeof(buf)));
    TEST_ASSERT_FALSE(uCxAtUtilBinaryToHex(NULL, 1, buf, 2));
}


void test_uCxAtUtilParseParamsVaList_withByteArray(void)
{
    uByteArray_t byteArray;
    uint8_t expData[] = {0x00,0x11,0x22,0x33,0x44,0x55};
    int32_t ret = uCxAtUtilParseParamsVaList_wrapper("001122334455", "h", &byteArray);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(byteArray.pData, expData, sizeof(expData));
    TEST_ASSERT_EQUAL(sizeof(expData), byteArray.length);
    TEST_ASSERT_EQUAL(1, ret);
}

void test_uCxAtUtilParseParamsF_withBinaryString_expectSuccess(void)
{
    char buf[] = "\"test\"";
    uBinaryString_t binStr;
    int32_t ret = uCxAtUtilParseParamsF(buf, "$", &binStr, U_CX_AT_UTIL_PARAM_LAST);
    TEST_ASSERT_EQUAL(1, ret);
    TEST_ASSERT_EQUAL(4, binStr.length);
    TEST_ASSERT_EQUAL_MEMORY("test", binStr.pData, 4);
}

void test_uCxAtUtilParseParamsF_withBinaryStringWithEscapes_expectSuccess(void)
{
    char buf[] = "\"te\\\"st\"";
    uBinaryString_t binStr;
    int32_t ret = uCxAtUtilParseParamsF(buf, "$", &binStr, U_CX_AT_UTIL_PARAM_LAST);
    TEST_ASSERT_EQUAL(1, ret);
    TEST_ASSERT_EQUAL(5, binStr.length);
    TEST_ASSERT_EQUAL_MEMORY("te\"st", binStr.pData, 5);
}

void test_uCxAtUtilParseParamsF_withBinaryStringWithNullChar_expectSuccess(void)
{
    char buf[] = "\"te\\0st\"";
    uBinaryString_t binStr;
    int32_t ret = uCxAtUtilParseParamsF(buf, "$", &binStr, U_CX_AT_UTIL_PARAM_LAST);
    TEST_ASSERT_EQUAL(1, ret);
    TEST_ASSERT_EQUAL(5, binStr.length);
    char expected[] = "te\0st";
    TEST_ASSERT_EQUAL_MEMORY(expected, binStr.pData, 5);
}

void test_uCxAtUtilParseParamsF_withEmptyBinaryString_expectSuccess(void)
{
    char buf[] = "\"\"";
    uBinaryString_t binStr;
    int32_t ret = uCxAtUtilParseParamsF(buf, "$", &binStr, U_CX_AT_UTIL_PARAM_LAST);
    TEST_ASSERT_EQUAL(1, ret);
    TEST_ASSERT_EQUAL(0, binStr.length);
}

void test_uCxAtUtilParseParamsF_withHexSmallData_expectSuccess(void)
{
    char buf[] = "010203";
    uByteArray_t byteArray;
    int32_t ret = uCxAtUtilParseParamsF(buf, "h", &byteArray, U_CX_AT_UTIL_PARAM_LAST);
    uint8_t expected[] = {0x01, 0x02, 0x03};
    TEST_ASSERT_EQUAL(1, ret);
    TEST_ASSERT_EQUAL(3, byteArray.length);
    TEST_ASSERT_EQUAL_MEMORY(expected, byteArray.pData, 3);
}

void test_uCxAtUtilParseParamsF_withHexLargeData_expectSuccess(void)
{
    char buf[] = "000102030405060708090A0B0C0D0E0F101112131415161718191A1B1C1D";
    uByteArray_t byteArray;
    int32_t ret = uCxAtUtilParseParamsF(buf, "h", &byteArray, U_CX_AT_UTIL_PARAM_LAST);
    uint8_t expected[30];
    for (int i = 0; i < 30; i++) {
        expected[i] = (uint8_t)i;
    }
    TEST_ASSERT_EQUAL(1, ret);
    TEST_ASSERT_EQUAL(30, byteArray.length);
    TEST_ASSERT_EQUAL_MEMORY(expected, byteArray.pData, 30);
}

void test_uCxAtUtilParseParamsF_withHexEmptyData_expectSuccess(void)
{
    char buf[] = "";
    uByteArray_t byteArray;
    int32_t ret = uCxAtUtilParseParamsF(buf, "h", &byteArray, U_CX_AT_UTIL_PARAM_LAST);
    TEST_ASSERT_EQUAL(1, ret);
    TEST_ASSERT_EQUAL(0, byteArray.length);
}

void test_uCxAtUtilParseParamsF_withStringEscapeQuote_expectSuccess(void)
{
    char buf[] = "\"te\\\"st\"";
    const char *pStr;
    int32_t ret = uCxAtUtilParseParamsF(buf, "s", &pStr, U_CX_AT_UTIL_PARAM_LAST);
    TEST_ASSERT_EQUAL(1, ret);
    TEST_ASSERT_EQUAL_STRING("te\"st", pStr);
}

void test_uCxAtUtilParseParamsF_withStringEscapeBackslash_expectSuccess(void)
{
    char buf[] = "\"te\\\\st\"";
    const char *pStr;
    int32_t ret = uCxAtUtilParseParamsF(buf, "s", &pStr, U_CX_AT_UTIL_PARAM_LAST);
    TEST_ASSERT_EQUAL(1, ret);
    TEST_ASSERT_EQUAL_STRING("te\\st", pStr);
}

void test_uCxAtUtilParseParamsF_withStringEscapeNewline_expectSuccess(void)
{
    char buf[] = "\"te\\nst\"";
    const char *pStr;
    int32_t ret = uCxAtUtilParseParamsF(buf, "s", &pStr, U_CX_AT_UTIL_PARAM_LAST);
    TEST_ASSERT_EQUAL(1, ret);
    TEST_ASSERT_EQUAL_STRING("te\nst", pStr);
}

void test_uCxAtUtilParseParamsF_withStringEscapeCarriageReturn_expectSuccess(void)
{
    char buf[] = "\"te\\rst\"";
    const char *pStr;
    int32_t ret = uCxAtUtilParseParamsF(buf, "s", &pStr, U_CX_AT_UTIL_PARAM_LAST);
    TEST_ASSERT_EQUAL(1, ret);
    TEST_ASSERT_EQUAL_STRING("te\rst", pStr);
}

void test_uCxAtUtilParseParamsF_withStringEscapeTab_expectSuccess(void)
{
    char buf[] = "\"te\\tst\"";
    const char *pStr;
    int32_t ret = uCxAtUtilParseParamsF(buf, "s", &pStr, U_CX_AT_UTIL_PARAM_LAST);
    TEST_ASSERT_EQUAL(1, ret);
    TEST_ASSERT_EQUAL_STRING("te\tst", pStr);
}

void test_uCxAtUtilParseParamsF_withStringEscapeBackspace_expectSuccess(void)
{
    char buf[] = "\"te\\bst\"";
    const char *pStr;
    int32_t ret = uCxAtUtilParseParamsF(buf, "s", &pStr, U_CX_AT_UTIL_PARAM_LAST);
    TEST_ASSERT_EQUAL(1, ret);
    TEST_ASSERT_EQUAL_STRING("te\bst", pStr);
}

void test_uCxAtUtilParseParamsF_withStringEscapeHex_expectSuccess(void)
{
    char buf[] = "\"te\\x01st\"";
    const char *pStr;
    int32_t ret = uCxAtUtilParseParamsF(buf, "s", &pStr, U_CX_AT_UTIL_PARAM_LAST);
    TEST_ASSERT_EQUAL(1, ret);
    char expected[] = {'t','e','\x01','s','t','\0'};
    TEST_ASSERT_EQUAL_STRING(expected, pStr);
}

void test_uCxAtUtilParseParamsF_withIntListShort_expectSuccess(void)
{
    char buf[] = "[1,2,3]";
    uIntList_t intList;
    int32_t ret = uCxAtUtilParseParamsF(buf, "l", &intList, U_CX_AT_UTIL_PARAM_LAST);
    TEST_ASSERT_EQUAL(1, ret);
    TEST_ASSERT_EQUAL(3, intList.length);
    TEST_ASSERT_EQUAL(1, intList.pIntValues[0]);
    TEST_ASSERT_EQUAL(2, intList.pIntValues[1]);
    TEST_ASSERT_EQUAL(3, intList.pIntValues[2]);
}

void test_uCxAtUtilParseParamsF_withIntListLong_expectSuccess(void)
{
    char buf[] = "[1,2,3,4,5,6,7,8,9,10,11,36,40,44,48,52,56,60,64]";
    uIntList_t intList;
    int32_t ret = uCxAtUtilParseParamsF(buf, "l", &intList, U_CX_AT_UTIL_PARAM_LAST);
    TEST_ASSERT_EQUAL(1, ret);
    TEST_ASSERT_EQUAL(19, intList.length);
    TEST_ASSERT_EQUAL(1, intList.pIntValues[0]);
    TEST_ASSERT_EQUAL(10, intList.pIntValues[9]);
    TEST_ASSERT_EQUAL(64, intList.pIntValues[18]);
}

void test_uCxAtUtilParseParamsF_withIntListAndOtherParams_expectSuccess(void)
{
    char buf[] = "123,[1,2,3,4,5],\"test\"";
    int32_t num;
    uIntList_t intList;
    char *pStr;
    int32_t ret = uCxAtUtilParseParamsF(buf, "dls", &num, &intList, &pStr, U_CX_AT_UTIL_PARAM_LAST);
    TEST_ASSERT_EQUAL(3, ret);
    TEST_ASSERT_EQUAL(123, num);
    TEST_ASSERT_EQUAL(5, intList.length);
    TEST_ASSERT_EQUAL(1, intList.pIntValues[0]);
    TEST_ASSERT_EQUAL(5, intList.pIntValues[4]);
    TEST_ASSERT_EQUAL_STRING("test", pStr);
}

void test_uCxAtUtilParseParamsF_withLongIntListAndOtherParams_expectSuccess(void)
{
    char buf[] = "456,[1,2,3,4,5,6,7,8,9,10,11,36,40,44,48,52,56,60,64],\"long test\"";
    int32_t num;
    uIntList_t intList;
    char *pStr;
    int32_t ret = uCxAtUtilParseParamsF(buf, "dls", &num, &intList, &pStr, U_CX_AT_UTIL_PARAM_LAST);
    TEST_ASSERT_EQUAL(3, ret);
    TEST_ASSERT_EQUAL(456, num);
    TEST_ASSERT_EQUAL(19, intList.length);
    TEST_ASSERT_EQUAL(1, intList.pIntValues[0]);
    TEST_ASSERT_EQUAL(64, intList.pIntValues[18]);
    TEST_ASSERT_EQUAL_STRING("long test", pStr);
}

void test_uCxAtUtilReplaceChar_withTestString_replaceSwithB()
{
    char str[] = "MyTeststring";
    uCxAtUtilReplaceChar(str, strlen(str), 's', 'b');
    TEST_ASSERT_EQUAL_STRING("MyTebtbtring", str);
}

void test_uCxAtUtilReplaceChar_withNullTermTestString_replaceNullWithComma()
{
    char str[] = "My\0Test\0String";
    uCxAtUtilReplaceChar(str, sizeof(str) - 1, 0, ',');
    TEST_ASSERT_EQUAL_STRING("My,Test,String", str);
}
