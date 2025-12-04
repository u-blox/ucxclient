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
#include "u_cx_bluetooth.h"

int32_t uCxBluetoothSetMode(uCxHandle_t * puCxHandle, uBtMode_t mode)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UBTM=", "d", mode, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxBluetoothGetMode(uCxHandle_t * puCxHandle, uBtMode_t * pMode)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UBTM?", "", U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UBTM:", NULL, NULL, "d", pMode, U_CX_AT_UTIL_PARAM_LAST);
    {
        // Always call uCxAtClientCmdEnd() even if any previous function failed
        int32_t endRet = uCxAtClientCmdEnd(pAtClient);
        if (ret >= 0) {
            ret = endRet;
        }
    }
    return ret;
}

int32_t uCxBluetoothConnect(uCxHandle_t * puCxHandle, uBtLeAddress_t * bd_addr)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UBTC=", "b", bd_addr, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxBluetoothDisconnect(uCxHandle_t * puCxHandle, int32_t conn_handle)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UBTDC=", "d", conn_handle, U_CX_AT_UTIL_PARAM_LAST);
}

bool uCxBluetoothGetLocalNameBegin(uCxHandle_t * puCxHandle, const char ** ppDeviceName)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UBTLN?", "", U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UBTLN:", NULL, NULL, "s", ppDeviceName, U_CX_AT_UTIL_PARAM_LAST);
    return ret >= 0;
}

int32_t uCxBluetoothSetLocalName(uCxHandle_t * puCxHandle, const char * device_name)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UBTLN=", "s", device_name, U_CX_AT_UTIL_PARAM_LAST);
}

void uCxBluetoothDiscoveryDefaultBegin(uCxHandle_t * puCxHandle)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    uCxAtClientCmdBeginF(pAtClient, "AT+UBTD", "", U_CX_AT_UTIL_PARAM_LAST);
}

bool uCxBluetoothDiscoveryDefaultGetNext(uCxHandle_t * puCxHandle, uCxBtDiscoveryDefault_t * pBtDiscoveryDefaultRsp)
{
    int32_t ret;
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UBTD:", NULL, NULL, "bdsdh", &pBtDiscoveryDefaultRsp->bd_addr, &pBtDiscoveryDefaultRsp->rssi, &pBtDiscoveryDefaultRsp->device_name, &pBtDiscoveryDefaultRsp->data_type, &pBtDiscoveryDefaultRsp->data, U_CX_AT_UTIL_PARAM_LAST);
    return ret >= 0;
}

void uCxBluetoothDiscovery1Begin(uCxHandle_t * puCxHandle, uBtDiscoveryType_t discovery_type)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    uCxAtClientCmdBeginF(pAtClient, "AT+UBTD=", "d", discovery_type, U_CX_AT_UTIL_PARAM_LAST);
}

bool uCxBluetoothDiscovery1GetNext(uCxHandle_t * puCxHandle, uCxBtDiscovery_t * pBtDiscoveryRsp)
{
    int32_t ret;
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UBTD:", NULL, NULL, "bdsdh", &pBtDiscoveryRsp->bd_addr, &pBtDiscoveryRsp->rssi, &pBtDiscoveryRsp->device_name, &pBtDiscoveryRsp->data_type, &pBtDiscoveryRsp->data, U_CX_AT_UTIL_PARAM_LAST);
    return ret >= 0;
}

void uCxBluetoothDiscovery2Begin(uCxHandle_t * puCxHandle, uBtDiscoveryType_t discovery_type, uBtDiscoveryMode_t discovery_mode)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    uCxAtClientCmdBeginF(pAtClient, "AT+UBTD=", "dd", discovery_type, discovery_mode, U_CX_AT_UTIL_PARAM_LAST);
}

