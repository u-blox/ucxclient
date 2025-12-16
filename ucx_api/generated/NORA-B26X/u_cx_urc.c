/*
* This file was automatically generated using csnake v0.3.5.
*
* This file should not be edited directly, any changes will be
* overwritten next time the script is run.
*
* Source code for csnake is available at:
* https://gitlab.com/andrejr/csnake
*
* csnake is also available on PyPI, at :
* https://pypi.org/project/csnake
*/
#include <stddef.h>
#include <string.h>
#include "u_cx.h"

/* ------------------------------------------------------------
 * PARSER FUNCTIONS
 * ---------------------------------------------------------- */

static int32_t parseSTARTUP(uCxHandle_t * puCxHandle, char * pParams, size_t paramsLength)
{
    (void)paramsLength;
    int32_t ret = uCxAtUtilParseParamsF(pParams, "", U_CX_AT_UTIL_PARAM_LAST);
    if ((ret >= 0) && puCxHandle->callbacks.STARTUP) {
        puCxHandle->callbacks.STARTUP(puCxHandle);
    }
    return ret;
}

static int32_t parseUEBTC(uCxHandle_t * puCxHandle, char * pParams, size_t paramsLength)
{
    (void)paramsLength;
    int32_t conn_handle;
    uBtLeAddress_t bd_addr;
    int32_t ret = uCxAtUtilParseParamsF(pParams, "db", &conn_handle, &bd_addr, U_CX_AT_UTIL_PARAM_LAST);
    if ((ret >= 0) && puCxHandle->callbacks.UEBTC) {
        puCxHandle->callbacks.UEBTC(puCxHandle, conn_handle, &bd_addr);
    }
    return ret;
}

static int32_t parseUEBTDC(uCxHandle_t * puCxHandle, char * pParams, size_t paramsLength)
{
    (void)paramsLength;
    int32_t conn_handle;
    int32_t ret = uCxAtUtilParseParamsF(pParams, "d", &conn_handle, U_CX_AT_UTIL_PARAM_LAST);
    if ((ret >= 0) && puCxHandle->callbacks.UEBTDC) {
        puCxHandle->callbacks.UEBTDC(puCxHandle, conn_handle);
    }
    return ret;
}

static int32_t parseUEBTB(uCxHandle_t * puCxHandle, char * pParams, size_t paramsLength)
{
    (void)paramsLength;
    uBtLeAddress_t bd_addr;
    int32_t bond_status;
    int32_t ret = uCxAtUtilParseParamsF(pParams, "bd", &bd_addr, &bond_status, U_CX_AT_UTIL_PARAM_LAST);
    if ((ret >= 0) && puCxHandle->callbacks.UEBTB) {
        puCxHandle->callbacks.UEBTB(puCxHandle, &bd_addr, bond_status);
    }
    return ret;
}

static int32_t parseUEBTUC(uCxHandle_t * puCxHandle, char * pParams, size_t paramsLength)
{
    (void)paramsLength;
    uBtLeAddress_t bd_addr;
    int32_t numeric_value;
    int32_t ret = uCxAtUtilParseParamsF(pParams, "bd", &bd_addr, &numeric_value, U_CX_AT_UTIL_PARAM_LAST);
    if ((ret >= 0) && puCxHandle->callbacks.UEBTUC) {
        puCxHandle->callbacks.UEBTUC(puCxHandle, &bd_addr, numeric_value);
    }
    return ret;
}

static int32_t parseUEBTUPD(uCxHandle_t * puCxHandle, char * pParams, size_t paramsLength)
{
    (void)paramsLength;
    uBtLeAddress_t bd_addr;
    int32_t numeric_value;
    int32_t ret = uCxAtUtilParseParamsF(pParams, "bd", &bd_addr, &numeric_value, U_CX_AT_UTIL_PARAM_LAST);
    if ((ret >= 0) && puCxHandle->callbacks.UEBTUPD) {
        puCxHandle->callbacks.UEBTUPD(puCxHandle, &bd_addr, numeric_value);
    }
    return ret;
}

static int32_t parseUEBTUPE(uCxHandle_t * puCxHandle, char * pParams, size_t paramsLength)
{
    (void)paramsLength;
    uBtLeAddress_t bd_addr;
    int32_t ret = uCxAtUtilParseParamsF(pParams, "b", &bd_addr, U_CX_AT_UTIL_PARAM_LAST);
    if ((ret >= 0) && puCxHandle->callbacks.UEBTUPE) {
        puCxHandle->callbacks.UEBTUPE(puCxHandle, &bd_addr);
    }
    return ret;
}

static int32_t parseUEBTPHYU(uCxHandle_t * puCxHandle, char * pParams, size_t paramsLength)
{
    (void)paramsLength;
    int32_t conn_handle;
    int32_t phy_status;
    int32_t tx_phy;
    int32_t rx_phy;
    int32_t ret = uCxAtUtilParseParamsF(pParams, "dddd", &conn_handle, &phy_status, &tx_phy, &rx_phy, U_CX_AT_UTIL_PARAM_LAST);
    if ((ret >= 0) && puCxHandle->callbacks.UEBTPHYU) {
        puCxHandle->callbacks.UEBTPHYU(puCxHandle, conn_handle, phy_status, tx_phy, rx_phy);
    }
    return ret;
}

