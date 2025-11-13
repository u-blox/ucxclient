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
#include "u_cx_log.h"

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
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "parseUEBTUPE: ret=%d, callback=%p", ret, (void*)puCxHandle->callbacks.UEBTUPE);
    if ((ret >= 0) && puCxHandle->callbacks.UEBTUPE) {
        U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "parseUEBTUPE: Calling callback!");
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

static int32_t parseUEWLU(uCxHandle_t * puCxHandle, char * pParams, size_t paramsLength)
{
    (void)paramsLength;
    int32_t wlan_handle;
    uMacAddress_t bssid;
    int32_t channel;
    int32_t ret = uCxAtUtilParseParamsF(pParams, "dmd", &wlan_handle, &bssid, &channel, U_CX_AT_UTIL_PARAM_LAST);
    if ((ret >= 0) && puCxHandle->callbacks.UEWLU) {
        puCxHandle->callbacks.UEWLU(puCxHandle, wlan_handle, &bssid, channel);
    }
    return ret;
}

static int32_t parseUEWLD(uCxHandle_t * puCxHandle, char * pParams, size_t paramsLength)
{
    (void)paramsLength;
    int32_t wlan_handle;
    int32_t reason;
    int32_t ret = uCxAtUtilParseParamsF(pParams, "dd", &wlan_handle, &reason, U_CX_AT_UTIL_PARAM_LAST);
    if ((ret >= 0) && puCxHandle->callbacks.UEWLD) {
        puCxHandle->callbacks.UEWLD(puCxHandle, wlan_handle, reason);
    }
    return ret;
}

static int32_t parseUEWSNU(uCxHandle_t * puCxHandle, char * pParams, size_t paramsLength)
{
    (void)paramsLength;
    int32_t ret = uCxAtUtilParseParamsF(pParams, "", U_CX_AT_UTIL_PARAM_LAST);
    if ((ret >= 0) && puCxHandle->callbacks.UEWSNU) {
        puCxHandle->callbacks.UEWSNU(puCxHandle);
    }
    return ret;
}

static int32_t parseUEWSND(uCxHandle_t * puCxHandle, char * pParams, size_t paramsLength)
{
    (void)paramsLength;
    int32_t ret = uCxAtUtilParseParamsF(pParams, "", U_CX_AT_UTIL_PARAM_LAST);
    if ((ret >= 0) && puCxHandle->callbacks.UEWSND) {
        puCxHandle->callbacks.UEWSND(puCxHandle);
    }
    return ret;
}

static int32_t parseUEWSRSI(uCxHandle_t * puCxHandle, char * pParams, size_t paramsLength)
{
    (void)paramsLength;
    int32_t ret = uCxAtUtilParseParamsF(pParams, "", U_CX_AT_UTIL_PARAM_LAST);
    if ((ret >= 0) && puCxHandle->callbacks.UEWSRSI) {
        puCxHandle->callbacks.UEWSRSI(puCxHandle);
    }
    return ret;
}

static int32_t parseUEWSRSF(uCxHandle_t * puCxHandle, char * pParams, size_t paramsLength)
{
    (void)paramsLength;
    int32_t ret = uCxAtUtilParseParamsF(pParams, "", U_CX_AT_UTIL_PARAM_LAST);
    if ((ret >= 0) && puCxHandle->callbacks.UEWSRSF) {
        puCxHandle->callbacks.UEWSRSF(puCxHandle);
    }
    return ret;
}

static int32_t parseUEWSRSC(uCxHandle_t * puCxHandle, char * pParams, size_t paramsLength)
{
    (void)paramsLength;
    int32_t wlan_handle;
    uMacAddress_t bssid;
    int32_t channel;
    int32_t ret = uCxAtUtilParseParamsF(pParams, "dmd", &wlan_handle, &bssid, &channel, U_CX_AT_UTIL_PARAM_LAST);
    if ((ret >= 0) && puCxHandle->callbacks.UEWSRSC) {
        puCxHandle->callbacks.UEWSRSC(puCxHandle, wlan_handle, &bssid, channel);
    }
    return ret;
}

static int32_t parseUEWAPNU(uCxHandle_t * puCxHandle, char * pParams, size_t paramsLength)
{
    (void)paramsLength;
    int32_t ret = uCxAtUtilParseParamsF(pParams, "", U_CX_AT_UTIL_PARAM_LAST);
    if ((ret >= 0) && puCxHandle->callbacks.UEWAPNU) {
        puCxHandle->callbacks.UEWAPNU(puCxHandle);
    }
    return ret;
}