bool uCxBluetoothDiscovery2GetNext(uCxHandle_t * puCxHandle, uCxBtDiscovery_t * pBtDiscoveryRsp)
{
    int32_t ret;
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UBTD:", NULL, NULL, "bdsdh", &pBtDiscoveryRsp->bd_addr, &pBtDiscoveryRsp->rssi, &pBtDiscoveryRsp->device_name, &pBtDiscoveryRsp->data_type, &pBtDiscoveryRsp->data, U_CX_AT_UTIL_PARAM_LAST);
    return ret >= 0;
}

void uCxBluetoothDiscovery3Begin(uCxHandle_t * puCxHandle, uBtDiscoveryType_t discovery_type, uBtDiscoveryMode_t discovery_mode, int32_t discovery_length)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    uCxAtClientCmdBeginF(pAtClient, "AT+UBTD=", "ddd", discovery_type, discovery_mode, discovery_length, U_CX_AT_UTIL_PARAM_LAST);
}

bool uCxBluetoothDiscovery3GetNext(uCxHandle_t * puCxHandle, uCxBtDiscovery_t * pBtDiscoveryRsp)
{
    int32_t ret;
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UBTD:", NULL, NULL, "bdsdh", &pBtDiscoveryRsp->bd_addr, &pBtDiscoveryRsp->rssi, &pBtDiscoveryRsp->device_name, &pBtDiscoveryRsp->data_type, &pBtDiscoveryRsp->data, U_CX_AT_UTIL_PARAM_LAST);
    return ret >= 0;
}

int32_t uCxBluetoothBgDiscoveryDefaultStart(uCxHandle_t * puCxHandle)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UBTBGD", "", U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxBluetoothBgDiscoveryStart1(uCxHandle_t * puCxHandle, uBtDiscoveryType_t discovery_type)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UBTBGD=", "d", discovery_type, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxBluetoothBgDiscoveryStart2(uCxHandle_t * puCxHandle, uBtDiscoveryType_t discovery_type, uBtDiscoveryMode_t discovery_mode)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UBTBGD=", "dd", discovery_type, discovery_mode, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxBluetoothBgDiscoveryStart3(uCxHandle_t * puCxHandle, uBtDiscoveryType_t discovery_type, uBtDiscoveryMode_t discovery_mode, uBtOutputEvents_t output_events)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UBTBGD=", "ddd", discovery_type, discovery_mode, output_events, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxBluetoothBgDiscoveryStop(uCxHandle_t * puCxHandle)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UBTBGDS", "", U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxBluetoothGetRssi(uCxHandle_t * puCxHandle, int32_t conn_handle, int32_t * pRssi)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UBTRSS=", "d", conn_handle, U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UBTRSS:", NULL, NULL, "d", pRssi, U_CX_AT_UTIL_PARAM_LAST);
    {
        // Always call uCxAtClientCmdEnd() even if any previous function failed
        int32_t endRet = uCxAtClientCmdEnd(pAtClient);
        if (ret >= 0) {
            ret = endRet;
        }
    }
    return ret;
}

void uCxBluetoothListConnectionsBegin(uCxHandle_t * puCxHandle)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    uCxAtClientCmdBeginF(pAtClient, "AT+UBTCL", "", U_CX_AT_UTIL_PARAM_LAST);
}

bool uCxBluetoothListConnectionsGetNext(uCxHandle_t * puCxHandle, uCxBtListConnections_t * pBtListConnectionsRsp)
{
    int32_t ret;
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UBTCL:", NULL, NULL, "db", &pBtListConnectionsRsp->conn_handle, &pBtListConnectionsRsp->bd_addr, U_CX_AT_UTIL_PARAM_LAST);
    return ret >= 0;
}

void uCxBluetoothListConnectionStatusBegin(uCxHandle_t * puCxHandle, int32_t conn_handle)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    uCxAtClientCmdBeginF(pAtClient, "AT+UBTCST=", "d", conn_handle, U_CX_AT_UTIL_PARAM_LAST);
}

