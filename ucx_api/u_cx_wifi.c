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
#include "u_cx_at_client.h"
#include "u_cx_wifi.h"

int32_t uCxWifiSetHostname(uCxHandle_t * puCxHandle, const char * host_name)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UWHN=", "s", host_name, U_CX_AT_UTIL_PARAM_LAST);
}

bool uCxBeginWifiGetHostname(uCxHandle_t * puCxHandle, const char ** ppHostName)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UWHN?", "", U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UWHN:", NULL, NULL, "s", ppHostName, U_CX_AT_UTIL_PARAM_LAST);
    return ret > 0;
}

int32_t uCxWifiStationSetSecurityEnterprise(uCxHandle_t * puCxHandle, int32_t wlan_handle, const char * ca_name, const char * client_cert_name, const char * client_key_name)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UWSSE=", "dsss", wlan_handle, ca_name, client_cert_name, client_key_name, U_CX_AT_UTIL_PARAM_LAST);
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

bool uCxBeginWifiStationGetConnectionParams(uCxHandle_t * puCxHandle, int32_t wlan_handle, uCxWifiStationGetConnectionParams_t * pWifiStationGetConnectionParamsRsp)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UWSCP=", "d", wlan_handle, U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UWSCP=", NULL, NULL, "ds", &pWifiStationGetConnectionParamsRsp->wlan_handle, &pWifiStationGetConnectionParamsRsp->ssid, U_CX_AT_UTIL_PARAM_LAST);
    return ret > 0;
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

int32_t uCxWifiStationGetNetworkStatus(uCxHandle_t * puCxHandle, uStatusId_t status_id, uCxWifiStationGetNetworkStatus_t * pWifiStationGetNetworkStatusRsp)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UWSNST=", "d", status_id, U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UWSNST:", NULL, NULL, "di", &pWifiStationGetNetworkStatusRsp->status_id, &pWifiStationGetNetworkStatusRsp->status_val, U_CX_AT_UTIL_PARAM_LAST);
    {
        // Always call uCxAtClientCmdEnd() even any previous function failed
        int32_t endRet = uCxAtClientCmdEnd(pAtClient);
        if (ret >= 0) {
            ret = endRet;
        }
    }
    return ret;
}

void uCxBeginWifiStationListNetworkStatus(uCxHandle_t * puCxHandle)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    uCxAtClientCmdBeginF(pAtClient, "AT+UWSNST?", "", U_CX_AT_UTIL_PARAM_LAST);
}

bool uCxWifiStationListNetworkStatusGetResponse(uCxHandle_t * puCxHandle, uCxWifiStationListNetworkStatus_t * pWifiStationListNetworkStatusRsp)
{
    int32_t ret;
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UWSNST:", NULL, NULL, "di", &pWifiStationListNetworkStatusRsp->status_id, &pWifiStationListNetworkStatusRsp->status_val, U_CX_AT_UTIL_PARAM_LAST);
    return ret >= 0;
}

void uCxBeginWifiStationScan(uCxHandle_t * puCxHandle)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    uCxAtClientCmdBeginF(pAtClient, "AT+UWSSC", "", U_CX_AT_UTIL_PARAM_LAST);
}

bool uCxWifiStationScanGetResponse(uCxHandle_t * puCxHandle, uCxWifiStationScan_t * pWifiStationScanRsp)
{
    int32_t ret;
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UWSSC:", NULL, NULL, "msddddd", &pWifiStationScanRsp->bssid, &pWifiStationScanRsp->ssid, &pWifiStationScanRsp->channel, &pWifiStationScanRsp->rssi, &pWifiStationScanRsp->authentication_suites, &pWifiStationScanRsp->unicast_ciphers, &pWifiStationScanRsp->group_ciphers, U_CX_AT_UTIL_PARAM_LAST);
    return ret >= 0;
}

void uCxBeginWifiStationScanEx1(uCxHandle_t * puCxHandle, uScanMode_t scan_mode)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    uCxAtClientCmdBeginF(pAtClient, "AT+UWSSC=", "d", scan_mode, U_CX_AT_UTIL_PARAM_LAST);
}

bool uCxWifiStationScanExGetResponse1(uCxHandle_t * puCxHandle, uCxWifiStationScanEx_t * pWifiStationScanExRsp)
{
    int32_t ret;
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UWSSC:", NULL, NULL, "msddddd", &pWifiStationScanExRsp->bssid, &pWifiStationScanExRsp->ssid, &pWifiStationScanExRsp->channel, &pWifiStationScanExRsp->rssi, &pWifiStationScanExRsp->authentication_suites, &pWifiStationScanExRsp->unicast_ciphers, &pWifiStationScanExRsp->group_ciphers, U_CX_AT_UTIL_PARAM_LAST);
    return ret >= 0;
}

void uCxBeginWifiStationScanEx2(uCxHandle_t * puCxHandle, uScanMode_t scan_mode, const char * ssid)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    uCxAtClientCmdBeginF(pAtClient, "AT+UWSSC=", "ds", scan_mode, ssid, U_CX_AT_UTIL_PARAM_LAST);
}

