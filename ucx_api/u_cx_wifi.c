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

int32_t uCxWifiStationSetSecurityEnterprise5(uCxHandle_t * puCxHandle, int32_t wlan_handle, uWifiTlsVersion_t tls_version, const char * ca_name, const char * client_cert_name, const char * client_key_name)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UWSSE=", "ddsss", wlan_handle, tls_version, ca_name, client_cert_name, client_key_name, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxWifiStationSetSecurityEnterprise6(uCxHandle_t * puCxHandle, int32_t wlan_handle, uWifiTlsVersion_t tls_version, const char * ca_name, const char * client_cert_name, const char * client_key_name, const char * identity)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UWSSE=", "ddssss", wlan_handle, tls_version, ca_name, client_cert_name, client_key_name, identity, U_CX_AT_UTIL_PARAM_LAST);
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
            ret = uCxAtUtilParseParamsF(pParamsLine, "-dd", &pWifiStationGetSecurityRsp->rsp.SecurityModeWpaThreshold.security_mode, &pWifiStationGetSecurityRsp->rsp.SecurityModeWpaThreshold.wpa_threshold, U_CX_AT_UTIL_PARAM_LAST);
            break;
        case 0:
            pWifiStationGetSecurityRsp->type = U_CX_WIFI_STATION_GET_SECURITY_RSP_TYPE_SECURITY_MODE;
            ret = uCxAtUtilParseParamsF(pParamsLine, "-d", &pWifiStationGetSecurityRsp->rsp.SecurityMode.security_mode, U_CX_AT_UTIL_PARAM_LAST);
            break;
        case -1:
            pWifiStationGetSecurityRsp->type = U_CX_WIFI_STATION_GET_SECURITY_RSP_TYPE_SECURITY_MODE_STR_STR_STR_STR;
            ret = uCxAtUtilParseParamsF(pParamsLine, "-dssss", &pWifiStationGetSecurityRsp->rsp.SecurityModeStrStrStrStr.security_mode, &pWifiStationGetSecurityRsp->rsp.SecurityModeStrStrStrStr.ca_name, &pWifiStationGetSecurityRsp->rsp.SecurityModeStrStrStrStr.client_cert_name, &pWifiStationGetSecurityRsp->rsp.SecurityModeStrStrStrStr.client_key_name, &pWifiStationGetSecurityRsp->rsp.SecurityModeStrStrStrStr.identity, U_CX_AT_UTIL_PARAM_LAST);
            break;
        case 2:
            pWifiStationGetSecurityRsp->type = U_CX_WIFI_STATION_GET_SECURITY_RSP_TYPE_SECURITY_MODE_TLS_VERSION_STR_STR_STR_STR;
            ret = uCxAtUtilParseParamsF(pParamsLine, "-ddssss", &pWifiStationGetSecurityRsp->rsp.SecurityModeTlsVersionStrStrStrStr.security_mode, &pWifiStationGetSecurityRsp->rsp.SecurityModeTlsVersionStrStrStrStr.tls_version, &pWifiStationGetSecurityRsp->rsp.SecurityModeTlsVersionStrStrStrStr.ca_name, &pWifiStationGetSecurityRsp->rsp.SecurityModeTlsVersionStrStrStrStr.client_cert_name, &pWifiStationGetSecurityRsp->rsp.SecurityModeTlsVersionStrStrStrStr.client_key_name, &pWifiStationGetSecurityRsp->rsp.SecurityModeTlsVersionStrStrStrStr.identity, U_CX_AT_UTIL_PARAM_LAST);
            break;
        case -2:
            pWifiStationGetSecurityRsp->type = U_CX_WIFI_STATION_GET_SECURITY_RSP_TYPE_SECURITY_MODE_STR_STR;
            ret = uCxAtUtilParseParamsF(pParamsLine, "-dss", &pWifiStationGetSecurityRsp->rsp.SecurityModeStrStr.security_mode, &pWifiStationGetSecurityRsp->rsp.SecurityModeStrStr.username, &pWifiStationGetSecurityRsp->rsp.SecurityModeStrStr.ca_name, U_CX_AT_UTIL_PARAM_LAST);
            break;
        case 3:
            pWifiStationGetSecurityRsp->type = U_CX_WIFI_STATION_GET_SECURITY_RSP_TYPE_SECURITY_MODE_TLS_VERSION_STR_STR;
            ret = uCxAtUtilParseParamsF(pParamsLine, "-ddss", &pWifiStationGetSecurityRsp->rsp.SecurityModeTlsVersionStrStr.security_mode, &pWifiStationGetSecurityRsp->rsp.SecurityModeTlsVersionStrStr.tls_version, &pWifiStationGetSecurityRsp->rsp.SecurityModeTlsVersionStrStr.username, &pWifiStationGetSecurityRsp->rsp.SecurityModeTlsVersionStrStr.ca_name, U_CX_AT_UTIL_PARAM_LAST);
            break;
        default:
            return false;
    } /* ~switch (rspSyntaxVal) */
    return ret >= 0;
}