bool uCxBluetoothListConnectionStatusGetNext(uCxHandle_t * puCxHandle, uCxBtListConnectionStatus_t * pBtListConnectionStatusRsp)
{
    int32_t ret;
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UBTCST:", NULL, NULL, "dd", &pBtListConnectionStatusRsp->prop_id, &pBtListConnectionStatusRsp->status_val, U_CX_AT_UTIL_PARAM_LAST);
    return ret >= 0;
}

int32_t uCxBluetoothGetConnectionStatus(uCxHandle_t * puCxHandle, int32_t conn_handle, uBtPropId_t prop_id, int32_t * pStatusVal)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UBTCST=", "dd", conn_handle, prop_id, U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UBTCST:", NULL, NULL, "-d", pStatusVal, U_CX_AT_UTIL_PARAM_LAST);
    {
        // Always call uCxAtClientCmdEnd() even if any previous function failed
        int32_t endRet = uCxAtClientCmdEnd(pAtClient);
        if (ret >= 0) {
            ret = endRet;
        }
    }
    return ret;
}

int32_t uCxBluetoothGetAdvertiseInformation(uCxHandle_t * puCxHandle, uCxBtGetAdvertiseInformation_t * pBtGetAdvertiseInformationRsp)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UBTA?", "", U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UBTA:", NULL, NULL, "ddl", &pBtGetAdvertiseInformationRsp->legacy_adv, &pBtGetAdvertiseInformationRsp->directed_adv, &pBtGetAdvertiseInformationRsp->extended_adv_list, U_CX_AT_UTIL_PARAM_LAST);
    {
        // Always call uCxAtClientCmdEnd() even if any previous function failed
        int32_t endRet = uCxAtClientCmdEnd(pAtClient);
        if (ret >= 0) {
            ret = endRet;
        }
    }
    return ret;
}

int32_t uCxBluetoothSetLegacyAdvertiseData(uCxHandle_t * puCxHandle, const uint8_t * adv_data, int32_t adv_data_len)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UBTADL=", "h", adv_data, adv_data_len, U_CX_AT_UTIL_PARAM_LAST);
}

bool uCxBluetoothGetLegacyAdvertiseDataBegin(uCxHandle_t * puCxHandle, uByteArray_t * pAdvData)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UBTADL?", "", U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UBTADL:", NULL, NULL, "h", pAdvData, U_CX_AT_UTIL_PARAM_LAST);
    return ret >= 0;
}

int32_t uCxBluetoothClearLegacyAdvertiseData(uCxHandle_t * puCxHandle)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UBTADLC", "", U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxBluetoothSetScanResponseData(uCxHandle_t * puCxHandle, const uint8_t * scan_rsp_data, int32_t scan_rsp_data_len)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UBTASD=", "h", scan_rsp_data, scan_rsp_data_len, U_CX_AT_UTIL_PARAM_LAST);
}

bool uCxBluetoothGetScanResponseDataBegin(uCxHandle_t * puCxHandle, uByteArray_t * pScanRspData)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UBTASD?", "", U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UBTASD:", NULL, NULL, "h", pScanRspData, U_CX_AT_UTIL_PARAM_LAST);
    return ret >= 0;
}