static int32_t parseUEBTBGD(uCxHandle_t * puCxHandle, char * pParams, size_t paramsLength)
{
    (void)paramsLength;
    uBtLeAddress_t bd_addr;
    int32_t rssi;
    const char * device_name;
    int32_t data_type;
    uByteArray_t data;
    int32_t ret = uCxAtUtilParseParamsF(pParams, "bdsdh", &bd_addr, &rssi, &device_name, &data_type, &data, U_CX_AT_UTIL_PARAM_LAST);
    if ((ret >= 0) && puCxHandle->callbacks.UEBTBGD) {
        puCxHandle->callbacks.UEBTBGD(puCxHandle, &bd_addr, rssi, device_name, data_type, &data);
    }
    return ret;
}

static int32_t parseUEBTGCN(uCxHandle_t * puCxHandle, char * pParams, size_t paramsLength)
{
    (void)paramsLength;
    int32_t conn_handle;
    int32_t value_handle;
    uByteArray_t hex_data;
    int32_t ret = uCxAtUtilParseParamsF(pParams, "ddh", &conn_handle, &value_handle, &hex_data, U_CX_AT_UTIL_PARAM_LAST);
    if ((ret >= 0) && puCxHandle->callbacks.UEBTGCN) {
        puCxHandle->callbacks.UEBTGCN(puCxHandle, conn_handle, value_handle, &hex_data);
    }
    return ret;
}

static int32_t parseUEBTGCI(uCxHandle_t * puCxHandle, char * pParams, size_t paramsLength)
{
    (void)paramsLength;
    int32_t conn_handle;
    int32_t value_handle;
    uByteArray_t hex_data;
    int32_t ret = uCxAtUtilParseParamsF(pParams, "ddh", &conn_handle, &value_handle, &hex_data, U_CX_AT_UTIL_PARAM_LAST);
    if ((ret >= 0) && puCxHandle->callbacks.UEBTGCI) {
        puCxHandle->callbacks.UEBTGCI(puCxHandle, conn_handle, value_handle, &hex_data);
    }
    return ret;
}

static int32_t parseUEBTGCW(uCxHandle_t * puCxHandle, char * pParams, size_t paramsLength)
{
    (void)paramsLength;
    int32_t conn_handle;
    int32_t value_handle;
    uByteArray_t value;
    int32_t options;
    int32_t ret = uCxAtUtilParseParamsF(pParams, "ddhd", &conn_handle, &value_handle, &value, &options, U_CX_AT_UTIL_PARAM_LAST);
    if ((ret >= 0) && puCxHandle->callbacks.UEBTGCW) {
        puCxHandle->callbacks.UEBTGCW(puCxHandle, conn_handle, value_handle, &value, options);
    }
    return ret;
}

static int32_t parseUEBTGRR(uCxHandle_t * puCxHandle, char * pParams, size_t paramsLength)
{
    (void)paramsLength;
    int32_t conn_handle;
    int32_t value_handle;
    int32_t ret = uCxAtUtilParseParamsF(pParams, "dd", &conn_handle, &value_handle, U_CX_AT_UTIL_PARAM_LAST);
    if ((ret >= 0) && puCxHandle->callbacks.UEBTGRR) {
        puCxHandle->callbacks.UEBTGRR(puCxHandle, conn_handle, value_handle);
    }
    return ret;
}

static int32_t parseUEBTGIC(uCxHandle_t * puCxHandle, char * pParams, size_t paramsLength)
{
    (void)paramsLength;
    int32_t conn_handle;
    int32_t char_handle;
    int32_t ret = uCxAtUtilParseParamsF(pParams, "dd", &conn_handle, &char_handle, U_CX_AT_UTIL_PARAM_LAST);
    if ((ret >= 0) && puCxHandle->callbacks.UEBTGIC) {
        puCxHandle->callbacks.UEBTGIC(puCxHandle, conn_handle, char_handle);
    }
    return ret;
}

static int32_t parseUESPSC(uCxHandle_t * puCxHandle, char * pParams, size_t paramsLength)
{
    (void)paramsLength;
    int32_t conn_handle;
    int32_t ret = uCxAtUtilParseParamsF(pParams, "d", &conn_handle, U_CX_AT_UTIL_PARAM_LAST);
    if ((ret >= 0) && puCxHandle->callbacks.UESPSC) {
        puCxHandle->callbacks.UESPSC(puCxHandle, conn_handle);
    }
    return ret;
}

static int32_t parseUESPSDC(uCxHandle_t * puCxHandle, char * pParams, size_t paramsLength)
{
    (void)paramsLength;
    int32_t conn_handle;
    int32_t ret = uCxAtUtilParseParamsF(pParams, "d", &conn_handle, U_CX_AT_UTIL_PARAM_LAST);
    if ((ret >= 0) && puCxHandle->callbacks.UESPSDC) {
        puCxHandle->callbacks.UESPSDC(puCxHandle, conn_handle);
    }
    return ret;
}