static int32_t parseUEWAPND(uCxHandle_t * puCxHandle, char * pParams, size_t paramsLength)
{
    (void)paramsLength;
    int32_t ret = uCxAtUtilParseParamsF(pParams, "", U_CX_AT_UTIL_PARAM_LAST);
    if ((ret >= 0) && puCxHandle->callbacks.UEWAPND) {
        puCxHandle->callbacks.UEWAPND(puCxHandle);
    }
    return ret;
}

static int32_t parseUEWAPU(uCxHandle_t * puCxHandle, char * pParams, size_t paramsLength)
{
    (void)paramsLength;
    int32_t ret = uCxAtUtilParseParamsF(pParams, "", U_CX_AT_UTIL_PARAM_LAST);
    if ((ret >= 0) && puCxHandle->callbacks.UEWAPU) {
        puCxHandle->callbacks.UEWAPU(puCxHandle);
    }
    return ret;
}

static int32_t parseUEWAPD(uCxHandle_t * puCxHandle, char * pParams, size_t paramsLength)
{
    (void)paramsLength;
    int32_t ret = uCxAtUtilParseParamsF(pParams, "", U_CX_AT_UTIL_PARAM_LAST);
    if ((ret >= 0) && puCxHandle->callbacks.UEWAPD) {
        puCxHandle->callbacks.UEWAPD(puCxHandle);
    }
    return ret;
}

static int32_t parseUEWAPSA(uCxHandle_t * puCxHandle, char * pParams, size_t paramsLength)
{
    (void)paramsLength;
    uMacAddress_t mac;
    int32_t ret = uCxAtUtilParseParamsF(pParams, "m", &mac, U_CX_AT_UTIL_PARAM_LAST);
    if ((ret >= 0) && puCxHandle->callbacks.UEWAPSA) {
        puCxHandle->callbacks.UEWAPSA(puCxHandle, &mac);
    }
    return ret;
}

static int32_t parseUEWAPSDA(uCxHandle_t * puCxHandle, char * pParams, size_t paramsLength)
{
    (void)paramsLength;
    uMacAddress_t mac;
    int32_t ret = uCxAtUtilParseParamsF(pParams, "m", &mac, U_CX_AT_UTIL_PARAM_LAST);
    if ((ret >= 0) && puCxHandle->callbacks.UEWAPSDA) {
        puCxHandle->callbacks.UEWAPSDA(puCxHandle, &mac);
    }
    return ret;
}

static int32_t parseUESOC(uCxHandle_t * puCxHandle, char * pParams, size_t paramsLength)
{
    (void)paramsLength;
    int32_t socket_handle;
    int32_t ret = uCxAtUtilParseParamsF(pParams, "d", &socket_handle, U_CX_AT_UTIL_PARAM_LAST);
    if ((ret >= 0) && puCxHandle->callbacks.UESOC) {
        puCxHandle->callbacks.UESOC(puCxHandle, socket_handle);
    }
    return ret;
}

static int32_t parseUESODA(uCxHandle_t * puCxHandle, char * pParams, size_t paramsLength)
{
    (void)paramsLength;
    int32_t socket_handle;
    int32_t number_bytes;
    int32_t ret = uCxAtUtilParseParamsF(pParams, "dd", &socket_handle, &number_bytes, U_CX_AT_UTIL_PARAM_LAST);
    if ((ret >= 0) && puCxHandle->callbacks.UESODA) {
        puCxHandle->callbacks.UESODA(puCxHandle, socket_handle, number_bytes);
    }
    return ret;
}

static int32_t parseUESODS(uCxHandle_t * puCxHandle, char * pParams, size_t paramsLength)
{
    (void)paramsLength;
    int32_t socket_handle;
    uByteArray_t string_data;
    int32_t ret = uCxAtUtilParseParamsF(pParams, "d$", &socket_handle, &string_data, U_CX_AT_UTIL_PARAM_LAST);
    if ((ret >= 0) && puCxHandle->callbacks.UESODS) {
        puCxHandle->callbacks.UESODS(puCxHandle, socket_handle, &string_data);
    }
    return ret;
}

static int32_t parseUESODSF(uCxHandle_t * puCxHandle, char * pParams, size_t paramsLength)
{
    (void)paramsLength;
    int32_t socket_handle;
    uSockIpAddress_t remote_ip;
    int32_t remote_port;
    uByteArray_t string_data;
    int32_t ret = uCxAtUtilParseParamsF(pParams, "did$", &socket_handle, &remote_ip, &remote_port, &string_data, U_CX_AT_UTIL_PARAM_LAST);
    if ((ret >= 0) && puCxHandle->callbacks.UESODSF) {
        puCxHandle->callbacks.UESODSF(puCxHandle, socket_handle, &remote_ip, remote_port, &string_data);
    }
    return ret;
}

