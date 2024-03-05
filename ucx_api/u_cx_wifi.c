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
#include <string.h>
#include "u_cx_at_client.h"
#include "u_cx_wifi.h"

int32_t uCxWifiSetHostname(uCxHandle_t * puCxHandle, const char * host_name)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UWHN=", "s", host_name, U_CX_AT_UTIL_PARAM_LAST);
}

bool uCxWifiGetHostnameBegin(uCxHandle_t * puCxHandle, const char ** ppHostName)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UWHN?", "", U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UWHN:", NULL, NULL, "s", ppHostName, U_CX_AT_UTIL_PARAM_LAST);
    return ret >= 0;
}

int32_t uCxWifiStationSetSecurityEnterprise(uCxHandle_t * puCxHandle, int32_t wlan_handle, const char * ca_name, const char * client_cert_name, const char * client_key_name)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UWSSE=", "dsss", wlan_handle, ca_name, client_cert_name, client_key_name, U_CX_AT_UTIL_PARAM_LAST);
}

bool uCxWifiStationGetSecurityBegin(uCxHandle_t * puCxHandle, int32_t wlan_handle, uCxWifiStationGetSecurity_t * pWifiStationGetSecurityRsp)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    char *pParamsLine;
    int32_t rspSyntaxVal;
    size_t  paramsLen;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UWSS=", "d", wlan_handle, U_CX_AT_UTIL_PARAM_LAST);
    pParamsLine = uCxAtClientCmdGetRspParamLine(pAtClient, "+UWSS:", NULL, NULL);
    if (pParamsLine == NULL) {
        return false;
    }
    paramsLen = strlen(pParamsLine);
    if (uCxAtUtilParseParamsF(pParamsLine, "-d", &rspSyntaxVal) != 1) {
        return false;
    }
    uCxAtUtilReplaceChar(pParamsLine, paramsLen, 0, ',');
    switch (rspSyntaxVal)
    {
        case 1:
            pWifiStationGetSecurityRsp->type = U_CX_WIFI_STATION_GET_SECURITY_RSP_TYPE_SECURITY_MODE_WPA_THRESHOLD;
            ret = uCxAtUtilParseParamsF(pParamsLine, "-dd", &pWifiStationGetSecurityRsp->rspSecurityModeWpaThreshold.security_mode, &pWifiStationGetSecurityRsp->rspSecurityModeWpaThreshold.wpa_threshold, U_CX_AT_UTIL_PARAM_LAST);
            break;
        case 0:
            pWifiStationGetSecurityRsp->type = U_CX_WIFI_STATION_GET_SECURITY_RSP_TYPE_SECURITY_MODE;
            ret = uCxAtUtilParseParamsF(pParamsLine, "-d", &pWifiStationGetSecurityRsp->rspSecurityMode.security_mode, U_CX_AT_UTIL_PARAM_LAST);
            break;
        case 2:
            pWifiStationGetSecurityRsp->type = U_CX_WIFI_STATION_GET_SECURITY_RSP_TYPE_SECURITY_MODE_STR_STR_STR;
            ret = uCxAtUtilParseParamsF(pParamsLine, "-dsss", &pWifiStationGetSecurityRsp->rspSecurityModeStrStrStr.security_mode, &pWifiStationGetSecurityRsp->rspSecurityModeStrStrStr.ca_name, &pWifiStationGetSecurityRsp->rspSecurityModeStrStrStr.client_cert_name, &pWifiStationGetSecurityRsp->rspSecurityModeStrStrStr.client_key_name, U_CX_AT_UTIL_PARAM_LAST);
            break;
        case 3:
            pWifiStationGetSecurityRsp->type = U_CX_WIFI_STATION_GET_SECURITY_RSP_TYPE_SECURITY_MODE_STR_STR;
            ret = uCxAtUtilParseParamsF(pParamsLine, "-dss", &pWifiStationGetSecurityRsp->rspSecurityModeStrStr.security_mode, &pWifiStationGetSecurityRsp->rspSecurityModeStrStr.username, &pWifiStationGetSecurityRsp->rspSecurityModeStrStr.ca_name, U_CX_AT_UTIL_PARAM_LAST);
            break;
        default:
            return false;
    } /* ~switch (rspSyntaxVal) */
    return ret >= 0;
}