int32_t uCxWifiStationSetSecurityPeap4(uCxHandle_t * puCxHandle, int32_t wlan_handle, uWifiTlsVersion_t tls_version, const char * peap_user, const char * peap_password)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UWSSP=", "ddss", wlan_handle, tls_version, peap_user, peap_password, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxWifiStationSetSecurityPeap5(uCxHandle_t * puCxHandle, int32_t wlan_handle, uWifiTlsVersion_t tls_version, const char * peap_user, const char * peap_password, const char * ca_name)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UWSSP=", "ddsss", wlan_handle, tls_version, peap_user, peap_password, ca_name, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxWifiStationSetSecurityWpa(uCxHandle_t * puCxHandle, int32_t wlan_handle, const char * passphrase, uWifiWpaThreshold_t wpa_threshold)
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
            ret = uCxAtUtilParseParamsF(pParamsLine, "-d", &pWifiStationGetIpConfigRsp->rsp.IpMode.ip_mode, U_CX_AT_UTIL_PARAM_LAST);
            break;
        case 1:
            pWifiStationGetIpConfigRsp->type = U_CX_WIFI_STATION_GET_IP_CONFIG_RSP_TYPE_IP_MODE_IP_IP_IP_IP_IP;
            ret = uCxAtUtilParseParamsF(pParamsLine, "-diiiii", &pWifiStationGetIpConfigRsp->rsp.IpModeIpIpIpIpIp.ip_mode, &pWifiStationGetIpConfigRsp->rsp.IpModeIpIpIpIpIp.ip_addr, &pWifiStationGetIpConfigRsp->rsp.IpModeIpIpIpIpIp.subnet_mask, &pWifiStationGetIpConfigRsp->rsp.IpModeIpIpIpIpIp.gateway, &pWifiStationGetIpConfigRsp->rsp.IpModeIpIpIpIpIp.prim_dns, &pWifiStationGetIpConfigRsp->rsp.IpModeIpIpIpIpIp.sec_dns, U_CX_AT_UTIL_PARAM_LAST);
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

int32_t uCxWifiStationGetNetworkStatus(uCxHandle_t * puCxHandle, uWifiNetStatusId_t net_status_id, uSockIpAddress_t * pNetStatusVal)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UWSNST=", "d", net_status_id, U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UWSNST:", NULL, NULL, "-i", pNetStatusVal, U_CX_AT_UTIL_PARAM_LAST);
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
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UWSNST:", NULL, NULL, "di", &pWifiStationListNetworkStatusRsp->net_status_id, &pWifiStationListNetworkStatusRsp->net_status_val, U_CX_AT_UTIL_PARAM_LAST);
    return ret >= 0;
}

int32_t uCxWifiSetRegulatoryDomain(uCxHandle_t * puCxHandle, uWifiRegDomain_t reg_domain)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UWRD=", "d", reg_domain, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxWifiGetRegulatoryDomain(uCxHandle_t * puCxHandle, uWifiRegDomain_t * pRegDomain)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UWRD?", "", U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UWRD:", NULL, NULL, "d", pRegDomain, U_CX_AT_UTIL_PARAM_LAST);
    {
        // Always call uCxAtClientCmdEnd() even if any previous function failed
        int32_t endRet = uCxAtClientCmdEnd(pAtClient);
        if (ret >= 0) {
            ret = endRet;
        }
    }
    return ret;
}

int32_t uCxWifiSetChannelList(uCxHandle_t * puCxHandle, const int16_t * channel_list, int32_t channel_list_len)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UWCL=", "l", channel_list, channel_list_len, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxWifiGetChannelList(uCxHandle_t * puCxHandle, uIntList_t * pChannelList)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UWCL?", "", U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UWCL:", NULL, NULL, "l", pChannelList, U_CX_AT_UTIL_PARAM_LAST);
    {
        // Always call uCxAtClientCmdEnd() even if any previous function failed
        int32_t endRet = uCxAtClientCmdEnd(pAtClient);
        if (ret >= 0) {
            ret = endRet;
        }
    }
    return ret;
}

int32_t uCxWifiGetActiveChannels(uCxHandle_t * puCxHandle, uIntList_t * pChannelList)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UWAC?", "", U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UWAC:", NULL, NULL, "l", pChannelList, U_CX_AT_UTIL_PARAM_LAST);
    {
        // Always call uCxAtClientCmdEnd() even if any previous function failed
        int32_t endRet = uCxAtClientCmdEnd(pAtClient);
        if (ret >= 0) {
            ret = endRet;
        }
    }
    return ret;
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

