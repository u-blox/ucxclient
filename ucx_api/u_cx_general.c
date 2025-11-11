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
#include "u_cx_general.h"

int32_t uCxGeneralAttention(uCxHandle_t * puCxHandle)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT", "", U_CX_AT_UTIL_PARAM_LAST);
}

bool uCxGeneralGetManufacturerIdentificationBegin(uCxHandle_t * puCxHandle, const char ** ppManufacturer)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+GMI", "", U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "", NULL, NULL, "s", ppManufacturer, U_CX_AT_UTIL_PARAM_LAST);
    return ret >= 0;
}

bool uCxGeneralGetDeviceModelIdentificationBegin(uCxHandle_t * puCxHandle, const char ** ppDeviceModel)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+GMM", "", U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "", NULL, NULL, "s", ppDeviceModel, U_CX_AT_UTIL_PARAM_LAST);
    return ret >= 0;
}

bool uCxGeneralGetSoftwareVersionBegin(uCxHandle_t * puCxHandle, const char ** ppVersion)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+GMR", "", U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "", NULL, NULL, "s", ppVersion, U_CX_AT_UTIL_PARAM_LAST);
    return ret >= 0;
}

bool uCxGeneralGetSerialNumberBegin(uCxHandle_t * puCxHandle, const char ** ppSerialNumber)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+GSN", "", U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "", NULL, NULL, "s", ppSerialNumber, U_CX_AT_UTIL_PARAM_LAST);
    return ret >= 0;
}

bool uCxGeneralGetIdentInfoBegin(uCxHandle_t * puCxHandle, uCxGeneralGetIdentInfo_t * pGeneralGetIdentInfoRsp)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "ATI9", "", U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "", NULL, NULL, "ss", &pGeneralGetIdentInfoRsp->application_version, &pGeneralGetIdentInfoRsp->unique_identifier, U_CX_AT_UTIL_PARAM_LAST);
    return ret >= 0;
}

bool uCxGeneralGetTypeCodeBegin(uCxHandle_t * puCxHandle, const char ** ppTypeCode)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "ATI0", "", U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "", NULL, NULL, "s", ppTypeCode, U_CX_AT_UTIL_PARAM_LAST);
    return ret >= 0;
}

int32_t uCxGeneralSetGreetingText1(uCxHandle_t * puCxHandle, uGreetingMode_t greeting_mode)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+CSGT=", "d", greeting_mode, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxGeneralSetGreetingText2(uCxHandle_t * puCxHandle, uGreetingMode_t greeting_mode, const char * text)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+CSGT=", "ds", greeting_mode, text, U_CX_AT_UTIL_PARAM_LAST);
}

bool uCxGeneralGetGreetingTextBegin(uCxHandle_t * puCxHandle, uCxGeneralGetGreetingText_t * pGeneralGetGreetingTextRsp)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+CSGT?", "", U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+CSGT:", NULL, NULL, "ds", &pGeneralGetGreetingTextRsp->greeting_mode, &pGeneralGetGreetingTextRsp->text, U_CX_AT_UTIL_PARAM_LAST);
    return ret >= 0;
}