int32_t uCxWifiStationSetSecurityPeap3(uCxHandle_t * puCxHandle, int32_t wlan_handle, const char * peap_user, const char * peap_password)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UWSSP=", "dss", wlan_handle, peap_user, peap_password, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxWifiStationSetSecurityPeap4(uCxHandle_t * puCxHandle, int32_t wlan_handle, const char * peap_user, const char * peap_password, const char * ca_name)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UWSSP=", "dsss", wlan_handle, peap_user, peap_password, ca_name, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxWifiStationSetSecurityWpa(uCxHandle_t * puCxHandle, int32_t wlan_handle, const char * passphrase, uWpaThreshold_t wpa_threshold)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UWSSW=", "dsd", wlan_handle, passphrase, wpa_threshold, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxWifiStationSetSecurityOpen(uCxHandle_t * puCxHandle, int32_t wlan_handle)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UWSSO=", "d", wlan_handle, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxWifiStationSetConnectionParams(uCxHandle_t * puCxHandle, int32_t wlan_handle, const char * ssid)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UWSCP=", "ds", wlan_handle, ssid, U_CX_AT_UTIL_PARAM_LAST);
}

bool uCxWifiStationGetConnectionParamsBegin(uCxHandle_t * puCxHandle, int32_t wlan_handle, const char ** ppSsid)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UWSCP=", "d", wlan_handle, U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UWSCP:", NULL, NULL, "-s", ppSsid, U_CX_AT_UTIL_PARAM_LAST);
    return ret >= 0;
}

int32_t uCxWifiStationSetIpConfigStatic4(uCxHandle_t * puCxHandle, int32_t wlan_handle, uSockIpAddress_t * ip_addr, uSockIpAddress_t * subnet_mask, uSockIpAddress_t * gateway)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UWSIPS=", "diii", wlan_handle, ip_addr, subnet_mask, gateway, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxWifiStationSetIpConfigStatic5(uCxHandle_t * puCxHandle, int32_t wlan_handle, uSockIpAddress_t * ip_addr, uSockIpAddress_t * subnet_mask, uSockIpAddress_t * gateway, uSockIpAddress_t * prim_dns)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UWSIPS=", "diiii", wlan_handle, ip_addr, subnet_mask, gateway, prim_dns, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxWifiStationSetIpConfigStatic6(uCxHandle_t * puCxHandle, int32_t wlan_handle, uSockIpAddress_t * ip_addr, uSockIpAddress_t * subnet_mask, uSockIpAddress_t * gateway, uSockIpAddress_t * prim_dns, uSockIpAddress_t * sec_dns)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UWSIPS=", "diiiii", wlan_handle, ip_addr, subnet_mask, gateway, prim_dns, sec_dns, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxWifiStationSetIpConfigDhcp(uCxHandle_t * puCxHandle, int32_t wlan_handle)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UWSIPD=", "d", wlan_handle, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxWifiStationGetIpConfig(uCxHandle_t * puCxHandle, int32_t wlan_handle, uCxWifiStationGetIpConfig_t * pWifiStationGetIpConfigRsp)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    char *pParamsLine;
    int32_t rspSyntaxVal;
    size_t  paramsLen;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UWSIP=", "d", wlan_handle, U_CX_AT_UTIL_PARAM_LAST);
    pParamsLine = uCxAtClientCmdGetRspParamLine(pAtClient, "+UWSIP:", NULL, NULL);
    if (pParamsLine == NULL) {
        return false;
    }
    paramsLen = strlen(pParamsLine);
    if (uCxAtUtilParseParamsF(pParamsLine, "-d", &rspSyntaxVal) != 1) {
        return false;
    }
    uCxAtUtilReplaceChar(pParamsLine, paramsLen, 0, ',');
    switch (rspSyntaxVal)
    {
        case 0:
            pWifiStationGetIpConfigRsp->type = U_CX_WIFI_STATION_GET_IP_CONFIG_RSP_TYPE_IP_MODE;
            ret = uCxAtUtilParseParamsF(pParamsLine, "-d", &pWifiStationGetIpConfigRsp->rspIpMode.ip_mode, U_CX_AT_UTIL_PARAM_LAST);
            break;
        case 1:
            pWifiStationGetIpConfigRsp->type = U_CX_WIFI_STATION_GET_IP_CONFIG_RSP_TYPE_IP_MODE_IP_IP_IP_IP_IP;
            ret = uCxAtUtilParseParamsF(pParamsLine, "-diiiii", &pWifiStationGetIpConfigRsp->rspIpModeIpIpIpIpIp.ip_mode, &pWifiStationGetIpConfigRsp->rspIpModeIpIpIpIpIp.ip_addr, &pWifiStationGetIpConfigRsp->rspIpModeIpIpIpIpIp.subnet_mask, &pWifiStationGetIpConfigRsp->rspIpModeIpIpIpIpIp.gateway, &pWifiStationGetIpConfigRsp->rspIpModeIpIpIpIpIp.prim_dns, &pWifiStationGetIpConfigRsp->rspIpModeIpIpIpIpIp.sec_dns, U_CX_AT_UTIL_PARAM_LAST);
            break;
        default:
            return false;
    } /* ~switch (rspSyntaxVal) */
    {
        // Always call uCxAtClientCmdEnd() even if any previous function failed
        int32_t endRet = uCxAtClientCmdEnd(pAtClient);
        if (ret >= 0) {
            ret = endRet;
        }
    }
    return ret;
}

