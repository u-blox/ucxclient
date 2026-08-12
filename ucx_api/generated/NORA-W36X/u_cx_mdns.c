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
#include "u_cx_mdns.h"

int32_t uCxMdnsSetEnabled(uCxHandle_t * puCxHandle, uEnabled_t enabled)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UMDE=", "d", enabled, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxMdnsGetEnabled(uCxHandle_t * puCxHandle, uEnabled_t * pEnabled)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UMDE?", "", U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UMDE:", NULL, NULL, "d", pEnabled, U_CX_AT_UTIL_PARAM_LAST);
    {
        // Always call uCxAtClientCmdEnd() even if any previous function failed
        int32_t endRet = uCxAtClientCmdEnd(pAtClient);
        if (ret >= 0) {
            ret = endRet;
        }
    }
    return ret;
}

int32_t uCxMdnsSetHostname(uCxHandle_t * puCxHandle, const char * mdns_hostname)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UMDHN=", "s", mdns_hostname, U_CX_AT_UTIL_PARAM_LAST);
}

bool uCxMdnsGetHostnameBegin(uCxHandle_t * puCxHandle, const char ** ppMdnsHostname)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UMDHN?", "", U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UMDHN:", NULL, NULL, "s", ppMdnsHostname, U_CX_AT_UTIL_PARAM_LAST);
    return ret >= 0;
}

int32_t uCxMdnsServiceAdd3(uCxHandle_t * puCxHandle, const char * service_type, uMdnsProtocol_t protocol, int32_t port, int32_t * pServiceId)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UMDSA=", "sdd", service_type, protocol, port, U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UMDSA:", NULL, NULL, "d", pServiceId, U_CX_AT_UTIL_PARAM_LAST);
    {
        // Always call uCxAtClientCmdEnd() even if any previous function failed
        int32_t endRet = uCxAtClientCmdEnd(pAtClient);
        if (ret >= 0) {
            ret = endRet;
        }
    }
    return ret;
}

int32_t uCxMdnsServiceAdd4(uCxHandle_t * puCxHandle, const char * service_type, uMdnsProtocol_t protocol, int32_t port, const char * instance_name, int32_t * pServiceId)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UMDSA=", "sdds", service_type, protocol, port, instance_name, U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UMDSA:", NULL, NULL, "d", pServiceId, U_CX_AT_UTIL_PARAM_LAST);
    {
        // Always call uCxAtClientCmdEnd() even if any previous function failed
        int32_t endRet = uCxAtClientCmdEnd(pAtClient);
        if (ret >= 0) {
            ret = endRet;
        }
    }
    return ret;
}

int32_t uCxMdnsServiceRemove(uCxHandle_t * puCxHandle, int32_t service_id)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UMDSR=", "d", service_id, U_CX_AT_UTIL_PARAM_LAST);
}

void uCxMdnsServiceListBegin(uCxHandle_t * puCxHandle)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    uCxAtClientCmdBeginF(pAtClient, "AT+UMDSL?", "", U_CX_AT_UTIL_PARAM_LAST);
}

bool uCxMdnsServiceListGetNext(uCxHandle_t * puCxHandle, uCxMdnsServiceList_t * pMdnsServiceListRsp)
{
    int32_t ret;
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UMDSL:", NULL, NULL, "dsdds", &pMdnsServiceListRsp->service_id, &pMdnsServiceListRsp->service_type, &pMdnsServiceListRsp->protocol, &pMdnsServiceListRsp->port, &pMdnsServiceListRsp->instance_name, U_CX_AT_UTIL_PARAM_LAST);
    return ret >= 0;
}

int32_t uCxMdnsTxtRecordAdd(uCxHandle_t * puCxHandle, int32_t service_id, const char * txt_key, const char * txt_value)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UMDTRA=", "dss", service_id, txt_key, txt_value, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxMdnsTxtRecordRemove(uCxHandle_t * puCxHandle, int32_t service_id, const char * txt_key)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UMDTRR=", "ds", service_id, txt_key, U_CX_AT_UTIL_PARAM_LAST);
}

void uCxMdnsTxtRecordListBegin(uCxHandle_t * puCxHandle, int32_t service_id)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    uCxAtClientCmdBeginF(pAtClient, "AT+UMDTRL=", "d", service_id, U_CX_AT_UTIL_PARAM_LAST);
}

bool uCxMdnsTxtRecordListGetNext(uCxHandle_t * puCxHandle, uCxMdnsTxtRecordList_t * pMdnsTxtRecordListRsp)
{
    int32_t ret;
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UMDTRL:", NULL, NULL, "dss", &pMdnsTxtRecordListRsp->service_id, &pMdnsTxtRecordListRsp->txt_key, &pMdnsTxtRecordListRsp->txt_value, U_CX_AT_UTIL_PARAM_LAST);
    return ret >= 0;
}

int32_t uCxMdnsTxtRecordClear(uCxHandle_t * puCxHandle, int32_t service_id)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UMDTRC=", "d", service_id, U_CX_AT_UTIL_PARAM_LAST);
}

void uCxMdnsRegisterMdnsUp(uCxHandle_t * puCxHandle, uUEMDU_t callback)
{
    puCxHandle->callbacks.UEMDU = callback;
}

void uCxMdnsRegisterMdnsDown(uCxHandle_t * puCxHandle, uUEMDD_t callback)
{
    puCxHandle->callbacks.UEMDD = callback;
}
