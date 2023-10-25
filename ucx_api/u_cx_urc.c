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

static int32_t parseUEBTC(uCxHandle_t * puCxHandle, char * pParams, size_t paramsLength)
{
    int32_t conn_handle;
    uBtLeAddress_t bd_addr;
    int32_t ret = 0;
    ret = uCxAtUtilParseParamsF(pParams, "db", &conn_handle, &bd_addr, U_CX_AT_UTIL_PARAM_LAST);
    if ((ret >= 0) && puCxHandle->callbacks.UEBTC) {
        puCxHandle->callbacks.UEBTC(puCxHandle, conn_handle, &bd_addr);
    }
    return ret;
}

static int32_t parseUEBTDC(uCxHandle_t * puCxHandle, char * pParams, size_t paramsLength)
{
    int32_t conn_handle;
    int32_t ret = 0;
    ret = uCxAtUtilParseParamsF(pParams, "d", &conn_handle, U_CX_AT_UTIL_PARAM_LAST);
    if ((ret >= 0) && puCxHandle->callbacks.UEBTDC) {
        puCxHandle->callbacks.UEBTDC(puCxHandle, conn_handle);
    }
    return ret;
}

static int32_t parseUEBTB(uCxHandle_t * puCxHandle, char * pParams, size_t paramsLength)
{
    uBtLeAddress_t bd_addr;
    int32_t bond_status;
    int32_t ret = 0;
    ret = uCxAtUtilParseParamsF(pParams, "bd", &bd_addr, &bond_status, U_CX_AT_UTIL_PARAM_LAST);
    if ((ret >= 0) && puCxHandle->callbacks.UEBTB) {
        puCxHandle->callbacks.UEBTB(puCxHandle, &bd_addr, bond_status);
    }
    return ret;
}

static int32_t parseUEBTUC(uCxHandle_t * puCxHandle, char * pParams, size_t paramsLength)
{
    uBtLeAddress_t bd_addr;
    int32_t numeric_value;
    int32_t ret = 0;
    ret = uCxAtUtilParseParamsF(pParams, "bd", &bd_addr, &numeric_value, U_CX_AT_UTIL_PARAM_LAST);
    if ((ret >= 0) && puCxHandle->callbacks.UEBTUC) {
        puCxHandle->callbacks.UEBTUC(puCxHandle, &bd_addr, numeric_value);
    }
    return ret;
}

static int32_t parseUEBTUPD(uCxHandle_t * puCxHandle, char * pParams, size_t paramsLength)
{
    uBtLeAddress_t bd_addr;
    int32_t numeric_value;
    int32_t ret = 0;
    ret = uCxAtUtilParseParamsF(pParams, "bd", &bd_addr, &numeric_value, U_CX_AT_UTIL_PARAM_LAST);
    if ((ret >= 0) && puCxHandle->callbacks.UEBTUPD) {
        puCxHandle->callbacks.UEBTUPD(puCxHandle, &bd_addr, numeric_value);
    }
    return ret;
}

static int32_t parseUEBTUPE(uCxHandle_t * puCxHandle, char * pParams, size_t paramsLength)
{
    uBtLeAddress_t bd_addr;
    int32_t ret = 0;
    ret = uCxAtUtilParseParamsF(pParams, "b", &bd_addr, U_CX_AT_UTIL_PARAM_LAST);
    if ((ret >= 0) && puCxHandle->callbacks.UEBTUPE) {
        puCxHandle->callbacks.UEBTUPE(puCxHandle, &bd_addr);
    }
    return ret;
}

static int32_t parseUEBTPHYU(uCxHandle_t * puCxHandle, char * pParams, size_t paramsLength)
{
    int32_t conn_handle;
    int32_t phy_status;
    int32_t tx_phy;
    int32_t rx_phy;
    int32_t ret = 0;
    ret = uCxAtUtilParseParamsF(pParams, "dddd", &conn_handle, &phy_status, &tx_phy, &rx_phy, U_CX_AT_UTIL_PARAM_LAST);
    if ((ret >= 0) && puCxHandle->callbacks.UEBTPHYU) {
        puCxHandle->callbacks.UEBTPHYU(puCxHandle, conn_handle, phy_status, tx_phy, rx_phy);
    }
    return ret;
}

