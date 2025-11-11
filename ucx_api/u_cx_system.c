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
#include "u_cx_system.h"

int32_t uCxSystemReboot(uCxHandle_t * puCxHandle)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+CPWROFF", "", U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxSystemStoreConfiguration(uCxHandle_t * puCxHandle)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT&W", "", U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxSystemGetLocalAddress(uCxHandle_t * puCxHandle, uInterfaceId_t interface_id, uMacAddress_t * pAddress)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+USYLA=", "d", interface_id, U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+USYLA:", NULL, NULL, "m", pAddress, U_CX_AT_UTIL_PARAM_LAST);
    {
        // Always call uCxAtClientCmdEnd() even if any previous function failed
        int32_t endRet = uCxAtClientCmdEnd(pAtClient);
        if (ret >= 0) {
            ret = endRet;
        }
    }
    return ret;
}

int32_t uCxSystemSetLocalAddress(uCxHandle_t * puCxHandle, uInterfaceId_t interface_id, uMacAddress_t * address)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+USYLA=", "dm", interface_id, address, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxSystemFactoryReset(uCxHandle_t * puCxHandle)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+USYFR", "", U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxSystemDefaultSettings(uCxHandle_t * puCxHandle)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+USYDS", "", U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxSystemSetUartSettings1(uCxHandle_t * puCxHandle, int32_t baud_rate)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+USYUS=", "d", baud_rate, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxSystemSetUartSettings2(uCxHandle_t * puCxHandle, int32_t baud_rate, int32_t flow_control)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+USYUS=", "dd", baud_rate, flow_control, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxSystemSetUartSettings3(uCxHandle_t * puCxHandle, int32_t baud_rate, int32_t flow_control, int32_t change_after_confirm)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+USYUS=", "ddd", baud_rate, flow_control, change_after_confirm, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxSystemGetUartSettings(uCxHandle_t * puCxHandle, uCxSystemGetUartSettings_t * pSystemGetUartSettingsRsp)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+USYUS?", "", U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+USYUS:", NULL, NULL, "dd", &pSystemGetUartSettingsRsp->baud_rate, &pSystemGetUartSettingsRsp->flow_control, U_CX_AT_UTIL_PARAM_LAST);
    {
        // Always call uCxAtClientCmdEnd() even if any previous function failed
        int32_t endRet = uCxAtClientCmdEnd(pAtClient);
        if (ret >= 0) {
            ret = endRet;
        }
    }
    return ret;
}

int32_t uCxSystemGetLastErrorCode(uCxHandle_t * puCxHandle, int32_t * pErrorCode)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+USYEC?", "", U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+USYEC:", NULL, NULL, "d", pErrorCode, U_CX_AT_UTIL_PARAM_LAST);
    {
        // Always call uCxAtClientCmdEnd() even if any previous function failed
        int32_t endRet = uCxAtClientCmdEnd(pAtClient);
        if (ret >= 0) {
            ret = endRet;
        }
    }
    return ret;
}

int32_t uCxSystemSetExtendedError(uCxHandle_t * puCxHandle, uExtendedErrors_t extended_errors)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+USYEE=", "d", extended_errors, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxSystemGetExtendedError(uCxHandle_t * puCxHandle, uExtendedErrors_t * pExtendedErrors)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+USYEE?", "", U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+USYEE:", NULL, NULL, "d", pExtendedErrors, U_CX_AT_UTIL_PARAM_LAST);
    {
        // Always call uCxAtClientCmdEnd() even if any previous function failed
        int32_t endRet = uCxAtClientCmdEnd(pAtClient);
        if (ret >= 0) {
            ret = endRet;
        }
    }
    return ret;
}

int32_t uCxSystemSetUnixTime(uCxHandle_t * puCxHandle, const uint8_t * unix_time, int32_t unix_time_len)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+USYTU=", "h", unix_time, unix_time_len, U_CX_AT_UTIL_PARAM_LAST);
}
bool uCxSystemGetUnixTimeBegin(uCxHandle_t * puCxHandle, uByteArray_t * pUnixTime)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+USYTU?", "", U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+USYTU:", NULL, NULL, "h", pUnixTime, U_CX_AT_UTIL_PARAM_LAST);
    return ret >= 0;
}
int32_t uCxSystemSetEchoOff(uCxHandle_t * puCxHandle)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "ATE0", "", U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxSystemSetEchoOn(uCxHandle_t * puCxHandle)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "ATE1", "", U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxSystemGetEcho(uCxHandle_t * puCxHandle, uEchoOn_t * pEchoOn)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "ATE?", "", U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "", NULL, NULL, "d", pEchoOn, U_CX_AT_UTIL_PARAM_LAST);
    {
        // Always call uCxAtClientCmdEnd() even if any previous function failed
        int32_t endRet = uCxAtClientCmdEnd(pAtClient);
        if (ret >= 0) {
            ret = endRet;
        }
    }
    return ret;
}