static int32_t parseUESODB(uCxHandle_t * puCxHandle, char * pParams, size_t paramsLength)
{
    (void)paramsLength;
    int32_t socket_handle;
    int32_t ret = uCxAtUtilParseParamsF(pParams, "d", &socket_handle, U_CX_AT_UTIL_PARAM_LAST);
    if ((ret >= 0) && puCxHandle->callbacks.UESODB) {
        puCxHandle->callbacks.UESODB(puCxHandle, socket_handle);
    }
    return ret;
}

static int32_t parseUESODBF(uCxHandle_t * puCxHandle, char * pParams, size_t paramsLength)
{
    (void)paramsLength;
    int32_t socket_handle;
    uSockIpAddress_t remote_ip;
    int32_t remote_port;
    int32_t ret = uCxAtUtilParseParamsF(pParams, "did", &socket_handle, &remote_ip, &remote_port, U_CX_AT_UTIL_PARAM_LAST);
    if ((ret >= 0) && puCxHandle->callbacks.UESODBF) {
        puCxHandle->callbacks.UESODBF(puCxHandle, socket_handle, &remote_ip, remote_port);
    }
    return ret;
}

static int32_t parseUESOCL(uCxHandle_t * puCxHandle, char * pParams, size_t paramsLength)
{
    (void)paramsLength;
    int32_t socket_handle;
    int32_t ret = uCxAtUtilParseParamsF(pParams, "d", &socket_handle, U_CX_AT_UTIL_PARAM_LAST);
    if ((ret >= 0) && puCxHandle->callbacks.UESOCL) {
        puCxHandle->callbacks.UESOCL(puCxHandle, socket_handle);
    }
    return ret;
}

static int32_t parseUESOIC(uCxHandle_t * puCxHandle, char * pParams, size_t paramsLength)
{
    (void)paramsLength;
    int32_t socket_handle;
    uSockIpAddress_t remote_ip;
    int32_t listening_socket_handle;
    int32_t ret = uCxAtUtilParseParamsF(pParams, "did", &socket_handle, &remote_ip, &listening_socket_handle, U_CX_AT_UTIL_PARAM_LAST);
    if ((ret >= 0) && puCxHandle->callbacks.UESOIC) {
        puCxHandle->callbacks.UESOIC(puCxHandle, socket_handle, &remote_ip, listening_socket_handle);
    }
    return ret;
}

static int32_t parseUEMQC(uCxHandle_t * puCxHandle, char * pParams, size_t paramsLength)
{
    (void)paramsLength;
    int32_t mqtt_id;
    int32_t ret = uCxAtUtilParseParamsF(pParams, "d", &mqtt_id, U_CX_AT_UTIL_PARAM_LAST);
    if ((ret >= 0) && puCxHandle->callbacks.UEMQC) {
        puCxHandle->callbacks.UEMQC(puCxHandle, mqtt_id);
    }
    return ret;
}

static int32_t parseUEMQDC(uCxHandle_t * puCxHandle, char * pParams, size_t paramsLength)
{
    (void)paramsLength;
    int32_t mqtt_id;
    int32_t disconnect_reason;
    int32_t ret = uCxAtUtilParseParamsF(pParams, "dd", &mqtt_id, &disconnect_reason, U_CX_AT_UTIL_PARAM_LAST);
    if ((ret >= 0) && puCxHandle->callbacks.UEMQDC) {
        puCxHandle->callbacks.UEMQDC(puCxHandle, mqtt_id, disconnect_reason);
    }
    return ret;
}

static int32_t parseUEMQDA(uCxHandle_t * puCxHandle, char * pParams, size_t paramsLength)
{
    (void)paramsLength;
    int32_t mqtt_id;
    int32_t message_len;
    int32_t ret = uCxAtUtilParseParamsF(pParams, "dd", &mqtt_id, &message_len, U_CX_AT_UTIL_PARAM_LAST);
    if ((ret >= 0) && puCxHandle->callbacks.UEMQDA) {
        puCxHandle->callbacks.UEMQDA(puCxHandle, mqtt_id, message_len);
    }
    return ret;
}

static int32_t parseUEMQDD(uCxHandle_t * puCxHandle, char * pParams, size_t paramsLength)
{
    (void)paramsLength;
    int32_t mqtt_id;
    int32_t message_len;
    int32_t ret = uCxAtUtilParseParamsF(pParams, "dd", &mqtt_id, &message_len, U_CX_AT_UTIL_PARAM_LAST);
    if ((ret >= 0) && puCxHandle->callbacks.UEMQDD) {
        puCxHandle->callbacks.UEMQDD(puCxHandle, mqtt_id, message_len);
    }
    return ret;
}