static int32_t parseUEBTGCN(uCxHandle_t * puCxHandle, char * pParams, size_t paramsLength)
{
    int32_t conn_handle;
    int32_t value_handle;
    uByteArray_t hex_data;
    int32_t ret = 0;
    ret = uCxAtUtilParseParamsF(pParams, "ddh", &conn_handle, &value_handle, &hex_data, U_CX_AT_UTIL_PARAM_LAST);
    if ((ret >= 0) && puCxHandle->callbacks.UEBTGCN) {
        puCxHandle->callbacks.UEBTGCN(puCxHandle, conn_handle, value_handle, &hex_data);
    }
    return ret;
}

static int32_t parseUEBTGCI(uCxHandle_t * puCxHandle, char * pParams, size_t paramsLength)
{
    int32_t conn_handle;
    int32_t value_handle;
    uByteArray_t hex_data;
    int32_t ret = 0;
    ret = uCxAtUtilParseParamsF(pParams, "ddh", &conn_handle, &value_handle, &hex_data, U_CX_AT_UTIL_PARAM_LAST);
    if ((ret >= 0) && puCxHandle->callbacks.UEBTGCI) {
        puCxHandle->callbacks.UEBTGCI(puCxHandle, conn_handle, value_handle, &hex_data);
    }
    return ret;
}

static int32_t parseUEBTGCW(uCxHandle_t * puCxHandle, char * pParams, size_t paramsLength)
{
    int32_t conn_handle;
    int32_t value_handle;
    uByteArray_t value;
    int32_t options;
    int32_t ret = 0;
    ret = uCxAtUtilParseParamsF(pParams, "ddhd", &conn_handle, &value_handle, &value, &options, U_CX_AT_UTIL_PARAM_LAST);
    if ((ret >= 0) && puCxHandle->callbacks.UEBTGCW) {
        puCxHandle->callbacks.UEBTGCW(puCxHandle, conn_handle, value_handle, &value, options);
    }
    return ret;
}

static int32_t parseUEBTGRR(uCxHandle_t * puCxHandle, char * pParams, size_t paramsLength)
{
    int32_t conn_handle;
    int32_t value_handle;
    int32_t ret = 0;
    ret = uCxAtUtilParseParamsF(pParams, "dd", &conn_handle, &value_handle, U_CX_AT_UTIL_PARAM_LAST);
    if ((ret >= 0) && puCxHandle->callbacks.UEBTGRR) {
        puCxHandle->callbacks.UEBTGRR(puCxHandle, conn_handle, value_handle);
    }
    return ret;
}

static int32_t parseUEBTGIC(uCxHandle_t * puCxHandle, char * pParams, size_t paramsLength)
{
    int32_t conn_handle;
    int32_t char_handle;
    int32_t ret = 0;
    ret = uCxAtUtilParseParamsF(pParams, "dd", &conn_handle, &char_handle, U_CX_AT_UTIL_PARAM_LAST);
    if ((ret >= 0) && puCxHandle->callbacks.UEBTGIC) {
        puCxHandle->callbacks.UEBTGIC(puCxHandle, conn_handle, char_handle);
    }
    return ret;
}

static int32_t parseUESPSC(uCxHandle_t * puCxHandle, char * pParams, size_t paramsLength)
{
    int32_t conn_handle;
    int32_t ret = 0;
    ret = uCxAtUtilParseParamsF(pParams, "d", &conn_handle, U_CX_AT_UTIL_PARAM_LAST);
    if ((ret >= 0) && puCxHandle->callbacks.UESPSC) {
        puCxHandle->callbacks.UESPSC(puCxHandle, conn_handle);
    }
    return ret;
}

static int32_t parseUESPSDC(uCxHandle_t * puCxHandle, char * pParams, size_t paramsLength)
{
    int32_t conn_handle;
    int32_t ret = 0;
    ret = uCxAtUtilParseParamsF(pParams, "d", &conn_handle, U_CX_AT_UTIL_PARAM_LAST);
    if ((ret >= 0) && puCxHandle->callbacks.UESPSDC) {
        puCxHandle->callbacks.UESPSDC(puCxHandle, conn_handle);
    }
    return ret;
}