int32_t uCxWifiStationConnect(uCxHandle_t * puCxHandle, int32_t wlan_handle)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UWSC=", "d", wlan_handle, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxWifiStationDisconnect(uCxHandle_t * puCxHandle)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UWSDC", "", U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxWifiStationGetNetworkStatus(uCxHandle_t * puCxHandle, uStatusId_t status_id, uSockIpAddress_t * pStatusVal)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UWSNST=", "d", status_id, U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UWSNST:", NULL, NULL, "-i", pStatusVal, U_CX_AT_UTIL_PARAM_LAST);
    {
        // Always call uCxAtClientCmdEnd() even if any previous function failed
        int32_t endRet = uCxAtClientCmdEnd(pAtClient);
        if (ret >= 0) {
            ret = endRet;
        }
    }
    return ret;
}

void uCxWifiStationListNetworkStatusBegin(uCxHandle_t * puCxHandle)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    uCxAtClientCmdBeginF(pAtClient, "AT+UWSNST?", "", U_CX_AT_UTIL_PARAM_LAST);
}

bool uCxWifiStationListNetworkStatusGetNext(uCxHandle_t * puCxHandle, uCxWifiStationListNetworkStatus_t * pWifiStationListNetworkStatusRsp)
{
    int32_t ret;
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UWSNST:", NULL, NULL, "di", &pWifiStationListNetworkStatusRsp->status_id, &pWifiStationListNetworkStatusRsp->status_val, U_CX_AT_UTIL_PARAM_LAST);
    return ret >= 0;
}

void uCxWifiStationScanDefaultBegin(uCxHandle_t * puCxHandle)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    uCxAtClientCmdBeginF(pAtClient, "AT+UWSSC", "", U_CX_AT_UTIL_PARAM_LAST);
}

bool uCxWifiStationScanDefaultGetNext(uCxHandle_t * puCxHandle, uCxWifiStationScanDefault_t * pWifiStationScanDefaultRsp)
{
    int32_t ret;
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UWSSC:", NULL, NULL, "msddddd", &pWifiStationScanDefaultRsp->bssid, &pWifiStationScanDefaultRsp->ssid, &pWifiStationScanDefaultRsp->channel, &pWifiStationScanDefaultRsp->rssi, &pWifiStationScanDefaultRsp->authentication_suites, &pWifiStationScanDefaultRsp->unicast_ciphers, &pWifiStationScanDefaultRsp->group_ciphers, U_CX_AT_UTIL_PARAM_LAST);
    return ret >= 0;
}