static int32_t parseUESPSDS(uCxHandle_t * puCxHandle, char * pParams, size_t paramsLength)
{
    (void)paramsLength;
    int32_t conn_handle;
    uByteArray_t string_data;
    int32_t ret = uCxAtUtilParseParamsF(pParams, "d$", &conn_handle, &string_data, U_CX_AT_UTIL_PARAM_LAST);
    if ((ret >= 0) && puCxHandle->callbacks.UESPSDS) {
        puCxHandle->callbacks.UESPSDS(puCxHandle, conn_handle, &string_data);
    }
    return ret;
}

static int32_t parseUESPSDB(uCxHandle_t * puCxHandle, char * pParams, size_t paramsLength)
{
    (void)paramsLength;
    int32_t conn_handle;
    int32_t ret = uCxAtUtilParseParamsF(pParams, "d", &conn_handle, U_CX_AT_UTIL_PARAM_LAST);
    if ((ret >= 0) && puCxHandle->callbacks.UESPSDB) {
        puCxHandle->callbacks.UESPSDB(puCxHandle, conn_handle);
    }
    return ret;
}

static int32_t parseUESPSDA(uCxHandle_t * puCxHandle, char * pParams, size_t paramsLength)
{
    (void)paramsLength;
    int32_t conn_handle;
    int32_t number_bytes;
    int32_t ret = uCxAtUtilParseParamsF(pParams, "dd", &conn_handle, &number_bytes, U_CX_AT_UTIL_PARAM_LAST);
    if ((ret >= 0) && puCxHandle->callbacks.UESPSDA) {
        puCxHandle->callbacks.UESPSDA(puCxHandle, conn_handle, number_bytes);
    }
    return ret;
}

/* ------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * ---------------------------------------------------------- */
int32_t uCxUrcParse(uCxHandle_t * puCxHandle, const char * pUrcName, char * pParams, size_t paramsLength)
{
    if (strcmp(pUrcName, "+STARTUP") == 0) {
        return parseSTARTUP(puCxHandle, pParams, paramsLength);
    }
    if (strcmp(pUrcName, "+UEBTC") == 0) {
        return parseUEBTC(puCxHandle, pParams, paramsLength);
    }
    if (strcmp(pUrcName, "+UEBTDC") == 0) {
        return parseUEBTDC(puCxHandle, pParams, paramsLength);
    }
    if (strcmp(pUrcName, "+UEBTB") == 0) {
        return parseUEBTB(puCxHandle, pParams, paramsLength);
    }
    if (strcmp(pUrcName, "+UEBTUC") == 0) {
        return parseUEBTUC(puCxHandle, pParams, paramsLength);
    }
    if (strcmp(pUrcName, "+UEBTUPD") == 0) {
        return parseUEBTUPD(puCxHandle, pParams, paramsLength);
    }
    if (strcmp(pUrcName, "+UEBTUPE") == 0) {
        return parseUEBTUPE(puCxHandle, pParams, paramsLength);
    }
    if (strcmp(pUrcName, "+UEBTPHYU") == 0) {
        return parseUEBTPHYU(puCxHandle, pParams, paramsLength);
    }
    if (strcmp(pUrcName, "+UEBTBGD") == 0) {
        return parseUEBTBGD(puCxHandle, pParams, paramsLength);
    }
    if (strcmp(pUrcName, "+UEBTGCN") == 0) {
        return parseUEBTGCN(puCxHandle, pParams, paramsLength);
    }
    if (strcmp(pUrcName, "+UEBTGCI") == 0) {
        return parseUEBTGCI(puCxHandle, pParams, paramsLength);
    }
    if (strcmp(pUrcName, "+UEBTGCW") == 0) {
        return parseUEBTGCW(puCxHandle, pParams, paramsLength);
    }
    if (strcmp(pUrcName, "+UEBTGRR") == 0) {
        return parseUEBTGRR(puCxHandle, pParams, paramsLength);
    }
    if (strcmp(pUrcName, "+UEBTGIC") == 0) {
        return parseUEBTGIC(puCxHandle, pParams, paramsLength);
    }
    if (strcmp(pUrcName, "+UESPSC") == 0) {
        return parseUESPSC(puCxHandle, pParams, paramsLength);
    }
    if (strcmp(pUrcName, "+UESPSDC") == 0) {
        return parseUESPSDC(puCxHandle, pParams, paramsLength);
    }
    if (strcmp(pUrcName, "+UESPSDS") == 0) {
        return parseUESPSDS(puCxHandle, pParams, paramsLength);
    }
    if (strcmp(pUrcName, "+UESPSDB") == 0) {
        return parseUESPSDB(puCxHandle, pParams, paramsLength);
    }
    if (strcmp(pUrcName, "+UESPSDA") == 0) {
        return parseUESPSDA(puCxHandle, pParams, paramsLength);
    }
    return -1;
}