static int32_t parseUESPSDS(uCxHandle_t * puCxHandle, char * pParams, size_t paramsLength)
{
    int32_t conn_handle;
    const char * string_data;
    int32_t ret = 0;
    ret = uCxAtUtilParseParamsF(pParams, "ds", &conn_handle, &string_data, U_CX_AT_UTIL_PARAM_LAST);
    if ((ret >= 0) && puCxHandle->callbacks.UESPSDS) {
        puCxHandle->callbacks.UESPSDS(puCxHandle, conn_handle, string_data);
    }
    return ret;
}

static int32_t parseUESPSDB(uCxHandle_t * puCxHandle, char * pParams, size_t paramsLength)
{
    int32_t conn_handle;
    int32_t ret = 0;
    ret = uCxAtUtilParseParamsF(pParams, "d", &conn_handle, U_CX_AT_UTIL_PARAM_LAST);
    if ((ret >= 0) && puCxHandle->callbacks.UESPSDB) {
        puCxHandle->callbacks.UESPSDB(puCxHandle, conn_handle);
    }
    return ret;
}

static int32_t parseUESPSDA(uCxHandle_t * puCxHandle, char * pParams, size_t paramsLength)
{
    int32_t conn_handle;
    int32_t number_bytes;
    int32_t ret = 0;
    ret = uCxAtUtilParseParamsF(pParams, "dd", &conn_handle, &number_bytes, U_CX_AT_UTIL_PARAM_LAST);
    if ((ret >= 0) && puCxHandle->callbacks.UESPSDA) {
        puCxHandle->callbacks.UESPSDA(puCxHandle, conn_handle, number_bytes);
    }
    return ret;
}

static int32_t parseUEWLU(uCxHandle_t * puCxHandle, char * pParams, size_t paramsLength)
{
    int32_t wlan_handle;
    uMacAddress_t bssid;
    int32_t channel;
    int32_t ret = 0;
    ret = uCxAtUtilParseParamsF(pParams, "dmd", &wlan_handle, &bssid, &channel, U_CX_AT_UTIL_PARAM_LAST);
    if ((ret >= 0) && puCxHandle->callbacks.UEWLU) {
        puCxHandle->callbacks.UEWLU(puCxHandle, wlan_handle, &bssid, channel);
    }
    return ret;
}

static int32_t parseUEWLD(uCxHandle_t * puCxHandle, char * pParams, size_t paramsLength)
{
    int32_t wlan_handle;
    int32_t reason;
    int32_t ret = 0;
    ret = uCxAtUtilParseParamsF(pParams, "dd", &wlan_handle, &reason, U_CX_AT_UTIL_PARAM_LAST);
    if ((ret >= 0) && puCxHandle->callbacks.UEWLD) {
        puCxHandle->callbacks.UEWLD(puCxHandle, wlan_handle, reason);
    }
    return ret;
}

static int32_t parseUEWSNU(uCxHandle_t * puCxHandle, char * pParams, size_t paramsLength)
{
    int32_t ret = 0;
    ret = uCxAtUtilParseParamsF(pParams, "", U_CX_AT_UTIL_PARAM_LAST);
    if ((ret >= 0) && puCxHandle->callbacks.UEWSNU) {
        puCxHandle->callbacks.UEWSNU(puCxHandle);
    }
    return ret;
}

static int32_t parseUEWSND(uCxHandle_t * puCxHandle, char * pParams, size_t paramsLength)
{
    int32_t ret = 0;
    ret = uCxAtUtilParseParamsF(pParams, "", U_CX_AT_UTIL_PARAM_LAST);
    if ((ret >= 0) && puCxHandle->callbacks.UEWSND) {
        puCxHandle->callbacks.UEWSND(puCxHandle);
    }
    return ret;
}

static int32_t parseUEWAPNU(uCxHandle_t * puCxHandle, char * pParams, size_t paramsLength)
{
    int32_t ret = 0;
    ret = uCxAtUtilParseParamsF(pParams, "", U_CX_AT_UTIL_PARAM_LAST);
    if ((ret >= 0) && puCxHandle->callbacks.UEWAPNU) {
        puCxHandle->callbacks.UEWAPNU(puCxHandle);
    }
    return ret;
}