void uCxWifiStationScan1Begin(uCxHandle_t * puCxHandle, uScanMode_t scan_mode)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    uCxAtClientCmdBeginF(pAtClient, "AT+UWSSC=", "d", scan_mode, U_CX_AT_UTIL_PARAM_LAST);
}

bool uCxWifiStationScan1GetNext(uCxHandle_t * puCxHandle, uCxWifiStationScan_t * pWifiStationScanRsp)
{
    int32_t ret;
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UWSSC:", NULL, NULL, "msddddd", &pWifiStationScanRsp->bssid, &pWifiStationScanRsp->ssid, &pWifiStationScanRsp->channel, &pWifiStationScanRsp->rssi, &pWifiStationScanRsp->authentication_suites, &pWifiStationScanRsp->unicast_ciphers, &pWifiStationScanRsp->group_ciphers, U_CX_AT_UTIL_PARAM_LAST);
    return ret >= 0;
}

void uCxWifiStationScan2Begin(uCxHandle_t * puCxHandle, uScanMode_t scan_mode, const char * ssid)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    uCxAtClientCmdBeginF(pAtClient, "AT+UWSSC=", "ds", scan_mode, ssid, U_CX_AT_UTIL_PARAM_LAST);
}

bool uCxWifiStationScan2GetNext(uCxHandle_t * puCxHandle, uCxWifiStationScan_t * pWifiStationScanRsp)
{
    int32_t ret;
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UWSSC:", NULL, NULL, "msddddd", &pWifiStationScanRsp->bssid, &pWifiStationScanRsp->ssid, &pWifiStationScanRsp->channel, &pWifiStationScanRsp->rssi, &pWifiStationScanRsp->authentication_suites, &pWifiStationScanRsp->unicast_ciphers, &pWifiStationScanRsp->group_ciphers, U_CX_AT_UTIL_PARAM_LAST);
    return ret >= 0;
}

bool uCxWifiStationStatusBegin(uCxHandle_t * puCxHandle, uWifiStatusId_t wifi_status_id, uCxWifiStationStatus_t * pWifiStationStatusRsp)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    char *pParamsLine;
    int32_t rspSyntaxVal;
    size_t  paramsLen;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UWSST=", "d", wifi_status_id, U_CX_AT_UTIL_PARAM_LAST);
    pParamsLine = uCxAtClientCmdGetRspParamLine(pAtClient, "+UWSST:", NULL, NULL);
    if (pParamsLine == NULL) {
        return false;
    }
    paramsLen = strlen(pParamsLine);
    if (uCxAtUtilParseParamsF(pParamsLine, "d", &rspSyntaxVal) != 1) {
        return false;
    }
    uCxAtUtilReplaceChar(pParamsLine, paramsLen, 0, ',');
    switch (rspSyntaxVal)
    {
        case 0:
            pWifiStationStatusRsp->type = U_CX_WIFI_STATION_STATUS_RSP_TYPE_WIFI_STATUS_ID_STR;
            ret = uCxAtUtilParseParamsF(pParamsLine, "ds", &pWifiStationStatusRsp->rspWifiStatusIdStr.wifi_status_id, &pWifiStationStatusRsp->rspWifiStatusIdStr.ssid, U_CX_AT_UTIL_PARAM_LAST);
            break;
        case 1:
            pWifiStationStatusRsp->type = U_CX_WIFI_STATION_STATUS_RSP_TYPE_WIFI_STATUS_ID_MAC;
            ret = uCxAtUtilParseParamsF(pParamsLine, "dm", &pWifiStationStatusRsp->rspWifiStatusIdMac.wifi_status_id, &pWifiStationStatusRsp->rspWifiStatusIdMac.bssid, U_CX_AT_UTIL_PARAM_LAST);
            break;
        case 2:
        case 3:
        case 4:
            pWifiStationStatusRsp->type = U_CX_WIFI_STATION_STATUS_RSP_TYPE_WIFI_STATUS_ID_INT;
            ret = uCxAtUtilParseParamsF(pParamsLine, "dd", &pWifiStationStatusRsp->rspWifiStatusIdInt.wifi_status_id, &pWifiStationStatusRsp->rspWifiStatusIdInt.int_val, U_CX_AT_UTIL_PARAM_LAST);
            break;
        default:
            return false;
    } /* ~switch (rspSyntaxVal) */
    return ret >= 0;
}

