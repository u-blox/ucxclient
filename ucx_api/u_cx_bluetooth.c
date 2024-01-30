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

int32_t uCxBluetoothSetMode(uCxHandle_t * puCxHandle, uBtMode_t bt_mode)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UBTM=", "d", bt_mode, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxBluetoothGetMode(uCxHandle_t * puCxHandle, uBtMode_t * pBtMode)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UBTM?", "", U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UBTM:", NULL, NULL, "d", pBtMode, U_CX_AT_UTIL_PARAM_LAST);
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

bool uCxBluetoothDiscoveryDefaultGetNext(uCxHandle_t * puCxHandle, uCxBluetoothDiscoveryDefault_t * pBluetoothDiscoveryDefaultRsp)
{
    int32_t ret;
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UBTD:", NULL, NULL, "bdsdh", &pBluetoothDiscoveryDefaultRsp->bd_addr, &pBluetoothDiscoveryDefaultRsp->rssi, &pBluetoothDiscoveryDefaultRsp->device_name, &pBluetoothDiscoveryDefaultRsp->data_type, &pBluetoothDiscoveryDefaultRsp->data, U_CX_AT_UTIL_PARAM_LAST);
    return ret >= 0;
}

void uCxBluetoothDiscovery1Begin(uCxHandle_t * puCxHandle, uDiscoveryType_t discovery_type)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    uCxAtClientCmdBeginF(pAtClient, "AT+UBTD=", "d", discovery_type, U_CX_AT_UTIL_PARAM_LAST);
}

bool uCxBluetoothDiscovery1GetNext(uCxHandle_t * puCxHandle, uCxBluetoothDiscovery_t * pBluetoothDiscoveryRsp)
{
    int32_t ret;
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UBTD:", NULL, NULL, "bdsdh", &pBluetoothDiscoveryRsp->bd_addr, &pBluetoothDiscoveryRsp->rssi, &pBluetoothDiscoveryRsp->device_name, &pBluetoothDiscoveryRsp->data_type, &pBluetoothDiscoveryRsp->data, U_CX_AT_UTIL_PARAM_LAST);
    return ret >= 0;
}

void uCxBluetoothDiscovery2Begin(uCxHandle_t * puCxHandle, uDiscoveryType_t discovery_type, uDiscoveryMode_t discovery_mode)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    uCxAtClientCmdBeginF(pAtClient, "AT+UBTD=", "dd", discovery_type, discovery_mode, U_CX_AT_UTIL_PARAM_LAST);
}

bool uCxBluetoothDiscovery2GetNext(uCxHandle_t * puCxHandle, uCxBluetoothDiscovery_t * pBluetoothDiscoveryRsp)
{
    int32_t ret;
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UBTD:", NULL, NULL, "bdsdh", &pBluetoothDiscoveryRsp->bd_addr, &pBluetoothDiscoveryRsp->rssi, &pBluetoothDiscoveryRsp->device_name, &pBluetoothDiscoveryRsp->data_type, &pBluetoothDiscoveryRsp->data, U_CX_AT_UTIL_PARAM_LAST);
    return ret >= 0;
}

void uCxBluetoothDiscovery3Begin(uCxHandle_t * puCxHandle, uDiscoveryType_t discovery_type, uDiscoveryMode_t discovery_mode, int32_t discovery_length)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    uCxAtClientCmdBeginF(pAtClient, "AT+UBTD=", "ddd", discovery_type, discovery_mode, discovery_length, U_CX_AT_UTIL_PARAM_LAST);
}

bool uCxBluetoothDiscovery3GetNext(uCxHandle_t * puCxHandle, uCxBluetoothDiscovery_t * pBluetoothDiscoveryRsp)
{
    int32_t ret;
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UBTD:", NULL, NULL, "bdsdh", &pBluetoothDiscoveryRsp->bd_addr, &pBluetoothDiscoveryRsp->rssi, &pBluetoothDiscoveryRsp->device_name, &pBluetoothDiscoveryRsp->data_type, &pBluetoothDiscoveryRsp->data, U_CX_AT_UTIL_PARAM_LAST);
    return ret >= 0;
}