static int32_t parseUEWAPND(uCxHandle_t * puCxHandle, char * pParams, size_t paramsLength)
{
    int32_t ret = 0;
    ret = uCxAtUtilParseParamsF(pParams, "", U_CX_AT_UTIL_PARAM_LAST);
    if ((ret >= 0) && puCxHandle->callbacks.UEWAPND) {
        puCxHandle->callbacks.UEWAPND(puCxHandle);
    }
    return ret;
}

static int32_t parseUEWAPU(uCxHandle_t * puCxHandle, char * pParams, size_t paramsLength)
{
    int32_t ret = 0;
    ret = uCxAtUtilParseParamsF(pParams, "", U_CX_AT_UTIL_PARAM_LAST);
    if ((ret >= 0) && puCxHandle->callbacks.UEWAPU) {
        puCxHandle->callbacks.UEWAPU(puCxHandle);
    }
    return ret;
}

static int32_t parseUEWAPD(uCxHandle_t * puCxHandle, char * pParams, size_t paramsLength)
{
    int32_t ret = 0;
    ret = uCxAtUtilParseParamsF(pParams, "", U_CX_AT_UTIL_PARAM_LAST);
    if ((ret >= 0) && puCxHandle->callbacks.UEWAPD) {
        puCxHandle->callbacks.UEWAPD(puCxHandle);
    }
    return ret;
}

static int32_t parseUEWAPSA(uCxHandle_t * puCxHandle, char * pParams, size_t paramsLength)
{
    uMacAddress_t mac;
    int32_t ret = 0;
    ret = uCxAtUtilParseParamsF(pParams, "m", &mac, U_CX_AT_UTIL_PARAM_LAST);
    if ((ret >= 0) && puCxHandle->callbacks.UEWAPSA) {
        puCxHandle->callbacks.UEWAPSA(puCxHandle, &mac);
    }
    return ret;
}

static int32_t parseUEWAPSDA(uCxHandle_t * puCxHandle, char * pParams, size_t paramsLength)
{
    uMacAddress_t mac;
    int32_t ret = 0;
    ret = uCxAtUtilParseParamsF(pParams, "m", &mac, U_CX_AT_UTIL_PARAM_LAST);
    if ((ret >= 0) && puCxHandle->callbacks.UEWAPSDA) {
        puCxHandle->callbacks.UEWAPSDA(puCxHandle, &mac);
    }
    return ret;
}

static int32_t parseUESOC(uCxHandle_t * puCxHandle, char * pParams, size_t paramsLength)
{
    int32_t socket_handle;
    int32_t ret = 0;
    ret = uCxAtUtilParseParamsF(pParams, "d", &socket_handle, U_CX_AT_UTIL_PARAM_LAST);
    if ((ret >= 0) && puCxHandle->callbacks.UESOC) {
        puCxHandle->callbacks.UESOC(puCxHandle, socket_handle);
    }
    return ret;
}

static int32_t parseUESODA(uCxHandle_t * puCxHandle, char * pParams, size_t paramsLength)
{
    int32_t socket_handle;
    int32_t number_bytes;
    int32_t ret = 0;
    ret = uCxAtUtilParseParamsF(pParams, "dd", &socket_handle, &number_bytes, U_CX_AT_UTIL_PARAM_LAST);
    if ((ret >= 0) && puCxHandle->callbacks.UESODA) {
        puCxHandle->callbacks.UESODA(puCxHandle, socket_handle, number_bytes);
    }
    return ret;
}

static int32_t parseUESODS(uCxHandle_t * puCxHandle, char * pParams, size_t paramsLength)
{
    int32_t socket_handle;
    const char * string_data;
    int32_t ret = 0;
    ret = uCxAtUtilParseParamsF(pParams, "ds", &socket_handle, &string_data, U_CX_AT_UTIL_PARAM_LAST);
    if ((ret >= 0) && puCxHandle->callbacks.UESODS) {
        puCxHandle->callbacks.UESODS(puCxHandle, socket_handle, string_data);
    }
    return ret;
}

