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
#include "u_cx_gatt_server.h"

int32_t uCxGattServerServiceDefine(uCxHandle_t * puCxHandle, const uint8_t * uuid, int32_t uuid_len, int32_t * pSerHandle)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UBTGS=", "h", uuid, uuid_len, U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UBTGS:", NULL, NULL, "d", pSerHandle, U_CX_AT_UTIL_PARAM_LAST);
    {
        // Always call uCxAtClientCmdEnd() even if any previous function failed
        int32_t endRet = uCxAtClientCmdEnd(pAtClient);
        if (ret >= 0) {
            ret = endRet;
        }
    }
    return ret;
}

int32_t uCxGattServerCharDefine5(uCxHandle_t * puCxHandle, const uint8_t * uuid, int32_t uuid_len, const uint8_t * properties, int32_t properties_len, uSecurityRead_t security_read, uSecurityWrite_t security_write, const uint8_t * value, int32_t value_len, uCxGattServerCharDefine_t * pGattServerCharDefineRsp)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UBTGC=", "hhddh", uuid, uuid_len, properties, properties_len, security_read, security_write, value, value_len, U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UBTGC:", NULL, NULL, "dd", &pGattServerCharDefineRsp->value_handle, &pGattServerCharDefineRsp->cccd_handle, U_CX_AT_UTIL_PARAM_LAST);
    {
        // Always call uCxAtClientCmdEnd() even if any previous function failed
        int32_t endRet = uCxAtClientCmdEnd(pAtClient);
        if (ret >= 0) {
            ret = endRet;
        }
    }
    return ret;
}

int32_t uCxGattServerCharDefine6(uCxHandle_t * puCxHandle, const uint8_t * uuid, int32_t uuid_len, const uint8_t * properties, int32_t properties_len, uSecurityRead_t security_read, uSecurityWrite_t security_write, const uint8_t * value, int32_t value_len, int32_t max_length, uCxGattServerCharDefine_t * pGattServerCharDefineRsp)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UBTGC=", "hhddhd", uuid, uuid_len, properties, properties_len, security_read, security_write, value, value_len, max_length, U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UBTGC:", NULL, NULL, "dd", &pGattServerCharDefineRsp->value_handle, &pGattServerCharDefineRsp->cccd_handle, U_CX_AT_UTIL_PARAM_LAST);
    {
        // Always call uCxAtClientCmdEnd() even if any previous function failed
        int32_t endRet = uCxAtClientCmdEnd(pAtClient);
        if (ret >= 0) {
            ret = endRet;
        }
    }
    return ret;
}

int32_t uCxGattServerHostCharDefine(uCxHandle_t * puCxHandle, const uint8_t * uuid, int32_t uuid_len, const uint8_t * properties, int32_t properties_len, uSecurityRead_t security_read, uSecurityWrite_t security_write, uCxGattServerHostCharDefine_t * pGattServerHostCharDefineRsp)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UBTGHCC=", "hhdd", uuid, uuid_len, properties, properties_len, security_read, security_write, U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UBTGHCC:", NULL, NULL, "dd", &pGattServerHostCharDefineRsp->value_handle, &pGattServerHostCharDefineRsp->cccd_handle, U_CX_AT_UTIL_PARAM_LAST);
    {
        // Always call uCxAtClientCmdEnd() even if any previous function failed
        int32_t endRet = uCxAtClientCmdEnd(pAtClient);
        if (ret >= 0) {
            ret = endRet;
        }
    }
    return ret;
}

int32_t uCxGattServerDescriptorDefine4(uCxHandle_t * puCxHandle, const uint8_t * uuid, int32_t uuid_len, uSecurityRead_t security_read, uSecurityWrite_t security_write, const uint8_t * value, int32_t value_len, int32_t * pDescHandle)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UBTGD=", "hddh", uuid, uuid_len, security_read, security_write, value, value_len, U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UBTGD:", NULL, NULL, "d", pDescHandle, U_CX_AT_UTIL_PARAM_LAST);
    {
        // Always call uCxAtClientCmdEnd() even if any previous function failed
        int32_t endRet = uCxAtClientCmdEnd(pAtClient);
        if (ret >= 0) {
            ret = endRet;
        }
    }
    return ret;
}

int32_t uCxGattServerDescriptorDefine5(uCxHandle_t * puCxHandle, const uint8_t * uuid, int32_t uuid_len, uSecurityRead_t security_read, uSecurityWrite_t security_write, const uint8_t * value, int32_t value_len, int32_t max_length, int32_t * pDescHandle)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UBTGD=", "hddhd", uuid, uuid_len, security_read, security_write, value, value_len, max_length, U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UBTGD:", NULL, NULL, "d", pDescHandle, U_CX_AT_UTIL_PARAM_LAST);
    {
        // Always call uCxAtClientCmdEnd() even if any previous function failed
        int32_t endRet = uCxAtClientCmdEnd(pAtClient);
        if (ret >= 0) {
            ret = endRet;
        }
    }
    return ret;
}

int32_t uCxGattServerServiceActivate(uCxHandle_t * puCxHandle)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UBTGSA", "", U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxGattServerReadReqResponse(uCxHandle_t * puCxHandle, int32_t conn_handle, const uint8_t * value, int32_t value_len)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UBTGRRR=", "dh", conn_handle, value, value_len, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxGattServerSendNotification(uCxHandle_t * puCxHandle, int32_t conn_handle, int32_t char_handle, const uint8_t * value, int32_t value_len)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UBTGNS=", "ddh", conn_handle, char_handle, value, value_len, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxGattServerSendIndication(uCxHandle_t * puCxHandle, int32_t conn_handle, int32_t char_handle, const uint8_t * value, int32_t value_len)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UBTGIS=", "ddh", conn_handle, char_handle, value, value_len, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxGattServerSetAttrValue(uCxHandle_t * puCxHandle, int32_t attr_handle, const uint8_t * value, int32_t value_len)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UBTGAV=", "dh", attr_handle, value, value_len, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxGattServerReadReqError(uCxHandle_t * puCxHandle, int32_t conn_handle, const uint8_t * error_code, int32_t error_code_len)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UBTGRRRE=", "dh", conn_handle, error_code, error_code_len, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxGattServerWriteReqError(uCxHandle_t * puCxHandle, int32_t conn_handle, const uint8_t * error_code, int32_t error_code_len)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UBTGWRE=", "dh", conn_handle, error_code, error_code_len, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxGattServerWriteReqResponse(uCxHandle_t * puCxHandle, int32_t conn_handle)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UBTGWRR=", "d", conn_handle, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxGattServerSendServiceChangedInd(uCxHandle_t * puCxHandle, int32_t conn_handle, int32_t start_handle, int32_t end_handle)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UBTGSCI=", "ddd", conn_handle, start_handle, end_handle, U_CX_AT_UTIL_PARAM_LAST);
}

void uCxGattServerRegisterNotification(uCxHandle_t * puCxHandle, uUEBTGCW_t callback)
{
    puCxHandle->callbacks.UEBTGCW = callback;
}

void uCxGattServerRegisterReadAttribute(uCxHandle_t * puCxHandle, uUEBTGRR_t callback)
{
    puCxHandle->callbacks.UEBTGRR = callback;
}

void uCxGattServerRegisterIndicationAck(uCxHandle_t * puCxHandle, uUEBTGIC_t callback)
{
    puCxHandle->callbacks.UEBTGIC = callback;
}