int32_t uCxBluetoothClearScanResponseData(uCxHandle_t * puCxHandle)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UBTASDC", "", U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxBluetoothLegacyAdvertisementStart(uCxHandle_t * puCxHandle)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UBTAL", "", U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxBluetoothLegacyAdvertisementStop(uCxHandle_t * puCxHandle)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UBTALD", "", U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxBluetoothDirectedAdvertisementStart1(uCxHandle_t * puCxHandle, uBtLeAddress_t * bd_addr)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UBTAD=", "b", bd_addr, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxBluetoothDirectedAdvertisementStart2(uCxHandle_t * puCxHandle, uBtLeAddress_t * bd_addr, int32_t timeout)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UBTAD=", "bd", bd_addr, timeout, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxBluetoothDirectedAdvertisementStop(uCxHandle_t * puCxHandle)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UBTADD", "", U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxBluetoothSetConnectionIntervalMin(uCxHandle_t * puCxHandle, int32_t connection_interval_minimum)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UBTCS0=", "d", connection_interval_minimum, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxBluetoothGetConnectionIntervalMin(uCxHandle_t * puCxHandle, int32_t * pConnectionIntervalMinimum)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UBTCS0?", "", U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UBTCS0:", NULL, NULL, "d", pConnectionIntervalMinimum, U_CX_AT_UTIL_PARAM_LAST);
    {
        // Always call uCxAtClientCmdEnd() even if any previous function failed
        int32_t endRet = uCxAtClientCmdEnd(pAtClient);
        if (ret >= 0) {
            ret = endRet;
        }
    }
    return ret;
}

int32_t uCxBluetoothSetConnectionIntervalMax(uCxHandle_t * puCxHandle, int32_t connection_interval_maximum)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UBTCS1=", "d", connection_interval_maximum, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxBluetoothGetConnectionIntervalMax(uCxHandle_t * puCxHandle, int32_t * pConnectionIntervalMaximum)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UBTCS1?", "", U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UBTCS1:", NULL, NULL, "d", pConnectionIntervalMaximum, U_CX_AT_UTIL_PARAM_LAST);
    {
        // Always call uCxAtClientCmdEnd() even if any previous function failed
        int32_t endRet = uCxAtClientCmdEnd(pAtClient);
        if (ret >= 0) {
            ret = endRet;
        }
    }
    return ret;
}

int32_t uCxBluetoothSetConnectionPeripheralLatency(uCxHandle_t * puCxHandle, int32_t connection_peripheral_latency)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UBTCS2=", "d", connection_peripheral_latency, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxBluetoothGetConnectionPeripheralLatency(uCxHandle_t * puCxHandle, int32_t * pConnectionPeripheralLatency)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UBTCS2?", "", U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UBTCS2:", NULL, NULL, "d", pConnectionPeripheralLatency, U_CX_AT_UTIL_PARAM_LAST);
    {
        // Always call uCxAtClientCmdEnd() even if any previous function failed
        int32_t endRet = uCxAtClientCmdEnd(pAtClient);
        if (ret >= 0) {
            ret = endRet;
        }
    }
    return ret;
}

int32_t uCxBluetoothSetConnectionLinklossTimeout(uCxHandle_t * puCxHandle, int32_t connection_linkloss_timeout)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UBTCS3=", "d", connection_linkloss_timeout, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxBluetoothGetConnectionLinklossTimeout(uCxHandle_t * puCxHandle, int32_t * pConnectionLinklossTimeout)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UBTCS3?", "", U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UBTCS3:", NULL, NULL, "d", pConnectionLinklossTimeout, U_CX_AT_UTIL_PARAM_LAST);
    {
        // Always call uCxAtClientCmdEnd() even if any previous function failed
        int32_t endRet = uCxAtClientCmdEnd(pAtClient);
        if (ret >= 0) {
            ret = endRet;
        }
    }
    return ret;
}

int32_t uCxBluetoothSetPreferredTxPhy(uCxHandle_t * puCxHandle, int32_t preferred_tx_phy)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UBTCS4=", "d", preferred_tx_phy, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxBluetoothGetPreferredTxPhy(uCxHandle_t * puCxHandle, int32_t * pPreferredTxPhy)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UBTCS4?", "", U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UBTCS4:", NULL, NULL, "d", pPreferredTxPhy, U_CX_AT_UTIL_PARAM_LAST);
    {
        // Always call uCxAtClientCmdEnd() even if any previous function failed
        int32_t endRet = uCxAtClientCmdEnd(pAtClient);
        if (ret >= 0) {
            ret = endRet;
        }
    }
    return ret;
}

