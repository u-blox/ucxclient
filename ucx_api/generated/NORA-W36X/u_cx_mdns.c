/*
 * NORA-W36 mDNS ucxclient API — firmware v3.4.0+
 *
 * Official mDNS AT commands (chapter 13 of AT command manual).
 * Replaces the previous proprietary AT+UWMDNS* commands.
 */
#include <string.h>
#include "u_cx_at_client.h"
#include "u_cx_mdns.h"

/* ---- responder control ---- */

int32_t uCxMdnsEnable(uCxHandle_t * puCxHandle, bool enable)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UMDNSE=", "d",
                                      (int32_t)(enable ? 1 : 0),
                                      U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxMdnsSetHostname(uCxHandle_t * puCxHandle, const char * hostname)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UMDNSHN=", "s",
                                      hostname, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxMdnsSetIpVersion(uCxHandle_t * puCxHandle, uCxMdnsIpVersion_t ipVersion)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UMDNSIP=", "d",
                                      (int32_t)ipVersion,
                                      U_CX_AT_UTIL_PARAM_LAST);
}

/* ---- service management ---- */

int32_t uCxMdnsServiceAdd(uCxHandle_t * puCxHandle, const char * serviceType,
                           uCxMdnsProtocol_t protocol, int32_t port,
                           const char * instanceName, int32_t * pServiceId)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;

    /* AT+UMDNSSA=<service_type>,<protocol>,<port>[,<instance_name>] */
    if (instanceName && instanceName[0] != '\0')
    {
        uCxAtClientCmdBeginF(pAtClient, "AT+UMDNSSA=", "sdds",
                             serviceType, (int32_t)protocol, port, instanceName,
                             U_CX_AT_UTIL_PARAM_LAST);
    }
    else
    {
        uCxAtClientCmdBeginF(pAtClient, "AT+UMDNSSA=", "sdd",
                             serviceType, (int32_t)protocol, port,
                             U_CX_AT_UTIL_PARAM_LAST);
    }

    /* Response: +UMDNSSA:<service_id> */
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UMDNSSA:", NULL, NULL,
                                       "d", pServiceId,
                                       U_CX_AT_UTIL_PARAM_LAST);
    if (ret >= 0)
    {
        ret = uCxAtClientCmdEnd(pAtClient);
    }
    else
    {
        (void)uCxAtClientCmdEnd(pAtClient);
    }

    return ret;
}

int32_t uCxMdnsServiceRemove(uCxHandle_t * puCxHandle, int32_t serviceId)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UMDNSSR=", "d",
                                      serviceId, U_CX_AT_UTIL_PARAM_LAST);
}

/* ---- TXT record management ---- */

int32_t uCxMdnsTxtRecordAdd(uCxHandle_t * puCxHandle, int32_t serviceId,
                             const char * key, const char * value)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    /* AT+UMDNSTRA=<service_id>,<txt_key>,<txt_value> */
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UMDNSTRA=", "dss",
                                      serviceId, key, value ? value : "",
                                      U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxMdnsTxtRecordRemove(uCxHandle_t * puCxHandle, int32_t serviceId,
                                const char * key)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UMDNSTRR=", "ds",
                                      serviceId, key,
                                      U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxMdnsTxtRecordClear(uCxHandle_t * puCxHandle, int32_t serviceId)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UMDNSTRC=", "d",
                                      serviceId, U_CX_AT_UTIL_PARAM_LAST);
}
