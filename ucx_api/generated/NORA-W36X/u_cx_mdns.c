/*
* This file was manually created for NORA-W36 mDNS support.
* Will be auto-generated in future ucxclient versions.
*/
#include <string.h>
#include "u_cx_at_client.h"
#include "u_cx_mdns.h"

int32_t uCxMdnsStart(uCxHandle_t * puCxHandle)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UWMDNSS", "", U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxMdnsStop(uCxHandle_t * puCxHandle)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UWMDNSP", "", U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxMdnsConfigureService(uCxHandle_t * puCxHandle, const char * service_name, 
                                 const char * service_type, int32_t port)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    // Format: AT+UWMDNSC="<service_name>","<service_type>",<port>
    // Format string: "ss" for two quoted strings, "d" for decimal integer
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UWMDNSC=", "ssd", 
                                      service_name, service_type, port, 
                                      U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxMdnsClearServices(uCxHandle_t * puCxHandle)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UWMDNSCL", "", U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxMdnsListServicesBegin(uCxHandle_t * puCxHandle)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    // Start the read command (AT+UWMDNSC?)
    uCxAtClientCmdBeginF(pAtClient, "AT+UWMDNSC?", "", U_CX_AT_UTIL_PARAM_LAST);
    return 0;
}

int32_t uCxMdnsListServicesGetNext(uCxHandle_t * puCxHandle, uCxMdnsServiceInfo_t * pServiceInfo)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    
    // Response format: +UWMDNSC: <index>,"<service_name>","<service_type>",<port>
    // Format string: "d" for index, "ss" for two strings, "d" for port
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UWMDNSC:", NULL, NULL, 
                                       "dssd", 
                                       &pServiceInfo->index,
                                       &pServiceInfo->service_name,
                                       &pServiceInfo->service_type,
                                       &pServiceInfo->port,
                                       U_CX_AT_UTIL_PARAM_LAST);
    
    if (ret < 0) {
        // No more services or error - end the command
        int32_t endRet = uCxAtClientCmdEnd(pAtClient);
        if (ret >= 0) {
            ret = endRet;
        }
    }
    
    return ret;
}