int32_t uCxBluetoothSetPreferredRxPhy(uCxHandle_t * puCxHandle, int32_t preferred_rx_phy)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UBTCS5=", "d", preferred_rx_phy, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxBluetoothGetPreferredRxPhy(uCxHandle_t * puCxHandle, int32_t * pPreferredRxPhy)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UBTCS5?", "", U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UBTCS5:", NULL, NULL, "d", pPreferredRxPhy, U_CX_AT_UTIL_PARAM_LAST);
    {
        // Always call uCxAtClientCmdEnd() even if any previous function failed
        int32_t endRet = uCxAtClientCmdEnd(pAtClient);
        if (ret >= 0) {
            ret = endRet;
        }
    }
    return ret;
}

int32_t uCxBluetoothSetLegacyAdvertisementConfig(uCxHandle_t * puCxHandle, int32_t advertisement_interval_minimum, int32_t advertisement_interval_maximum)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UBTALS=", "dd", advertisement_interval_minimum, advertisement_interval_maximum, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxBluetoothGetLegacyAdvertisementConfig(uCxHandle_t * puCxHandle, uCxBtGetLegacyAdvertisementConfig_t * pBtGetLegacyAdvertisementConfigRsp)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UBTALS?", "", U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UBTALS:", NULL, NULL, "dd", &pBtGetLegacyAdvertisementConfigRsp->advertisement_interval_minimum, &pBtGetLegacyAdvertisementConfigRsp->advertisement_interval_maximum, U_CX_AT_UTIL_PARAM_LAST);
    {
        // Always call uCxAtClientCmdEnd() even if any previous function failed
        int32_t endRet = uCxAtClientCmdEnd(pAtClient);
        if (ret >= 0) {
            ret = endRet;
        }
    }
    return ret;
}

int32_t uCxBluetoothSetScanInterval(uCxHandle_t * puCxHandle, int32_t scan_interval)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UBTSS0=", "d", scan_interval, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxBluetoothGetScanInterval(uCxHandle_t * puCxHandle, int32_t * pScanInterval)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UBTSS0?", "", U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UBTSS0:", NULL, NULL, "d", pScanInterval, U_CX_AT_UTIL_PARAM_LAST);
    {
        // Always call uCxAtClientCmdEnd() even if any previous function failed
        int32_t endRet = uCxAtClientCmdEnd(pAtClient);
        if (ret >= 0) {
            ret = endRet;
        }
    }
    return ret;
}

int32_t uCxBluetoothSetScanWindow(uCxHandle_t * puCxHandle, int32_t scan_window)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UBTSS1=", "d", scan_window, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxBluetoothGetScanWindow(uCxHandle_t * puCxHandle, int32_t * pScanWindow)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UBTSS1?", "", U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UBTSS1:", NULL, NULL, "d", pScanWindow, U_CX_AT_UTIL_PARAM_LAST);
    {
        // Always call uCxAtClientCmdEnd() even if any previous function failed
        int32_t endRet = uCxAtClientCmdEnd(pAtClient);
        if (ret >= 0) {
            ret = endRet;
        }
    }
    return ret;
}

int32_t uCxBluetoothSetConnectToDirectedAdv(uCxHandle_t * puCxHandle, int32_t connect_to_directed_adv)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UBTSS2=", "d", connect_to_directed_adv, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxBluetoothGetConnectToDirectedAdv(uCxHandle_t * puCxHandle, int32_t * pConnectToDirectedAdv)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UBTSS2?", "", U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UBTSS2:", NULL, NULL, "d", pConnectToDirectedAdv, U_CX_AT_UTIL_PARAM_LAST);
    {
        // Always call uCxAtClientCmdEnd() even if any previous function failed
        int32_t endRet = uCxAtClientCmdEnd(pAtClient);
        if (ret >= 0) {
            ret = endRet;
        }
    }
    return ret;
}

