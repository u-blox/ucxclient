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
#include "u_cx_gatt_client.h"

void uCxGattClientDiscoverPrimaryServicesBegin(uCxHandle_t * puCxHandle, int32_t conn_handle)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    uCxAtClientCmdBeginF(pAtClient, "AT+UBTGPSD=", "d", conn_handle, U_CX_AT_UTIL_PARAM_LAST);
}

bool uCxGattClientDiscoverPrimaryServicesGetNext(uCxHandle_t * puCxHandle, uCxGattClientDiscoverPrimaryServices_t * pGattClientDiscoverPrimaryServicesRsp)
{
    int32_t ret;
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UBTGPSD:", NULL, NULL, "-ddh", &pGattClientDiscoverPrimaryServicesRsp->start_handle, &pGattClientDiscoverPrimaryServicesRsp->end_handle, &pGattClientDiscoverPrimaryServicesRsp->uuid, U_CX_AT_UTIL_PARAM_LAST);
    return ret >= 0;
}

void uCxGattClientDiscoverPrimaryServicesByUuidBegin(uCxHandle_t * puCxHandle, int32_t conn_handle, const uint8_t * uuid, int32_t uuid_len)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    uCxAtClientCmdBeginF(pAtClient, "AT+UBTGPSDU=", "dh", conn_handle, uuid, uuid_len, U_CX_AT_UTIL_PARAM_LAST);
}

bool uCxGattClientDiscoverPrimaryServicesByUuidGetNext(uCxHandle_t * puCxHandle, uCxGattClientDiscoverPrimaryServicesByUuid_t * pGattClientDiscoverPrimaryServicesByUuidRsp)
{
    int32_t ret;
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UBTGPSDU:", NULL, NULL, "-dd", &pGattClientDiscoverPrimaryServicesByUuidRsp->start_handle, &pGattClientDiscoverPrimaryServicesByUuidRsp->end_handle, U_CX_AT_UTIL_PARAM_LAST);
    return ret >= 0;
}

void uCxGattClientDiscoverServiceCharsBegin(uCxHandle_t * puCxHandle, int32_t conn_handle, int32_t start, int32_t end)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    uCxAtClientCmdBeginF(pAtClient, "AT+UBTGSCD=", "ddd", conn_handle, start, end, U_CX_AT_UTIL_PARAM_LAST);
}

bool uCxGattClientDiscoverServiceCharsGetNext(uCxHandle_t * puCxHandle, uCxGattClientDiscoverServiceChars_t * pGattClientDiscoverServiceCharsRsp)
{
    int32_t ret;
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UBTGSCD:", NULL, NULL, "-dhdh", &pGattClientDiscoverServiceCharsRsp->attr_handle, &pGattClientDiscoverServiceCharsRsp->properties, &pGattClientDiscoverServiceCharsRsp->value_handle, &pGattClientDiscoverServiceCharsRsp->uuid, U_CX_AT_UTIL_PARAM_LAST);
    return ret >= 0;
}

void uCxGattClientDiscoverCharDescriptorsBegin(uCxHandle_t * puCxHandle, int32_t conn_handle, int32_t value_handle, int32_t char_end_handle)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    uCxAtClientCmdBeginF(pAtClient, "AT+UBTGCDD=", "ddd", conn_handle, value_handle, char_end_handle, U_CX_AT_UTIL_PARAM_LAST);
}

bool uCxGattClientDiscoverCharDescriptorsGetNext(uCxHandle_t * puCxHandle, uCxGattClientDiscoverCharDescriptors_t * pGattClientDiscoverCharDescriptorsRsp)
{
    int32_t ret;
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UBTGCDD:", NULL, NULL, "-ddh", &pGattClientDiscoverCharDescriptorsRsp->char_handle, &pGattClientDiscoverCharDescriptorsRsp->desc_handle, &pGattClientDiscoverCharDescriptorsRsp->uuid, U_CX_AT_UTIL_PARAM_LAST);
    return ret >= 0;
}

bool uCxGattClientReadBegin(uCxHandle_t * puCxHandle, int32_t conn_handle, int32_t value_handle, uByteArray_t * pHexData)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UBTGR=", "dd", conn_handle, value_handle, U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UBTGR:", NULL, NULL, "--h", pHexData, U_CX_AT_UTIL_PARAM_LAST);
    return ret >= 0;
}

bool uCxGattClientReadByUuidBegin(uCxHandle_t * puCxHandle, int32_t conn_handle, int32_t start, int32_t end, const uint8_t * uuid, int32_t uuid_len, uCxGattClientReadByUuid_t * pGattClientReadByUuidRsp)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UBTGRU=", "dddh", conn_handle, start, end, uuid, uuid_len, U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UBTGRU:", NULL, NULL, "-dh", &pGattClientReadByUuidRsp->value_handle, &pGattClientReadByUuidRsp->hex_data, U_CX_AT_UTIL_PARAM_LAST);
    return ret >= 0;
}

int32_t uCxGattClientWrite(uCxHandle_t * puCxHandle, int32_t conn_handle, int32_t value_handle, const uint8_t * hex_data, int32_t hex_data_len)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UBTGW=", "ddh", conn_handle, value_handle, hex_data, hex_data_len, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxGattClientConfigWrite(uCxHandle_t * puCxHandle, int32_t conn_handle, int32_t desc_handle, uGattClientConfig_t config)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UBTGCCW=", "ddd", conn_handle, desc_handle, config, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxGattClientWriteNoRsp(uCxHandle_t * puCxHandle, int32_t conn_handle, int32_t value_handle, const uint8_t * hex_data, int32_t hex_data_len)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UBTGWNR=", "ddh", conn_handle, value_handle, hex_data, hex_data_len, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxGattClientWriteLong(uCxHandle_t * puCxHandle, int32_t conn_handle, int32_t value_handle, const uint8_t * hex_data, int32_t hex_data_len, uGattClientReliable_t reliable, uGattClientFlag_t flag, int32_t offset)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UBTGWL=", "ddhddd", conn_handle, value_handle, hex_data, hex_data_len, reliable, flag, offset, U_CX_AT_UTIL_PARAM_LAST);
}

void uCxGattClientRegisterNotification(uCxHandle_t * puCxHandle, uUEBTGCN_t callback)
{
    puCxHandle->callbacks.UEBTGCN = callback;
}

void uCxGattClientRegisterIndication(uCxHandle_t * puCxHandle, uUEBTGCI_t callback)
{
    puCxHandle->callbacks.UEBTGCI = callback;
}