int32_t uCxWifiApActivate(uCxHandle_t * puCxHandle)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UWAPA", "", U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxWifiApDeactivate(uCxHandle_t * puCxHandle)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UWAPD", "", U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxWifiApSetConnectionParams1(uCxHandle_t * puCxHandle, const char * ssid)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UWAPCP=", "s", ssid, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxWifiApSetConnectionParams2(uCxHandle_t * puCxHandle, const char * ssid, uChannel_t channel)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UWAPCP=", "sd", ssid, channel, U_CX_AT_UTIL_PARAM_LAST);
}

bool uCxWifiApGetConnectionParamsBegin(uCxHandle_t * puCxHandle, uCxWifiApGetConnectionParams_t * pWifiApGetConnectionParamsRsp)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UWAPCP?", "", U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UWAPCP:", NULL, NULL, "sd", &pWifiApGetConnectionParamsRsp->ssid, &pWifiApGetConnectionParamsRsp->channel, U_CX_AT_UTIL_PARAM_LAST);
    return ret >= 0;
}

int32_t uCxWifiApSetSecurityWpa1(uCxHandle_t * puCxHandle, const char * passphrase)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UWAPSW=", "s", passphrase, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxWifiApSetSecurityWpa2(uCxHandle_t * puCxHandle, const char * passphrase, uWpaVersion_t wpa_version)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UWAPSW=", "sd", passphrase, wpa_version, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxWifiApSetSecurityOpen(uCxHandle_t * puCxHandle)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UWAPSO", "", U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxWifiApGetSecurity(uCxHandle_t * puCxHandle, uCxWifiApGetSecurity_t * pWifiApGetSecurityRsp)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    char *pParamsLine;
    int32_t rspSyntaxVal;
    size_t  paramsLen;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UWAPS?", "", U_CX_AT_UTIL_PARAM_LAST);
    pParamsLine = uCxAtClientCmdGetRspParamLine(pAtClient, "+UWAPS:", NULL, NULL);
    if (pParamsLine == NULL) {
        return false;
    }
    paramsLen = strlen(pParamsLine);
    if (uCxAtUtilParseParamsF(pParamsLine, "d", &rspSyntaxVal) != 1) {
        return false;
    }
    uCxAtUtilReplaceChar(pParamsLine, paramsLen, 0, ',');
    switch (rspSyntaxVal)
    {
        case 1:
            pWifiApGetSecurityRsp->type = U_CX_WIFI_AP_GET_SECURITY_RSP_TYPE_SECURITY_MODE_WPA_VERSION;
            ret = uCxAtUtilParseParamsF(pParamsLine, "dd", &pWifiApGetSecurityRsp->rspSecurityModeWpaVersion.security_mode, &pWifiApGetSecurityRsp->rspSecurityModeWpaVersion.wpa_version, U_CX_AT_UTIL_PARAM_LAST);
            break;
        case 0:
            pWifiApGetSecurityRsp->type = U_CX_WIFI_AP_GET_SECURITY_RSP_TYPE_SECURITY_MODE;
            ret = uCxAtUtilParseParamsF(pParamsLine, "d", &pWifiApGetSecurityRsp->rspSecurityMode.security_mode, U_CX_AT_UTIL_PARAM_LAST);
            break;
        default:
            return false;
    } /* ~switch (rspSyntaxVal) */
    {
        // Always call uCxAtClientCmdEnd() even if any previous function failed
        int32_t endRet = uCxAtClientCmdEnd(pAtClient);
        if (ret >= 0) {
            ret = endRet;
        }
    }
    return ret;
}