static int32_t parseUESODSF(uCxHandle_t * puCxHandle, char * pParams, size_t paramsLength)
{
    int32_t socket_handle;
    uSockIpAddress_t remote_ip;
    int32_t remote_port;
    const char * string_data;
    int32_t ret = 0;
    ret = uCxAtUtilParseParamsF(pParams, "dids", &socket_handle, &remote_ip, &remote_port, &string_data, U_CX_AT_UTIL_PARAM_LAST);
    if ((ret >= 0) && puCxHandle->callbacks.UESODSF) {
        puCxHandle->callbacks.UESODSF(puCxHandle, socket_handle, &remote_ip, remote_port, string_data);
    }
    return ret;
}

static int32_t parseUESODB(uCxHandle_t * puCxHandle, char * pParams, size_t paramsLength)
{
    int32_t socket_handle;
    int32_t ret = 0;
    ret = uCxAtUtilParseParamsF(pParams, "d", &socket_handle, U_CX_AT_UTIL_PARAM_LAST);
    if ((ret >= 0) && puCxHandle->callbacks.UESODB) {
        puCxHandle->callbacks.UESODB(puCxHandle, socket_handle);
    }
    return ret;
}

static int32_t parseUESODBF(uCxHandle_t * puCxHandle, char * pParams, size_t paramsLength)
{
    int32_t socket_handle;
    uSockIpAddress_t remote_ip;
    int32_t remote_port;
    int32_t ret = 0;
    ret = uCxAtUtilParseParamsF(pParams, "did", &socket_handle, &remote_ip, &remote_port, U_CX_AT_UTIL_PARAM_LAST);
    if ((ret >= 0) && puCxHandle->callbacks.UESODBF) {
        puCxHandle->callbacks.UESODBF(puCxHandle, socket_handle, &remote_ip, remote_port);
    }
    return ret;
}

static int32_t parseUESOCL(uCxHandle_t * puCxHandle, char * pParams, size_t paramsLength)
{
    int32_t socket_handle;
    int32_t ret = 0;
    ret = uCxAtUtilParseParamsF(pParams, "d", &socket_handle, U_CX_AT_UTIL_PARAM_LAST);
    if ((ret >= 0) && puCxHandle->callbacks.UESOCL) {
        puCxHandle->callbacks.UESOCL(puCxHandle, socket_handle);
    }
    return ret;
}

static int32_t parseUESOIC(uCxHandle_t * puCxHandle, char * pParams, size_t paramsLength)
{
    int32_t socket_handle;
    uSockIpAddress_t remote_ip;
    int32_t listening_socket_handle;
    int32_t ret = 0;
    ret = uCxAtUtilParseParamsF(pParams, "did", &socket_handle, &remote_ip, &listening_socket_handle, U_CX_AT_UTIL_PARAM_LAST);
    if ((ret >= 0) && puCxHandle->callbacks.UESOIC) {
        puCxHandle->callbacks.UESOIC(puCxHandle, socket_handle, &remote_ip, listening_socket_handle);
    }
    return ret;
}

static int32_t parseUEMQC(uCxHandle_t * puCxHandle, char * pParams, size_t paramsLength)
{
    int32_t mqtt_id;
    int32_t ret = 0;
    ret = uCxAtUtilParseParamsF(pParams, "d", &mqtt_id, U_CX_AT_UTIL_PARAM_LAST);
    if ((ret >= 0) && puCxHandle->callbacks.UEMQC) {
        puCxHandle->callbacks.UEMQC(puCxHandle, mqtt_id);
    }
    return ret;
}

static int32_t parseUEMQDC(uCxHandle_t * puCxHandle, char * pParams, size_t paramsLength)
{
    int32_t mqtt_id;
    int32_t disconnect_reason;
    int32_t ret = 0;
    ret = uCxAtUtilParseParamsF(pParams, "dd", &mqtt_id, &disconnect_reason, U_CX_AT_UTIL_PARAM_LAST);
    if ((ret >= 0) && puCxHandle->callbacks.UEMQDC) {
        puCxHandle->callbacks.UEMQDC(puCxHandle, mqtt_id, disconnect_reason);
    }
    return ret;
}