static int32_t parseUEMQPC(uCxHandle_t * puCxHandle, char * pParams, size_t paramsLength)
{
    (void)paramsLength;
    int32_t mqtt_id;
    int32_t packet_id;
    int32_t message_len;
    int32_t ret = uCxAtUtilParseParamsF(pParams, "ddd", &mqtt_id, &packet_id, &message_len, U_CX_AT_UTIL_PARAM_LAST);
    if ((ret >= 0) && puCxHandle->callbacks.UEMQPC) {
        puCxHandle->callbacks.UEMQPC(puCxHandle, mqtt_id, packet_id, message_len);
    }
    return ret;
}

static int32_t parseUEMQSC(uCxHandle_t * puCxHandle, char * pParams, size_t paramsLength)
{
    (void)paramsLength;
    int32_t mqtt_id;
    int32_t subscribe_action;
    int32_t ret = uCxAtUtilParseParamsF(pParams, "dd", &mqtt_id, &subscribe_action, U_CX_AT_UTIL_PARAM_LAST);
    if ((ret >= 0) && puCxHandle->callbacks.UEMQSC) {
        puCxHandle->callbacks.UEMQSC(puCxHandle, mqtt_id, subscribe_action);
    }
    return ret;
}

static int32_t parseUEHTCDC(uCxHandle_t * puCxHandle, char * pParams, size_t paramsLength)
{
    (void)paramsLength;
    int32_t session_id;
    int32_t ret = uCxAtUtilParseParamsF(pParams, "d", &session_id, U_CX_AT_UTIL_PARAM_LAST);
    if ((ret >= 0) && puCxHandle->callbacks.UEHTCDC) {
        puCxHandle->callbacks.UEHTCDC(puCxHandle, session_id);
    }
    return ret;
}

static int32_t parseUEHTCRS(uCxHandle_t * puCxHandle, char * pParams, size_t paramsLength)
{
    (void)paramsLength;
    int32_t session_id;
    int32_t status_code;
    const char * description;
    int32_t ret = uCxAtUtilParseParamsF(pParams, "dds", &session_id, &status_code, &description, U_CX_AT_UTIL_PARAM_LAST);
    if ((ret >= 0) && puCxHandle->callbacks.UEHTCRS) {
        puCxHandle->callbacks.UEHTCRS(puCxHandle, session_id, status_code, description);
    }
    return ret;
}

static int32_t parseUEDGPC(uCxHandle_t * puCxHandle, char * pParams, size_t paramsLength)
{
    (void)paramsLength;
    int32_t transmitted_packets;
    int32_t received_packets;
    int32_t packet_loss_rate;
    int32_t avg_response_time;
    int32_t ret = uCxAtUtilParseParamsF(pParams, "dddd", &transmitted_packets, &received_packets, &packet_loss_rate, &avg_response_time, U_CX_AT_UTIL_PARAM_LAST);
    if ((ret >= 0) && puCxHandle->callbacks.UEDGPC) {
        puCxHandle->callbacks.UEDGPC(puCxHandle, transmitted_packets, received_packets, packet_loss_rate, avg_response_time);
    }
    return ret;
}

static int32_t parseUEDGP(uCxHandle_t * puCxHandle, char * pParams, size_t paramsLength)
{
    (void)paramsLength;
    int32_t ping_response;
    int32_t response_time;
    int32_t ret = uCxAtUtilParseParamsF(pParams, "dd", &ping_response, &response_time, U_CX_AT_UTIL_PARAM_LAST);
    if ((ret >= 0) && puCxHandle->callbacks.UEDGP) {
        puCxHandle->callbacks.UEDGP(puCxHandle, ping_response, response_time);
    }
    return ret;
}