bool uCxWifiStationScanExGetResponse2(uCxHandle_t * puCxHandle, uCxWifiStationScanEx_t * pWifiStationScanExRsp)
{
    int32_t ret;
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UWSSC:", NULL, NULL, "msddddd", &pWifiStationScanExRsp->bssid, &pWifiStationScanExRsp->ssid, &pWifiStationScanExRsp->channel, &pWifiStationScanExRsp->rssi, &pWifiStationScanExRsp->authentication_suites, &pWifiStationScanExRsp->unicast_ciphers, &pWifiStationScanExRsp->group_ciphers, U_CX_AT_UTIL_PARAM_LAST);
    return ret >= 0;
}

bool uCxBeginWifiStationStatus(uCxHandle_t * puCxHandle, uWifiStatusId_t wifi_status_id, uCxWifiStationStatus_t * pWifiStationStatusRsp)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UWSST=", "d", wifi_status_id, U_CX_AT_UTIL_PARAM_LAST);
    char *pParamsLine = uCxAtClientCmdGetRspParamLine(pAtClient, "+UWSST:", NULL, NULL);
    //pWifiStationStatusRsp->type = GetResponseType(pParamsLine); // TODO
    switch (pWifiStationStatusRsp->type)
    {
        case U_CX_WIFI_STATION_STATUS_RSP_TYPE_WIFI_STATUS_ID_STR:
            ret = uCxAtUtilParseParamsF(pParamsLine, "ds", &pWifiStationStatusRsp->rspWifiStatusIdStr.wifi_status_id, &pWifiStationStatusRsp->rspWifiStatusIdStr.ssid, U_CX_AT_UTIL_PARAM_LAST);
            break;
        case U_CX_WIFI_STATION_STATUS_RSP_TYPE_WIFI_STATUS_ID_MAC:
            ret = uCxAtUtilParseParamsF(pParamsLine, "dm", &pWifiStationStatusRsp->rspWifiStatusIdMac.wifi_status_id, &pWifiStationStatusRsp->rspWifiStatusIdMac.bssid, U_CX_AT_UTIL_PARAM_LAST);
            break;
        case U_CX_WIFI_STATION_STATUS_RSP_TYPE_WIFI_STATUS_ID_INT:
            ret = uCxAtUtilParseParamsF(pParamsLine, "dd", &pWifiStationStatusRsp->rspWifiStatusIdInt.wifi_status_id, &pWifiStationStatusRsp->rspWifiStatusIdInt.int_val, U_CX_AT_UTIL_PARAM_LAST);
            break;
        default:
            return -1;
    } /* ~switch (pWifiStationStatusRsp->type) */
    return ret > 0;
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

bool uCxBeginWifiApGetConnectionParams(uCxHandle_t * puCxHandle, uCxWifiApGetConnectionParams_t * pWifiApGetConnectionParamsRsp)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UWAPCP?", "", U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UWAPCP:", NULL, NULL, "sd", &pWifiApGetConnectionParamsRsp->ssid, &pWifiApGetConnectionParamsRsp->channel, U_CX_AT_UTIL_PARAM_LAST);
    return ret > 0;
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

void uCxBeginWifiApListStations(uCxHandle_t * puCxHandle)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    uCxAtClientCmdBeginF(pAtClient, "AT+UWAPCS?", "", U_CX_AT_UTIL_PARAM_LAST);
}

bool uCxWifiApListStationsGetResponse(uCxHandle_t * puCxHandle, uMacAddress_t * pMac)
{
    int32_t ret;
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UWAPCS:", NULL, NULL, "m", pMac, U_CX_AT_UTIL_PARAM_LAST);
    return ret >= 0;
}

int32_t uCxWifiApGetNetworkStatus(uCxHandle_t * puCxHandle, uStatusId_t status_id, uCxWifiApGetNetworkStatus_t * pWifiApGetNetworkStatusRsp)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UWAPNST=", "d", status_id, U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UWAPNST:", NULL, NULL, "di", &pWifiApGetNetworkStatusRsp->status_id, &pWifiApGetNetworkStatusRsp->status_val, U_CX_AT_UTIL_PARAM_LAST);
    {
        // Always call uCxAtClientCmdEnd() even any previous function failed
        int32_t endRet = uCxAtClientCmdEnd(pAtClient);
        if (ret >= 0) {
            ret = endRet;
        }
    }
    return ret;
}

void uCxBeginWifiApListNetworkStatus(uCxHandle_t * puCxHandle)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    uCxAtClientCmdBeginF(pAtClient, "AT+UWAPNST?", "", U_CX_AT_UTIL_PARAM_LAST);
}

bool uCxWifiApListNetworkStatusGetResponse(uCxHandle_t * puCxHandle, uCxWifiApListNetworkStatus_t * pWifiApListNetworkStatusRsp)
{
    int32_t ret;
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UWAPNST:", NULL, NULL, "di", &pWifiApListNetworkStatusRsp->status_id, &pWifiApListNetworkStatusRsp->status_val, U_CX_AT_UTIL_PARAM_LAST);
    return ret >= 0;
}