void uCxWifiApListStationsBegin(uCxHandle_t * puCxHandle)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    uCxAtClientCmdBeginF(pAtClient, "AT+UWAPCS?", "", U_CX_AT_UTIL_PARAM_LAST);
}

bool uCxWifiApListStationsGetNext(uCxHandle_t * puCxHandle, uMacAddress_t * pMac)
{
    int32_t ret;
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UWAPCS:", NULL, NULL, "m", pMac, U_CX_AT_UTIL_PARAM_LAST);
    return ret >= 0;
}

int32_t uCxWifiApGetNetworkStatus(uCxHandle_t * puCxHandle, uStatusId_t status_id, uSockIpAddress_t * pStatusVal)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UWAPNST=", "d", status_id, U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UWAPNST:", NULL, NULL, "-i", pStatusVal, U_CX_AT_UTIL_PARAM_LAST);
    {
        // Always call uCxAtClientCmdEnd() even if any previous function failed
        int32_t endRet = uCxAtClientCmdEnd(pAtClient);
        if (ret >= 0) {
            ret = endRet;
        }
    }
    return ret;
}

void uCxWifiApListNetworkStatusBegin(uCxHandle_t * puCxHandle)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    uCxAtClientCmdBeginF(pAtClient, "AT+UWAPNST?", "", U_CX_AT_UTIL_PARAM_LAST);
}

bool uCxWifiApListNetworkStatusGetNext(uCxHandle_t * puCxHandle, uCxWifiApListNetworkStatus_t * pWifiApListNetworkStatusRsp)
{
    int32_t ret;
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UWAPNST:", NULL, NULL, "di", &pWifiApListNetworkStatusRsp->status_id, &pWifiApListNetworkStatusRsp->status_val, U_CX_AT_UTIL_PARAM_LAST);
    return ret >= 0;
}

void uCxWifiRegisterLinkUp(uCxHandle_t * puCxHandle, uUEWLU_t callback)
{
    puCxHandle->callbacks.UEWLU = callback;
}

void uCxWifiRegisterLinkDown(uCxHandle_t * puCxHandle, uUEWLD_t callback)
{
    puCxHandle->callbacks.UEWLD = callback;
}

void uCxWifiRegisterStationNetworkUp(uCxHandle_t * puCxHandle, uUEWSNU_t callback)
{
    puCxHandle->callbacks.UEWSNU = callback;
}

void uCxWifiRegisterStationNetworkDown(uCxHandle_t * puCxHandle, uUEWSND_t callback)
{
    puCxHandle->callbacks.UEWSND = callback;
}

void uCxWifiRegisterApNetworkUp(uCxHandle_t * puCxHandle, uUEWAPNU_t callback)
{
    puCxHandle->callbacks.UEWAPNU = callback;
}

void uCxWifiRegisterApNetworkDown(uCxHandle_t * puCxHandle, uUEWAPND_t callback)
{
    puCxHandle->callbacks.UEWAPND = callback;
}

void uCxWifiRegisterApUp(uCxHandle_t * puCxHandle, uUEWAPU_t callback)
{
    puCxHandle->callbacks.UEWAPU = callback;
}

void uCxWifiRegisterApDown(uCxHandle_t * puCxHandle, uUEWAPD_t callback)
{
    puCxHandle->callbacks.UEWAPD = callback;
}

void uCxWifiRegisterApStationAssociated(uCxHandle_t * puCxHandle, uUEWAPSA_t callback)
{
    puCxHandle->callbacks.UEWAPSA = callback;
}

void uCxWifiRegisterApStationDisassociated(uCxHandle_t * puCxHandle, uUEWAPSDA_t callback)
{
    puCxHandle->callbacks.UEWAPSDA = callback;
}