static int32_t parseUEDGI(uCxHandle_t * puCxHandle, char * pParams, size_t paramsLength)
{
    (void)paramsLength;
    const char * iperf_output;
    int32_t ret = uCxAtUtilParseParamsF(pParams, "s", &iperf_output, U_CX_AT_UTIL_PARAM_LAST);
    if ((ret >= 0) && puCxHandle->callbacks.UEDGI) {
        puCxHandle->callbacks.UEDGI(puCxHandle, iperf_output);
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
    if (strcmp(pUrcName, "+UEWLU") == 0) {
        return parseUEWLU(puCxHandle, pParams, paramsLength);
    }
    if (strcmp(pUrcName, "+UEWLD") == 0) {
        return parseUEWLD(puCxHandle, pParams, paramsLength);
    }
    if (strcmp(pUrcName, "+UEWSNU") == 0) {
        return parseUEWSNU(puCxHandle, pParams, paramsLength);
    }
    if (strcmp(pUrcName, "+UEWSND") == 0) {
        return parseUEWSND(puCxHandle, pParams, paramsLength);
    }
    if (strcmp(pUrcName, "+UEWSRSI") == 0) {
        return parseUEWSRSI(puCxHandle, pParams, paramsLength);
    }
    if (strcmp(pUrcName, "+UEWSRSF") == 0) {
        return parseUEWSRSF(puCxHandle, pParams, paramsLength);
    }
    if (strcmp(pUrcName, "+UEWSRSC") == 0) {
        return parseUEWSRSC(puCxHandle, pParams, paramsLength);
    }
    if (strcmp(pUrcName, "+UEWAPNU") == 0) {
        return parseUEWAPNU(puCxHandle, pParams, paramsLength);
    }
    if (strcmp(pUrcName, "+UEWAPND") == 0) {
        return parseUEWAPND(puCxHandle, pParams, paramsLength);
    }
    if (strcmp(pUrcName, "+UEWAPU") == 0) {
        return parseUEWAPU(puCxHandle, pParams, paramsLength);
    }
    if (strcmp(pUrcName, "+UEWAPD") == 0) {
        return parseUEWAPD(puCxHandle, pParams, paramsLength);
    }
    if (strcmp(pUrcName, "+UEWAPSA") == 0) {
        return parseUEWAPSA(puCxHandle, pParams, paramsLength);
    }
    if (strcmp(pUrcName, "+UEWAPSDA") == 0) {
        return parseUEWAPSDA(puCxHandle, pParams, paramsLength);
    }
    if (strcmp(pUrcName, "+UESOC") == 0) {
        return parseUESOC(puCxHandle, pParams, paramsLength);
    }
    if (strcmp(pUrcName, "+UESODA") == 0) {
        return parseUESODA(puCxHandle, pParams, paramsLength);
    }
    if (strcmp(pUrcName, "+UESODS") == 0) {
        return parseUESODS(puCxHandle, pParams, paramsLength);
    }
    if (strcmp(pUrcName, "+UESODSF") == 0) {
        return parseUESODSF(puCxHandle, pParams, paramsLength);
    }
    if (strcmp(pUrcName, "+UESODB") == 0) {
        return parseUESODB(puCxHandle, pParams, paramsLength);
    }
    if (strcmp(pUrcName, "+UESODBF") == 0) {
        return parseUESODBF(puCxHandle, pParams, paramsLength);
    }
    if (strcmp(pUrcName, "+UESOCL") == 0) {
        return parseUESOCL(puCxHandle, pParams, paramsLength);
    }
    if (strcmp(pUrcName, "+UESOIC") == 0) {
        return parseUESOIC(puCxHandle, pParams, paramsLength);
    }
    if (strcmp(pUrcName, "+UEMQC") == 0) {
        return parseUEMQC(puCxHandle, pParams, paramsLength);
    }
    if (strcmp(pUrcName, "+UEMQDC") == 0) {
        return parseUEMQDC(puCxHandle, pParams, paramsLength);
    }
    if (strcmp(pUrcName, "+UEMQDA") == 0) {
        return parseUEMQDA(puCxHandle, pParams, paramsLength);
    }
    if (strcmp(pUrcName, "+UEMQDD") == 0) {
        return parseUEMQDD(puCxHandle, pParams, paramsLength);
    }
    if (strcmp(pUrcName, "+UEMQPC") == 0) {
        return parseUEMQPC(puCxHandle, pParams, paramsLength);
    }
    if (strcmp(pUrcName, "+UEMQSC") == 0) {
        return parseUEMQSC(puCxHandle, pParams, paramsLength);
    }
    if (strcmp(pUrcName, "+UEHTCDC") == 0) {
        return parseUEHTCDC(puCxHandle, pParams, paramsLength);
    }
    if (strcmp(pUrcName, "+UEHTCRS") == 0) {
        return parseUEHTCRS(puCxHandle, pParams, paramsLength);
    }
    if (strcmp(pUrcName, "+UEDGPC") == 0) {
        return parseUEDGPC(puCxHandle, pParams, paramsLength);
    }
    if (strcmp(pUrcName, "+UEDGP") == 0) {
        return parseUEDGP(puCxHandle, pParams, paramsLength);
    }
    if (strcmp(pUrcName, "+UEDGI") == 0) {
        return parseUEDGI(puCxHandle, pParams, paramsLength);
    }
    return -1;
}

