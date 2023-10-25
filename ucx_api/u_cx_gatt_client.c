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
#include "u_cx_gatt_client.h"

void uCxBeginGattClientDiscoverPrimaryServices(uCxHandle_t * puCxHandle, int32_t conn_handle)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    uCxAtClientCmdBeginF(pAtClient, "AT+UBTGPSD=", "d", conn_handle, U_CX_AT_UTIL_PARAM_LAST);
}

bool uCxGattClientDiscoverPrimaryServicesGetResponse(uCxHandle_t * puCxHandle, uCxGattClientDiscoverPrimaryServices_t * pGattClientDiscoverPrimaryServicesRsp)
{
    int32_t ret;
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UBTGPSD:", NULL, NULL, "dddh", &pGattClientDiscoverPrimaryServicesRsp->conn_handle, &pGattClientDiscoverPrimaryServicesRsp->start_handle, &pGattClientDiscoverPrimaryServicesRsp->end_handle, &pGattClientDiscoverPrimaryServicesRsp->uuid, U_CX_AT_UTIL_PARAM_LAST);
    return ret >= 0;
}

void uCxBeginGattClientDiscoverPrimaryServicesByUuid(uCxHandle_t * puCxHandle, int32_t conn_handle, const uint8_t * uuid, int32_t uuid_len)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    uCxAtClientCmdBeginF(pAtClient, "AT+UBTGPSDU=", "dh", conn_handle, uuid, uuid_len, U_CX_AT_UTIL_PARAM_LAST);
}

bool uCxGattClientDiscoverPrimaryServicesByUuidGetResponse(uCxHandle_t * puCxHandle, uCxGattClientDiscoverPrimaryServicesByUuid_t * pGattClientDiscoverPrimaryServicesByUuidRsp)
{
    int32_t ret;
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UBTGPSDU:", NULL, NULL, "ddd", &pGattClientDiscoverPrimaryServicesByUuidRsp->conn_handle, &pGattClientDiscoverPrimaryServicesByUuidRsp->start_handle, &pGattClientDiscoverPrimaryServicesByUuidRsp->end_handle, U_CX_AT_UTIL_PARAM_LAST);
    return ret >= 0;
}

void uCxBeginGattClientDiscoverServiceChars(uCxHandle_t * puCxHandle, int32_t conn_handle, int32_t start, int32_t end)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    uCxAtClientCmdBeginF(pAtClient, "AT+UBTGSCD=", "ddd", conn_handle, start, end, U_CX_AT_UTIL_PARAM_LAST);
}

bool uCxGattClientDiscoverServiceCharsGetResponse(uCxHandle_t * puCxHandle, uCxGattClientDiscoverServiceChars_t * pGattClientDiscoverServiceCharsRsp)
{
    int32_t ret;
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UBTGSCD:", NULL, NULL, "ddhdh", &pGattClientDiscoverServiceCharsRsp->conn_handle, &pGattClientDiscoverServiceCharsRsp->attr_handle, &pGattClientDiscoverServiceCharsRsp->properties, &pGattClientDiscoverServiceCharsRsp->value_handle, &pGattClientDiscoverServiceCharsRsp->uuid, U_CX_AT_UTIL_PARAM_LAST);
    return ret >= 0;
}

void uCxBeginGattClientDiscoverCharDescriptors(uCxHandle_t * puCxHandle, int32_t conn_handle, int32_t value_handle, int32_t characteristic_end_handle)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    uCxAtClientCmdBeginF(pAtClient, "AT+UBTGCDD=", "ddd", conn_handle, value_handle, characteristic_end_handle, U_CX_AT_UTIL_PARAM_LAST);
}

bool uCxGattClientDiscoverCharDescriptorsGetResponse(uCxHandle_t * puCxHandle, uCxGattClientDiscoverCharDescriptors_t * pGattClientDiscoverCharDescriptorsRsp)
{
    int32_t ret;
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UBTGCDD:", NULL, NULL, "dddh", &pGattClientDiscoverCharDescriptorsRsp->conn_handle, &pGattClientDiscoverCharDescriptorsRsp->char_handle, &pGattClientDiscoverCharDescriptorsRsp->desc_handle, &pGattClientDiscoverCharDescriptorsRsp->uuid, U_CX_AT_UTIL_PARAM_LAST);
    return ret >= 0;
}

bool uCxBeginGattClientRead(uCxHandle_t * puCxHandle, int32_t conn_handle, int32_t value_handle, uCxGattClientRead_t * pGattClientReadRsp)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UBTGR=", "dd", conn_handle, value_handle, U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UBTGR:", NULL, NULL, "ddh", &pGattClientReadRsp->conn_handle, &pGattClientReadRsp->value_handle, &pGattClientReadRsp->hex_data, U_CX_AT_UTIL_PARAM_LAST);
    return ret > 0;
}

bool uCxBeginGattClientReadByUuid(uCxHandle_t * puCxHandle, int32_t conn_handle, int32_t start, int32_t end, const uint8_t * uuid, int32_t uuid_len, uCxGattClientReadByUuid_t * pGattClientReadByUuidRsp)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UBTGRU=", "dddh", conn_handle, start, end, uuid, uuid_len, U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UBTGRU:", NULL, NULL, "ddh", &pGattClientReadByUuidRsp->conn_handle, &pGattClientReadByUuidRsp->value_handle, &pGattClientReadByUuidRsp->hex_data, U_CX_AT_UTIL_PARAM_LAST);
    return ret > 0;
}

int32_t uCxGattClientWrite(uCxHandle_t * puCxHandle, int32_t conn_handle, int32_t value_handle, const uint8_t * hex_data, int32_t hex_data_len)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UBTGW=", "ddh", conn_handle, value_handle, hex_data, hex_data_len, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxGattClientConfigWrite(uCxHandle_t * puCxHandle, int32_t conn_handle, int32_t desc_handle, uConfig_t config)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UBTGCCW=", "ddd", conn_handle, desc_handle, config, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxGattClientWriteNoRsp(uCxHandle_t * puCxHandle, int32_t conn_handle, int32_t value_handle, const uint8_t * hex_data, int32_t hex_data_len)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UBTGWNR=", "ddh", conn_handle, value_handle, hex_data, hex_data_len, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxGattClientWriteLong(uCxHandle_t * puCxHandle, int32_t conn_handle, int32_t value_handle, const char * hex_data, uReliable_t reliable, uFlag_t flag, int32_t offset)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UBTGWL=", "ddsddd", conn_handle, value_handle, hex_data, reliable, flag, offset, U_CX_AT_UTIL_PARAM_LAST);
}