int32_t uCxBluetoothSetBgDiscovery(uCxHandle_t * puCxHandle, uBackgroundDiscoveryMode_t background_discovery_mode)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UBTBGD=", "d", background_discovery_mode, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxBluetoothGetBgDiscovery(uCxHandle_t * puCxHandle, uBackgroundDiscoveryMode_t * pBackgroundDiscoveryMode)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UBTBGD?", "", U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UBTBGD:", NULL, NULL, "d", pBackgroundDiscoveryMode, U_CX_AT_UTIL_PARAM_LAST);
    {
        // Always call uCxAtClientCmdEnd() even if any previous function failed
        int32_t endRet = uCxAtClientCmdEnd(pAtClient);
        if (ret >= 0) {
            ret = endRet;
        }
    }
    return ret;
}

int32_t uCxBluetoothRssi(uCxHandle_t * puCxHandle, int32_t conn_handle, int32_t * pRssi)
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

bool uCxBluetoothListConnectionsGetNext(uCxHandle_t * puCxHandle, uCxBluetoothListConnections_t * pBluetoothListConnectionsRsp)
{
    int32_t ret;
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UBTCL:", NULL, NULL, "db", &pBluetoothListConnectionsRsp->conn_handle, &pBluetoothListConnectionsRsp->bd_addr, U_CX_AT_UTIL_PARAM_LAST);
    return ret >= 0;
}

void uCxBluetoothListConnectionStatusBegin(uCxHandle_t * puCxHandle, int32_t conn_handle)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    uCxAtClientCmdBeginF(pAtClient, "AT+UBTCST=", "d", conn_handle, U_CX_AT_UTIL_PARAM_LAST);
}

bool uCxBluetoothListConnectionStatusGetNext(uCxHandle_t * puCxHandle, uCxBluetoothListConnectionStatus_t * pBluetoothListConnectionStatusRsp)
{
    int32_t ret;
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UBTCST:", NULL, NULL, "dd", &pBluetoothListConnectionStatusRsp->property_id, &pBluetoothListConnectionStatusRsp->status_val, U_CX_AT_UTIL_PARAM_LAST);
    return ret >= 0;
}

int32_t uCxBluetoothGetConnectionStatus(uCxHandle_t * puCxHandle, int32_t conn_handle, uPropertyId_t property_id, uCxBluetoothGetConnectionStatus_t * pBluetoothGetConnectionStatusRsp)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UBTCST=", "dd", conn_handle, property_id, U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UBTCST:", NULL, NULL, "dd", &pBluetoothGetConnectionStatusRsp->property_id, &pBluetoothGetConnectionStatusRsp->status_val, U_CX_AT_UTIL_PARAM_LAST);
    {
        // Always call uCxAtClientCmdEnd() even if any previous function failed
        int32_t endRet = uCxAtClientCmdEnd(pAtClient);
        if (ret >= 0) {
            ret = endRet;
        }
    }
    return ret;
}

int32_t uCxBluetoothSetAdvertiseData(uCxHandle_t * puCxHandle, const uint8_t * adv_data, int32_t adv_data_len)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UBTAD=", "h", adv_data, adv_data_len, U_CX_AT_UTIL_PARAM_LAST);
}

bool uCxBluetoothGetAdvertiseDataBegin(uCxHandle_t * puCxHandle, uByteArray_t * pAdvData)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UBTAD?", "", U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UBTAD:", NULL, NULL, "h", pAdvData, U_CX_AT_UTIL_PARAM_LAST);
    return ret >= 0;
}

int32_t uCxBluetoothSetScanResponseData(uCxHandle_t * puCxHandle, const uint8_t * scan_rsp_data, int32_t scan_rsp_data_len)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UBTSD=", "h", scan_rsp_data, scan_rsp_data_len, U_CX_AT_UTIL_PARAM_LAST);
}

bool uCxBluetoothGetScanResponseDataBegin(uCxHandle_t * puCxHandle, uByteArray_t * pScanRspData)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UBTSD?", "", U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UBTSD:", NULL, NULL, "h", pScanRspData, U_CX_AT_UTIL_PARAM_LAST);
    return ret >= 0;
}

