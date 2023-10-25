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
#include "u_cx_general.h"

int32_t uCxGeneralAttention(uCxHandle_t * puCxHandle)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT", "", U_CX_AT_UTIL_PARAM_LAST);
}

bool uCxBeginGeneralGetManufacturerIdentification(uCxHandle_t * puCxHandle, const char ** ppManufacturer)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+GMI", "", U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "", NULL, NULL, "s", ppManufacturer, U_CX_AT_UTIL_PARAM_LAST);
    return ret > 0;
}

bool uCxBeginGeneralGetDeviceModelIdentification(uCxHandle_t * puCxHandle, const char ** ppDeviceModel)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+GMM", "", U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "", NULL, NULL, "s", ppDeviceModel, U_CX_AT_UTIL_PARAM_LAST);
    return ret > 0;
}

bool uCxBeginGeneralGetSoftwareVersion(uCxHandle_t * puCxHandle, const char ** ppVersion)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+GMR", "", U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "", NULL, NULL, "s", ppVersion, U_CX_AT_UTIL_PARAM_LAST);
    return ret > 0;
}

bool uCxBeginGeneralGetSerialNumber(uCxHandle_t * puCxHandle, const char ** ppSerialNumber)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+GSN", "", U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "", NULL, NULL, "s", ppSerialNumber, U_CX_AT_UTIL_PARAM_LAST);
    return ret > 0;
}
