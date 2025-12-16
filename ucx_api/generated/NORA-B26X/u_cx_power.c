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

int32_t uCxPowerDeepSleep(uCxHandle_t * puCxHandle, uPowerWakeupMode_t wakeup_mode)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UPMDS=", "d", wakeup_mode, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxPowerSleep(uCxHandle_t * puCxHandle)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UPMS", "", U_CX_AT_UTIL_PARAM_LAST);
}