static int32_t parseUEMQDA(uCxHandle_t * puCxHandle, char * pParams, size_t paramsLength)
{
    int32_t mqtt_id;
    int32_t message_len;
    int32_t ret = 0;
    ret = uCxAtUtilParseParamsF(pParams, "dd", &mqtt_id, &message_len, U_CX_AT_UTIL_PARAM_LAST);
    if ((ret >= 0) && puCxHandle->callbacks.UEMQDA) {
        puCxHandle->callbacks.UEMQDA(puCxHandle, mqtt_id, message_len);
    }
    return ret;
}

static int32_t parseUEDGPC(uCxHandle_t * puCxHandle, char * pParams, size_t paramsLength)
{
    int32_t transmitted_packets;
    int32_t received_packets;
    int32_t packet_loss_rate;
    int32_t avg_response_time;
    int32_t ret = 0;
    ret = uCxAtUtilParseParamsF(pParams, "dddd", &transmitted_packets, &received_packets, &packet_loss_rate, &avg_response_time, U_CX_AT_UTIL_PARAM_LAST);
    if ((ret >= 0) && puCxHandle->callbacks.UEDGPC) {
        puCxHandle->callbacks.UEDGPC(puCxHandle, transmitted_packets, received_packets, packet_loss_rate, avg_response_time);
    }
    return ret;
}

static int32_t parseUEDGP(uCxHandle_t * puCxHandle, char * pParams, size_t paramsLength)
{
    int32_t ping_response;
    int32_t response_time;
    int32_t ret = 0;
    ret = uCxAtUtilParseParamsF(pParams, "dd", &ping_response, &response_time, U_CX_AT_UTIL_PARAM_LAST);
    if ((ret >= 0) && puCxHandle->callbacks.UEDGP) {
        puCxHandle->callbacks.UEDGP(puCxHandle, ping_response, response_time);
    }
    return ret;
}