int32_t uCxBluetoothSetIoCapabilities(uCxHandle_t * puCxHandle, uBtIoCap_t io_cap)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UBTIOC=", "d", io_cap, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxBluetoothGetIoCapabilities(uCxHandle_t * puCxHandle, uBtIoCap_t * pIoCap)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UBTIOC?", "", U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UBTIOC:", NULL, NULL, "d", pIoCap, U_CX_AT_UTIL_PARAM_LAST);
    {
        // Always call uCxAtClientCmdEnd() even if any previous function failed
        int32_t endRet = uCxAtClientCmdEnd(pAtClient);
        if (ret >= 0) {
            ret = endRet;
        }
    }
    return ret;
}

int32_t uCxBluetoothSetSecurityMode(uCxHandle_t * puCxHandle, uBtSecurityMode_t security_mode)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UBTBSM=", "d", security_mode, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxBluetoothGetSecurityMode(uCxHandle_t * puCxHandle, uBtSecurityMode_t * pSecurityMode)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UBTBSM?", "", U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UBTBSM:", NULL, NULL, "d", pSecurityMode, U_CX_AT_UTIL_PARAM_LAST);
    {
        // Always call uCxAtClientCmdEnd() even if any previous function failed
        int32_t endRet = uCxAtClientCmdEnd(pAtClient);
        if (ret >= 0) {
            ret = endRet;
        }
    }
    return ret;
}

int32_t uCxBluetoothSetPairingMode(uCxHandle_t * puCxHandle, uBtPairingMode_t pairing_mode)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UBTPM=", "d", pairing_mode, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxBluetoothGetPairingMode(uCxHandle_t * puCxHandle, uBtPairingMode_t * pPairingMode)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UBTPM?", "", U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UBTPM:", NULL, NULL, "d", pPairingMode, U_CX_AT_UTIL_PARAM_LAST);
    {
        // Always call uCxAtClientCmdEnd() even if any previous function failed
        int32_t endRet = uCxAtClientCmdEnd(pAtClient);
        if (ret >= 0) {
            ret = endRet;
        }
    }
    return ret;
}

int32_t uCxBluetoothUserConfirmation(uCxHandle_t * puCxHandle, uBtLeAddress_t * bd_addr, uBtConfirm_t confirm)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UBTUC=", "bd", bd_addr, confirm, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxBluetoothUserPasskeyEntry2(uCxHandle_t * puCxHandle, uBtLeAddress_t * bd_addr, uBtConfirm_t confirm)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UBTUPE=", "bd", bd_addr, confirm, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxBluetoothUserPasskeyEntry3(uCxHandle_t * puCxHandle, uBtLeAddress_t * bd_addr, uBtConfirm_t confirm, int32_t passkey)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UBTUPE=", "bdd", bd_addr, confirm, passkey, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxBluetoothBond(uCxHandle_t * puCxHandle, uBtLeAddress_t * bd_addr)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UBTB=", "b", bd_addr, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxBluetoothUnbond(uCxHandle_t * puCxHandle, uBtLeAddress_t * bd_addr)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UBTUB=", "b", bd_addr, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxBluetoothUnbondAll(uCxHandle_t * puCxHandle)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UBTUB", "", U_CX_AT_UTIL_PARAM_LAST);
}

void uCxBluetoothListBondedDevicesBegin(uCxHandle_t * puCxHandle)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    uCxAtClientCmdBeginF(pAtClient, "AT+UBTBDL", "", U_CX_AT_UTIL_PARAM_LAST);
}

bool uCxBluetoothListBondedDevicesGetNext(uCxHandle_t * puCxHandle, uBtLeAddress_t * pBdAddr)
{
    int32_t ret;
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UBTBDL:", NULL, NULL, "b", pBdAddr, U_CX_AT_UTIL_PARAM_LAST);
    return ret >= 0;
}