void uCxWifiStationScan1Begin(uCxHandle_t * puCxHandle, uWifiScanMode_t scan_mode)
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

void uCxWifiStationScan2Begin(uCxHandle_t * puCxHandle, uWifiScanMode_t scan_mode, const char * ssid)
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

bool uCxWifiStationStatusBegin(uCxHandle_t * puCxHandle, uWifiStatusId_t status_id, uCxWifiStationStatus_t * pWifiStationStatusRsp)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    char *pParamsLine;
    int32_t rspSyntaxVal;
    size_t  paramsLen;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UWSST=", "d", status_id, U_CX_AT_UTIL_PARAM_LAST);
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
            pWifiStationStatusRsp->type = U_CX_WIFI_STATION_STATUS_RSP_TYPE_STATUS_ID_STR;
            ret = uCxAtUtilParseParamsF(pParamsLine, "ds", &pWifiStationStatusRsp->rsp.StatusIdStr.status_id, &pWifiStationStatusRsp->rsp.StatusIdStr.ssid, U_CX_AT_UTIL_PARAM_LAST);
            break;
        case 1:
            pWifiStationStatusRsp->type = U_CX_WIFI_STATION_STATUS_RSP_TYPE_STATUS_ID_MAC;
            ret = uCxAtUtilParseParamsF(pParamsLine, "dm", &pWifiStationStatusRsp->rsp.StatusIdMac.status_id, &pWifiStationStatusRsp->rsp.StatusIdMac.bssid, U_CX_AT_UTIL_PARAM_LAST);
            break;
        case 2:
        case 3:
        case 4:
            pWifiStationStatusRsp->type = U_CX_WIFI_STATION_STATUS_RSP_TYPE_STATUS_ID_INT;
            ret = uCxAtUtilParseParamsF(pParamsLine, "dd", &pWifiStationStatusRsp->rsp.StatusIdInt.status_id, &pWifiStationStatusRsp->rsp.StatusIdInt.int_val, U_CX_AT_UTIL_PARAM_LAST);
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

int32_t uCxWifiApSetConnectionParams2(uCxHandle_t * puCxHandle, const char * ssid, uWifiChannel_t channel)
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

int32_t uCxWifiApSetSecurityWpa2(uCxHandle_t * puCxHandle, const char * passphrase, uWifiWpaVersion_t wpa_version)
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
            ret = uCxAtUtilParseParamsF(pParamsLine, "dd", &pWifiApGetSecurityRsp->rsp.SecurityModeWpaVersion.security_mode, &pWifiApGetSecurityRsp->rsp.SecurityModeWpaVersion.wpa_version, U_CX_AT_UTIL_PARAM_LAST);
            break;
        case 0:
            pWifiApGetSecurityRsp->type = U_CX_WIFI_AP_GET_SECURITY_RSP_TYPE_SECURITY_MODE;
            ret = uCxAtUtilParseParamsF(pParamsLine, "d", &pWifiApGetSecurityRsp->rsp.SecurityMode.security_mode, U_CX_AT_UTIL_PARAM_LAST);
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

int32_t uCxWifiApGetNetworkStatus(uCxHandle_t * puCxHandle, uWifiNetStatusId_t net_status_id, uSockIpAddress_t * pNetStatusVal)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UWAPNST=", "d", net_status_id, U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UWAPNST:", NULL, NULL, "-i", pNetStatusVal, U_CX_AT_UTIL_PARAM_LAST);
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
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UWAPNST:", NULL, NULL, "di", &pWifiApListNetworkStatusRsp->net_status_id, &pWifiApListNetworkStatusRsp->net_status_val, U_CX_AT_UTIL_PARAM_LAST);
    return ret >= 0;
}

int32_t uCxWifiSetWifiRoaming(uCxHandle_t * puCxHandle, uWifiRoaming_t roaming)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UWSROE=", "d", roaming, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxWifiGetWifiRoaming(uCxHandle_t * puCxHandle, uWifiRoaming_t * pRoaming)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UWSROE?", "", U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UWSROE:", NULL, NULL, "d", pRoaming, U_CX_AT_UTIL_PARAM_LAST);
    {
        // Always call uCxAtClientCmdEnd() even if any previous function failed
        int32_t endRet = uCxAtClientCmdEnd(pAtClient);
        if (ret >= 0) {
            ret = endRet;
        }
    }
    return ret;
}

