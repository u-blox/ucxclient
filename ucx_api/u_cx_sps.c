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
#include "u_cx_sps.h"

int32_t uCxSpsConnect1(uCxHandle_t * puCxHandle, int32_t conn_handle)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+USPSC=", "d", conn_handle, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxSpsConnect2(uCxHandle_t * puCxHandle, int32_t conn_handle, int32_t flow_control)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+USPSC=", "dd", conn_handle, flow_control, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxSpsSetServiceEnable(uCxHandle_t * puCxHandle, uSpsServiceOption_t service_option)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+USPS=", "d", service_option, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxSpsGetServiceEnable(uCxHandle_t * puCxHandle, uSpsServiceOption_t * pServiceOption)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+USPS?", "", U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+USPS:", NULL, NULL, "d", pServiceOption, U_CX_AT_UTIL_PARAM_LAST);
    {
        // Always call uCxAtClientCmdEnd() even if any previous function failed
        int32_t endRet = uCxAtClientCmdEnd(pAtClient);
        if (ret >= 0) {
            ret = endRet;
        }
    }
    return ret;
}

int32_t uCxSpsWrite(uCxHandle_t * puCxHandle, int32_t conn_handle, const uint8_t * binary_data, int32_t binary_data_len)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t written_length;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+USPSWB=", "dB", conn_handle, binary_data, binary_data_len, U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+USPSWB:", NULL, NULL, "-d", &written_length, U_CX_AT_UTIL_PARAM_LAST);
    {
        // Always call uCxAtClientCmdEnd() even if any previous function failed
        int32_t endRet = uCxAtClientCmdEnd(pAtClient);
        if (ret >= 0) {
            ret = endRet;
        }
    }
    if (ret >= 0) {
        ret = written_length;
    }
    return ret;
}

int32_t uCxSpsSetDataMode(uCxHandle_t * puCxHandle, uReadMode_t read_mode)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+USPSRM=", "d", read_mode, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxSpsGetDataMode(uCxHandle_t * puCxHandle, uReadMode_t * pReadMode)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+USPSRM?", "", U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+USPSRM:", NULL, NULL, "d", pReadMode, U_CX_AT_UTIL_PARAM_LAST);
    {
        // Always call uCxAtClientCmdEnd() even if any previous function failed
        int32_t endRet = uCxAtClientCmdEnd(pAtClient);
        if (ret >= 0) {
            ret = endRet;
        }
    }
    return ret;
}

int32_t uCxSpsRead(uCxHandle_t * puCxHandle, int32_t conn_handle, int32_t length, uint8_t * pDataBuf)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    uint8_t *pBinBuffer = pDataBuf;
    uint16_t binBufferLen = (uint16_t)length;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+USPSRB=", "dd", conn_handle, length, U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+USPSRB:", pBinBuffer, &binBufferLen, "-", U_CX_AT_UTIL_PARAM_LAST);
    {
        // Always call uCxAtClientCmdEnd() even if any previous function failed
        int32_t endRet = uCxAtClientCmdEnd(pAtClient);
        if (ret >= 0) {
            ret = endRet;
        }
    }
    if (ret >= 0) {
        ret = (int32_t)binBufferLen;
    }
    return ret;
}

void uCxSpsRegisterConnect(uCxHandle_t * puCxHandle, uUESPSC_t callback)
{
    puCxHandle->callbacks.UESPSC = callback;
}

void uCxSpsRegisterDisconnect(uCxHandle_t * puCxHandle, uUESPSDC_t callback)
{
    puCxHandle->callbacks.UESPSDC = callback;
}

void uCxSpsRegisterDataAvailable(uCxHandle_t * puCxHandle, uUESPSDA_t callback)
{
    puCxHandle->callbacks.UESPSDA = callback;
}
