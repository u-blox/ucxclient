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
#include "u_cx_channel_sounding.h"

int32_t uCxChannelSoundingSetChannelSoundingMode(uCxHandle_t * puCxHandle, uBtChannelSoundingMode_t channel_sounding_mode)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UBTCSM=", "d", channel_sounding_mode, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxChannelSoundingGetChannelSoundingMode(uCxHandle_t * puCxHandle, uBtChannelSoundingMode_t * pChannelSoundingMode)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UBTCSM?", "", U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UBTCSM:", NULL, NULL, "d", pChannelSoundingMode, U_CX_AT_UTIL_PARAM_LAST);
    {
        // Always call uCxAtClientCmdEnd() even if any previous function failed
        int32_t endRet = uCxAtClientCmdEnd(pAtClient);
        if (ret >= 0) {
            ret = endRet;
        }
    }
    return ret;
}

void uCxChannelSoundingRegisterCsStatus(uCxHandle_t * puCxHandle, uUEBTCSS_t callback)
{
    puCxHandle->callbacks.UEBTCSS = callback;
}