static int32_t parseUEDGI(uCxHandle_t * puCxHandle, char * pParams, size_t paramsLength)
{
    const char * iperf_output;
    int32_t ret = 0;
    ret = uCxAtUtilParseParamsF(pParams, "s", &iperf_output, U_CX_AT_UTIL_PARAM_LAST);
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

void uCxUrcRegisterBluetoothConnect(struct uCxHandle * puCxHandle, uUEBTC_t callback)
{
    puCxHandle->callbacks.UEBTC = callback;
}

void uCxUrcRegisterBluetoothDisconnect(struct uCxHandle * puCxHandle, uUEBTDC_t callback)
{
    puCxHandle->callbacks.UEBTDC = callback;
}

void uCxUrcRegisterBluetoothBondStatus(struct uCxHandle * puCxHandle, uUEBTB_t callback)
{
    puCxHandle->callbacks.UEBTB = callback;
}

void uCxUrcRegisterBluetoothUserConfirmation(struct uCxHandle * puCxHandle, uUEBTUC_t callback)
{
    puCxHandle->callbacks.UEBTUC = callback;
}

void uCxUrcRegisterBluetoothPasskeyRequest(struct uCxHandle * puCxHandle, uUEBTUPE_t callback)
{
    puCxHandle->callbacks.UEBTUPE = callback;
}

void uCxUrcRegisterBluetoothPhyUpdate(struct uCxHandle * puCxHandle, uUEBTPHYU_t callback)
{
    puCxHandle->callbacks.UEBTPHYU = callback;
}

void uCxUrcRegisterGattClientNotification(struct uCxHandle * puCxHandle, uUEBTGCN_t callback)
{
    puCxHandle->callbacks.UEBTGCN = callback;
}

void uCxUrcRegisterGattClientIndication(struct uCxHandle * puCxHandle, uUEBTGCI_t callback)
{
    puCxHandle->callbacks.UEBTGCI = callback;
}

void uCxUrcRegisterGattServerNotification(struct uCxHandle * puCxHandle, uUEBTGCW_t callback)
{
    puCxHandle->callbacks.UEBTGCW = callback;
}

void uCxUrcRegisterGattServerReadAttribute(struct uCxHandle * puCxHandle, uUEBTGRR_t callback)
{
    puCxHandle->callbacks.UEBTGRR = callback;
}

void uCxUrcRegisterGattServerIndicationAck(struct uCxHandle * puCxHandle, uUEBTGIC_t callback)
{
    puCxHandle->callbacks.UEBTGIC = callback;
}

void uCxUrcRegisterSpsConnect(struct uCxHandle * puCxHandle, uUESPSC_t callback)
{
    puCxHandle->callbacks.UESPSC = callback;
}

void uCxUrcRegisterSpsDisconnect(struct uCxHandle * puCxHandle, uUESPSDC_t callback)
{
    puCxHandle->callbacks.UESPSDC = callback;
}

void uCxUrcRegisterSpsDataAvailable(struct uCxHandle * puCxHandle, uUESPSDA_t callback)
{
    puCxHandle->callbacks.UESPSDA = callback;
}

void uCxUrcRegisterWifiLinkUp(struct uCxHandle * puCxHandle, uUEWLU_t callback)
{
    puCxHandle->callbacks.UEWLU = callback;
}

void uCxUrcRegisterWifiLinkDown(struct uCxHandle * puCxHandle, uUEWLD_t callback)
{
    puCxHandle->callbacks.UEWLD = callback;
}

void uCxUrcRegisterWifiStationNetworkUp(struct uCxHandle * puCxHandle, uUEWSNU_t callback)
{
    puCxHandle->callbacks.UEWSNU = callback;
}

void uCxUrcRegisterWifiStationNetworkDown(struct uCxHandle * puCxHandle, uUEWSND_t callback)
{
    puCxHandle->callbacks.UEWSND = callback;
}

void uCxUrcRegisterWifiApNetworkUp(struct uCxHandle * puCxHandle, uUEWAPNU_t callback)
{
    puCxHandle->callbacks.UEWAPNU = callback;
}

void uCxUrcRegisterWifiApNetworkDown(struct uCxHandle * puCxHandle, uUEWAPND_t callback)
{
    puCxHandle->callbacks.UEWAPND = callback;
}

void uCxUrcRegisterWifiApUp(struct uCxHandle * puCxHandle, uUEWAPU_t callback)
{
    puCxHandle->callbacks.UEWAPU = callback;
}

void uCxUrcRegisterWifiApDown(struct uCxHandle * puCxHandle, uUEWAPD_t callback)
{
    puCxHandle->callbacks.UEWAPD = callback;
}

void uCxUrcRegisterWifiApStationAssociated(struct uCxHandle * puCxHandle, uUEWAPSA_t callback)
{
    puCxHandle->callbacks.UEWAPSA = callback;
}

void uCxUrcRegisterWifiApStationDisassociated(struct uCxHandle * puCxHandle, uUEWAPSDA_t callback)
{
    puCxHandle->callbacks.UEWAPSDA = callback;
}

void uCxUrcRegisterSocketConnect(struct uCxHandle * puCxHandle, uUESOC_t callback)
{
    puCxHandle->callbacks.UESOC = callback;
}

void uCxUrcRegisterSocketDataAvailable(struct uCxHandle * puCxHandle, uUESODA_t callback)
{
    puCxHandle->callbacks.UESODA = callback;
}

void uCxUrcRegisterSocketClosed(struct uCxHandle * puCxHandle, uUESOCL_t callback)
{
    puCxHandle->callbacks.UESOCL = callback;
}

void uCxUrcRegisterSocketIncommingConnection(struct uCxHandle * puCxHandle, uUESOIC_t callback)
{
    puCxHandle->callbacks.UESOIC = callback;
}

void uCxUrcRegisterMqttConnect(struct uCxHandle * puCxHandle, uUEMQC_t callback)
{
    puCxHandle->callbacks.UEMQC = callback;
}

void uCxUrcRegisterMqttDisconnect(struct uCxHandle * puCxHandle, uUEMQDC_t callback)
{
    puCxHandle->callbacks.UEMQDC = callback;
}

void uCxUrcRegisterMqttDataAvailable(struct uCxHandle * puCxHandle, uUEMQDA_t callback)
{
    puCxHandle->callbacks.UEMQDA = callback;
}

void uCxUrcRegisterDiagnosticsPingComplete(struct uCxHandle * puCxHandle, uUEDGPC_t callback)
{
    puCxHandle->callbacks.UEDGPC = callback;
}

void uCxUrcRegisterDiagnosticsPingResponse(struct uCxHandle * puCxHandle, uUEDGP_t callback)
{
    puCxHandle->callbacks.UEDGP = callback;
}