int32_t uCxBluetoothSetDeviceInfoServiceChar(uCxHandle_t * puCxHandle, uBtCharId_t char_id, const char * char_value)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UBTDIS=", "ds", char_id, char_value, U_CX_AT_UTIL_PARAM_LAST);
}

bool uCxBluetoothGetDeviceInfoServiceCharBegin(uCxHandle_t * puCxHandle, uBtCharId_t char_id, const char ** ppCharValue)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UBTDIS=", "d", char_id, U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UBTDIS:", NULL, NULL, "-s", ppCharValue, U_CX_AT_UTIL_PARAM_LAST);
    return ret >= 0;
}

void uCxBluetoothListDeviceInfoServiceCharsBegin(uCxHandle_t * puCxHandle)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    uCxAtClientCmdBeginF(pAtClient, "AT+UBTDIS?", "", U_CX_AT_UTIL_PARAM_LAST);
}

bool uCxBluetoothListDeviceInfoServiceCharsGetNext(uCxHandle_t * puCxHandle, uCxBtListDeviceInfoServiceChars_t * pBtListDeviceInfoServiceCharsRsp)
{
    int32_t ret;
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UBTDIS:", NULL, NULL, "ds", &pBtListDeviceInfoServiceCharsRsp->char_id, &pBtListDeviceInfoServiceCharsRsp->char_value, U_CX_AT_UTIL_PARAM_LAST);
    return ret >= 0;
}

int32_t uCxBluetoothRequestPhy(uCxHandle_t * puCxHandle, int32_t conn_handle, int32_t tx_phy, int32_t rx_phy)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UBTPHYR=", "ddd", conn_handle, tx_phy, rx_phy, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxBluetoothGetPhy(uCxHandle_t * puCxHandle, int32_t conn_handle, uCxBtGetPhy_t * pBtGetPhyRsp)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UBTPHYR=", "d", conn_handle, U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UBTPHYR:", NULL, NULL, "-dd", &pBtGetPhyRsp->tx_phy, &pBtGetPhyRsp->rx_phy, U_CX_AT_UTIL_PARAM_LAST);
    {
        // Always call uCxAtClientCmdEnd() even if any previous function failed
        int32_t endRet = uCxAtClientCmdEnd(pAtClient);
        if (ret >= 0) {
            ret = endRet;
        }
    }
    return ret;
}

void uCxBluetoothRegisterConnect(uCxHandle_t * puCxHandle, uUEBTC_t callback)
{
    puCxHandle->callbacks.UEBTC = callback;
}

void uCxBluetoothRegisterDisconnect(uCxHandle_t * puCxHandle, uUEBTDC_t callback)
{
    puCxHandle->callbacks.UEBTDC = callback;
}

void uCxBluetoothRegisterBondStatus(uCxHandle_t * puCxHandle, uUEBTB_t callback)
{
    puCxHandle->callbacks.UEBTB = callback;
}

void uCxBluetoothRegisterUserConfirmation(uCxHandle_t * puCxHandle, uUEBTUC_t callback)
{
    puCxHandle->callbacks.UEBTUC = callback;
}

void uCxBluetoothRegisterPasskeyEntry(uCxHandle_t * puCxHandle, uUEBTUPD_t callback)
{
    puCxHandle->callbacks.UEBTUPD = callback;
}

void uCxBluetoothRegisterPasskeyRequest(uCxHandle_t * puCxHandle, uUEBTUPE_t callback)
{
    puCxHandle->callbacks.UEBTUPE = callback;
}

void uCxBluetoothRegisterPhyUpdate(uCxHandle_t * puCxHandle, uUEBTPHYU_t callback)
{
    puCxHandle->callbacks.UEBTPHYU = callback;
}

void uCxBluetoothRegisterBackgroundDiscovery(uCxHandle_t * puCxHandle, uUEBTBGD_t callback)
{
    puCxHandle->callbacks.UEBTBGD = callback;
}