int32_t uCxBluetoothSetAdvertisements(uCxHandle_t * puCxHandle, uAdvMode_t adv_mode)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UBTA=", "d", adv_mode, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxBluetoothGetAdvertisements(uCxHandle_t * puCxHandle, uAdvMode_t * pAdvMode)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UBTA?", "", U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UBTA:", NULL, NULL, "d", pAdvMode, U_CX_AT_UTIL_PARAM_LAST);
    {
        // Always call uCxAtClientCmdEnd() even if any previous function failed
        int32_t endRet = uCxAtClientCmdEnd(pAtClient);
        if (ret >= 0) {
            ret = endRet;
        }
    }
    return ret;
}

int32_t uCxBluetoothDirectedAdvertisement1(uCxHandle_t * puCxHandle, uBtLeAddress_t * bd_addr)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UBTDA=", "b", bd_addr, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxBluetoothDirectedAdvertisement2(uCxHandle_t * puCxHandle, uBtLeAddress_t * bd_addr, int32_t timeout)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UBTDA=", "bd", bd_addr, timeout, U_CX_AT_UTIL_PARAM_LAST);
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

int32_t uCxBluetoothSetAdvIntervalMin(uCxHandle_t * puCxHandle, int32_t advertisement_interval_minimum)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UBTAS0=", "d", advertisement_interval_minimum, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxBluetoothGetAdvIntervalMin(uCxHandle_t * puCxHandle, int32_t * pAdvertisementIntervalMinimum)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UBTAS0?", "", U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UBTAS0:", NULL, NULL, "d", pAdvertisementIntervalMinimum, U_CX_AT_UTIL_PARAM_LAST);
    {
        // Always call uCxAtClientCmdEnd() even if any previous function failed
        int32_t endRet = uCxAtClientCmdEnd(pAtClient);
        if (ret >= 0) {
            ret = endRet;
        }
    }
    return ret;
}

int32_t uCxBluetoothSetAdvIntervalMax(uCxHandle_t * puCxHandle, int32_t advertisement_interval_maximum)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UBTAS1=", "d", advertisement_interval_maximum, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxBluetoothGetAdvIntervalMax(uCxHandle_t * puCxHandle, int32_t * pAdvertisementIntervalMaximum)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UBTAS1?", "", U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UBTAS1:", NULL, NULL, "d", pAdvertisementIntervalMaximum, U_CX_AT_UTIL_PARAM_LAST);
    {
        // Always call uCxAtClientCmdEnd() even if any previous function failed
        int32_t endRet = uCxAtClientCmdEnd(pAtClient);
        if (ret >= 0) {
            ret = endRet;
        }
    }
    return ret;
}

int32_t uCxBluetoothSetIoCapabilities(uCxHandle_t * puCxHandle, uIoCapabilities_t io_capabilities)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UBTIOC=", "d", io_capabilities, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxBluetoothGetIoCapabilities(uCxHandle_t * puCxHandle, uIoCapabilities_t * pIoCapabilities)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UBTIOC?", "", U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UBTIOC:", NULL, NULL, "d", pIoCapabilities, U_CX_AT_UTIL_PARAM_LAST);
    {
        // Always call uCxAtClientCmdEnd() even if any previous function failed
        int32_t endRet = uCxAtClientCmdEnd(pAtClient);
        if (ret >= 0) {
            ret = endRet;
        }
    }
    return ret;
}

int32_t uCxBluetoothSetSecurityMode(uCxHandle_t * puCxHandle, uBtSecurityMode_t bt_security_mode)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UBTBSM=", "d", bt_security_mode, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxBluetoothGetSecurityMode(uCxHandle_t * puCxHandle, uBtSecurityMode_t * pBtSecurityMode)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UBTBSM?", "", U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UBTBSM:", NULL, NULL, "d", pBtSecurityMode, U_CX_AT_UTIL_PARAM_LAST);
    {
        // Always call uCxAtClientCmdEnd() even if any previous function failed
        int32_t endRet = uCxAtClientCmdEnd(pAtClient);
        if (ret >= 0) {
            ret = endRet;
        }
    }
    return ret;
}

int32_t uCxBluetoothSetPairingMode(uCxHandle_t * puCxHandle, uPairingMode_t pairing_mode)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UBTPM=", "d", pairing_mode, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxBluetoothGetPairingMode(uCxHandle_t * puCxHandle, uPairingMode_t * pPairingMode)
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