int32_t uCxWifiStationSetRoamingBGScanThreshold(uCxHandle_t * puCxHandle, int32_t roaming_scanning_threshold)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UWSROS0=", "d", roaming_scanning_threshold, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxWifiStationGetRoamingBGScanThreshold(uCxHandle_t * puCxHandle, int32_t * pRoamingScanningThreshold)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UWSROS0?", "", U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UWSROS0:", NULL, NULL, "d", pRoamingScanningThreshold, U_CX_AT_UTIL_PARAM_LAST);
    {
        // Always call uCxAtClientCmdEnd() even if any previous function failed
        int32_t endRet = uCxAtClientCmdEnd(pAtClient);
        if (ret >= 0) {
            ret = endRet;
        }
    }
    return ret;
}

int32_t uCxWifiStationSetRoamingSwitchLimit(uCxHandle_t * puCxHandle, int32_t roaming_switch_limit)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UWSROS1=", "d", roaming_switch_limit, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxWifiStationGetRoamingSwitchLimit(uCxHandle_t * puCxHandle, int32_t * pRoamingSwitchLimit)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UWSROS1?", "", U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UWSROS1:", NULL, NULL, "d", pRoamingSwitchLimit, U_CX_AT_UTIL_PARAM_LAST);
    {
        // Always call uCxAtClientCmdEnd() even if any previous function failed
        int32_t endRet = uCxAtClientCmdEnd(pAtClient);
        if (ret >= 0) {
            ret = endRet;
        }
    }
    return ret;
}

int32_t uCxWifiStationSetRoamingScanInterval(uCxHandle_t * puCxHandle, int32_t roaming_scan_interval)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UWSROS2=", "d", roaming_scan_interval, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxWifiStationGetRoamingScanInterval(uCxHandle_t * puCxHandle, int32_t * pRoamingScanInterval)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UWSROS2?", "", U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UWSROS2:", NULL, NULL, "d", pRoamingScanInterval, U_CX_AT_UTIL_PARAM_LAST);
    {
        // Always call uCxAtClientCmdEnd() even if any previous function failed
        int32_t endRet = uCxAtClientCmdEnd(pAtClient);
        if (ret >= 0) {
            ret = endRet;
        }
    }
    return ret;
}

int32_t uCxWifiStationSetAggressiveRoaming(uCxHandle_t * puCxHandle, uWifiRoamingAggressive_t roaming_aggressive)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UWSROS3=", "d", roaming_aggressive, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxWifiStationGetAggressiveRoaming(uCxHandle_t * puCxHandle, uWifiRoamingAggressive_t * pRoamingAggressive)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UWSROS3?", "", U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UWSROS3:", NULL, NULL, "d", pRoamingAggressive, U_CX_AT_UTIL_PARAM_LAST);
    {
        // Always call uCxAtClientCmdEnd() even if any previous function failed
        int32_t endRet = uCxAtClientCmdEnd(pAtClient);
        if (ret >= 0) {
            ret = endRet;
        }
    }
    return ret;
}

int32_t uCxWifiStationSetRoamingDelayMs(uCxHandle_t * puCxHandle, int32_t roaming_delay_time)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UWSROS4=", "d", roaming_delay_time, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxWifiStationGetRoamingDelayMs(uCxHandle_t * puCxHandle, int32_t * pRoamingDelayTime)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UWSROS4?", "", U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UWSROS4:", NULL, NULL, "d", pRoamingDelayTime, U_CX_AT_UTIL_PARAM_LAST);
    {
        // Always call uCxAtClientCmdEnd() even if any previous function failed
        int32_t endRet = uCxAtClientCmdEnd(pAtClient);
        if (ret >= 0) {
            ret = endRet;
        }
    }
    return ret;
}

int32_t uCxWifiStationSetRoamingAllChannels(uCxHandle_t * puCxHandle, uWifiRoamingChannels_t roaming_channels)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UWSROS5=", "d", roaming_channels, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxWifiStationGetRoamingAllChannels(uCxHandle_t * puCxHandle, uWifiRoamingChannels_t * pRoamingChannels)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UWSROS5?", "", U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UWSROS5:", NULL, NULL, "d", pRoamingChannels, U_CX_AT_UTIL_PARAM_LAST);
    {
        // Always call uCxAtClientCmdEnd() even if any previous function failed
        int32_t endRet = uCxAtClientCmdEnd(pAtClient);
        if (ret >= 0) {
            ret = endRet;
        }
    }
    return ret;
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

void uCxWifiRegisterStationRoamingSwitchInitiated(uCxHandle_t * puCxHandle, uUEWSRSI_t callback)
{
    puCxHandle->callbacks.UEWSRSI = callback;
}

void uCxWifiRegisterStationRoamingSwitchFailed(uCxHandle_t * puCxHandle, uUEWSRSF_t callback)
{
    puCxHandle->callbacks.UEWSRSF = callback;
}

void uCxWifiRegisterStationRoamingSwitchCompleted(uCxHandle_t * puCxHandle, uUEWSRSC_t callback)
{
    puCxHandle->callbacks.UEWSRSC = callback;
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