int32_t uCxSystemSetEscSequenceChar(uCxHandle_t * puCxHandle, int32_t escape_char)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "ATS2=", "d", escape_char, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxSystemGetEscSequenceChar(uCxHandle_t * puCxHandle, int32_t * pEscapeChar)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "ATS2?", "", U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "", NULL, NULL, "d", pEscapeChar, U_CX_AT_UTIL_PARAM_LAST);
    {
        // Always call uCxAtClientCmdEnd() even if any previous function failed
        int32_t endRet = uCxAtClientCmdEnd(pAtClient);
        if (ret >= 0) {
            ret = endRet;
        }
    }
    return ret;
}

int32_t uCxSystemSetLineTermChar(uCxHandle_t * puCxHandle, int32_t line_term)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "ATS3=", "d", line_term, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxSystemGetLineTermChar(uCxHandle_t * puCxHandle, int32_t * pLineTerm)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "ATS3?", "", U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "", NULL, NULL, "d", pLineTerm, U_CX_AT_UTIL_PARAM_LAST);
    {
        // Always call uCxAtClientCmdEnd() even if any previous function failed
        int32_t endRet = uCxAtClientCmdEnd(pAtClient);
        if (ret >= 0) {
            ret = endRet;
        }
    }
    return ret;
}

int32_t uCxSystemSetRspFormatChar(uCxHandle_t * puCxHandle, int32_t resp_format)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "ATS4=", "d", resp_format, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxSystemGetRspFormatChar(uCxHandle_t * puCxHandle, int32_t * pRespFormat)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "ATS4?", "", U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "", NULL, NULL, "d", pRespFormat, U_CX_AT_UTIL_PARAM_LAST);
    {
        // Always call uCxAtClientCmdEnd() even if any previous function failed
        int32_t endRet = uCxAtClientCmdEnd(pAtClient);
        if (ret >= 0) {
            ret = endRet;
        }
    }
    return ret;
}

int32_t uCxSystemSetBackspaceChar(uCxHandle_t * puCxHandle, int32_t backspace)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "ATS5=", "d", backspace, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxSystemGetBackspaceChar(uCxHandle_t * puCxHandle, int32_t * pBackspace)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "ATS5?", "", U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "", NULL, NULL, "d", pBackspace, U_CX_AT_UTIL_PARAM_LAST);
    {
        // Always call uCxAtClientCmdEnd() even if any previous function failed
        int32_t endRet = uCxAtClientCmdEnd(pAtClient);
        if (ret >= 0) {
            ret = endRet;
        }
    }
    return ret;
}

int32_t uCxSystemSetEscSequenceSettings(uCxHandle_t * puCxHandle, int32_t pre_timeout, int32_t post_timeout, int32_t escape_timeout)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UTMES=", "ddd", pre_timeout, post_timeout, escape_timeout, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxSystemGetEscSequenceSettings(uCxHandle_t * puCxHandle, uCxSystemGetEscSequenceSettings_t * pSystemGetEscSequenceSettingsRsp)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UTMES?", "", U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UTMES:", NULL, NULL, "ddd", &pSystemGetEscSequenceSettingsRsp->pre_timeout, &pSystemGetEscSequenceSettingsRsp->post_timeout, &pSystemGetEscSequenceSettingsRsp->escape_timeout, U_CX_AT_UTIL_PARAM_LAST);
    {
        // Always call uCxAtClientCmdEnd() even if any previous function failed
        int32_t endRet = uCxAtClientCmdEnd(pAtClient);
        if (ret >= 0) {
            ret = endRet;
        }
    }
    return ret;
}

void uCxSystemRegisterStartup(uCxHandle_t * puCxHandle, uSTARTUP_t callback)
{
    puCxHandle->callbacks.STARTUP = callback;
}