int32_t uCxBluetoothUserConfirmation(uCxHandle_t * puCxHandle, uBtLeAddress_t * bd_addr, uYesNo_t yes_no)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UBTUC=", "bd", bd_addr, yes_no, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxBluetoothUserPasskeyEntry2(uCxHandle_t * puCxHandle, uBtLeAddress_t * bd_addr, uYesNo_t yes_no)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UBTUPE=", "bd", bd_addr, yes_no, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxBluetoothUserPasskeyEntry3(uCxHandle_t * puCxHandle, uBtLeAddress_t * bd_addr, uYesNo_t yes_no, int32_t passkey)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UBTUPE=", "bdd", bd_addr, yes_no, passkey, U_CX_AT_UTIL_PARAM_LAST);
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

bool uCxBluetoothSetDeviceInfoServiceChar1Begin(uCxHandle_t * puCxHandle, uCharacteristicId_t characteristic_id, uCxBluetoothSetDeviceInfoServiceChar_t * pBluetoothSetDeviceInfoServiceCharRsp)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UBTDIS=", "d", characteristic_id, U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UBTDIS:", NULL, NULL, "ds", &pBluetoothSetDeviceInfoServiceCharRsp->characteristic_id, &pBluetoothSetDeviceInfoServiceCharRsp->characteristic_value, U_CX_AT_UTIL_PARAM_LAST);
    return ret >= 0;
}

bool uCxBluetoothSetDeviceInfoServiceChar2Begin(uCxHandle_t * puCxHandle, uCharacteristicId_t characteristic_id, const char * characteristic_value, uCxBluetoothSetDeviceInfoServiceChar_t * pBluetoothSetDeviceInfoServiceCharRsp)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UBTDIS=", "ds", characteristic_id, characteristic_value, U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UBTDIS:", NULL, NULL, "ds", &pBluetoothSetDeviceInfoServiceCharRsp->characteristic_id, &pBluetoothSetDeviceInfoServiceCharRsp->characteristic_value, U_CX_AT_UTIL_PARAM_LAST);
    return ret >= 0;
}

void uCxBluetoothListDeviceInfoServiceCharsBegin(uCxHandle_t * puCxHandle)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    uCxAtClientCmdBeginF(pAtClient, "AT+UBTDIS?", "", U_CX_AT_UTIL_PARAM_LAST);
}

bool uCxBluetoothListDeviceInfoServiceCharsGetNext(uCxHandle_t * puCxHandle, uCxBluetoothListDeviceInfoServiceChars_t * pBluetoothListDeviceInfoServiceCharsRsp)
{
    int32_t ret;
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UBTDIS:", NULL, NULL, "ds", &pBluetoothListDeviceInfoServiceCharsRsp->characteristic_id, &pBluetoothListDeviceInfoServiceCharsRsp->characteristic_value, U_CX_AT_UTIL_PARAM_LAST);
    return ret >= 0;
}

int32_t uCxBluetoothRequestPhy(uCxHandle_t * puCxHandle, int32_t conn_handle, int32_t tx_phy, int32_t rx_phy)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UBTPHYR=", "ddd", conn_handle, tx_phy, rx_phy, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxBluetoothGetPhy(uCxHandle_t * puCxHandle, int32_t conn_handle, uCxBluetoothGetPhy_t * pBluetoothGetPhyRsp)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UBTPHYR=", "d", conn_handle, U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UBTPHYR:", NULL, NULL, "ddd", &pBluetoothGetPhyRsp->conn_handle, &pBluetoothGetPhyRsp->tx_phy, &pBluetoothGetPhyRsp->rx_phy, U_CX_AT_UTIL_PARAM_LAST);
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

void uCxBluetoothRegisterPasskeyRequest(uCxHandle_t * puCxHandle, uUEBTUPE_t callback)
{
    puCxHandle->callbacks.UEBTUPE = callback;
}

void uCxBluetoothRegisterPhyUpdate(uCxHandle_t * puCxHandle, uUEBTPHYU_t callback)
{
    puCxHandle->callbacks.UEBTPHYU = callback;
}
