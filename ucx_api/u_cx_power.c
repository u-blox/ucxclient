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
#include "u_cx_power.h"

int32_t uCxPowerDeepSleepWithGpioWakeup(uCxHandle_t * puCxHandle)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UPMDS", "", U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxPowerDeepSleep(uCxHandle_t * puCxHandle, uWakeupMode_t wakeup_mode)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UPMDS=", "d", wakeup_mode, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxPowerSetPowerSaveLevel(uCxHandle_t * puCxHandle, int32_t level)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UPMPSL=", "d", level, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxPowerGetPowerSaveLevel(uCxHandle_t * puCxHandle, int32_t * pLevel)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UPMPSL?", "", U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UPMPSL:", NULL, NULL, "d", pLevel, U_CX_AT_UTIL_PARAM_LAST);
    {
        // Always call uCxAtClientCmdEnd() even if any previous function failed
        int32_t endRet = uCxAtClientCmdEnd(pAtClient);
        if (ret >= 0) {
            ret = endRet;
        }
    }
    return ret;
}

int32_t uCxPowerSetPowerSaveTimeout(uCxHandle_t * puCxHandle, int32_t timeoutMs)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UPMPSTO=", "d", timeoutMs, U_CX_AT_UTIL_PARAM_LAST);
}
