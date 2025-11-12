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
#include "u_cx_network_time.h"

int32_t uCxNetworkTimeSetClientEnabled(uCxHandle_t * puCxHandle, uEnable_t enable)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UNTE=", "d", enable, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxNetworkTimeGetClientEnabled(uCxHandle_t * puCxHandle, uEnable_t * pEnable)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UNTE?", "", U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UNTE:", NULL, NULL, "d", pEnable, U_CX_AT_UTIL_PARAM_LAST);
    {
        // Always call uCxAtClientCmdEnd() even if any previous function failed
        int32_t endRet = uCxAtClientCmdEnd(pAtClient);
        if (ret >= 0) {
            ret = endRet;
        }
    }
    return ret;
}

int32_t uCxNetworkTimeSetNtpServer(uCxHandle_t * puCxHandle, int32_t ntp_server_id, const char * ntp_server_address)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UNTSC=", "ds", ntp_server_id, ntp_server_address, U_CX_AT_UTIL_PARAM_LAST);
}

bool uCxNetworkTimeGetNtpServerBegin(uCxHandle_t * puCxHandle, uCxNetworkTimeGetNtpServer_t * pNetworkTimeGetNtpServerRsp)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UNTSC?", "", U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UNTSC:", NULL, NULL, "dsid", &pNetworkTimeGetNtpServerRsp->ntp_server_id, &pNetworkTimeGetNtpServerRsp->ntp_server_address, &pNetworkTimeGetNtpServerRsp->ntp_server_ip, &pNetworkTimeGetNtpServerRsp->reachable, U_CX_AT_UTIL_PARAM_LAST);
    return ret >= 0;
}
