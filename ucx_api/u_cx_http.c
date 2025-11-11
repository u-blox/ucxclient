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
#include "u_cx_http.h"

int32_t uCxHttpSetConnectionParams2(uCxHandle_t * puCxHandle, int32_t session_id, const char * host)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UHTCCP=", "ds", session_id, host, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxHttpSetConnectionParams3(uCxHandle_t * puCxHandle, int32_t session_id, const char * host, int32_t port)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UHTCCP=", "dsd", session_id, host, port, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxHttpSetHttpTLS2(uCxHandle_t * puCxHandle, int32_t session_id, uTlsVersion_t tls_version)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UHTCTLS=", "dd", session_id, tls_version, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxHttpSetHttpTLS3(uCxHandle_t * puCxHandle, int32_t session_id, uTlsVersion_t tls_version, const char * ca_name)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UHTCTLS=", "dds", session_id, tls_version, ca_name, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxHttpSetHttpTLS5(uCxHandle_t * puCxHandle, int32_t session_id, uTlsVersion_t tls_version, const char * ca_name, const char * client_cert_name, const char * client_key_name)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UHTCTLS=", "ddsss", session_id, tls_version, ca_name, client_cert_name, client_key_name, U_CX_AT_UTIL_PARAM_LAST);
}

bool uCxHttpGetHttpTlsBegin(uCxHandle_t * puCxHandle, int32_t session_id, uCxHttpGetHttpTls_t * pHttpGetHttpTlsRsp)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UHTCTLS=", "d", session_id, U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UHTCTLS:", NULL, NULL, "ddsss", &pHttpGetHttpTlsRsp->session_id, &pHttpGetHttpTlsRsp->tls_version, &pHttpGetHttpTlsRsp->ca_name, &pHttpGetHttpTlsRsp->client_cert_name, &pHttpGetHttpTlsRsp->client_key_name, U_CX_AT_UTIL_PARAM_LAST);
    return ret >= 0;
}

int32_t uCxHttpDisconnectHttpSession(uCxHandle_t * puCxHandle, int32_t session_id)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UHTCDC=", "d", session_id, U_CX_AT_UTIL_PARAM_LAST);
}

bool uCxHttpHeaderGet1Begin(uCxHandle_t * puCxHandle, int32_t session_id, uCxHttpHeaderGet_t * pHttpHeaderGetRsp)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UHTCGH=", "d", session_id, U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UHTCGH:", NULL, NULL, "dd$", &pHttpHeaderGetRsp->session_id, &pHttpHeaderGetRsp->more_to_read, &pHttpHeaderGetRsp->byte_array_data, U_CX_AT_UTIL_PARAM_LAST);
    return ret >= 0;
}

bool uCxHttpHeaderGet2Begin(uCxHandle_t * puCxHandle, int32_t session_id, int32_t data_length, uCxHttpHeaderGet_t * pHttpHeaderGetRsp)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UHTCGH=", "dd", session_id, data_length, U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UHTCGH:", NULL, NULL, "dd$", &pHttpHeaderGetRsp->session_id, &pHttpHeaderGetRsp->more_to_read, &pHttpHeaderGetRsp->byte_array_data, U_CX_AT_UTIL_PARAM_LAST);
    return ret >= 0;
}

int32_t uCxHttpGetHttpBodyBinary(uCxHandle_t * puCxHandle, int32_t session_id, int32_t data_length, int32_t * pMoreToRead)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UHTCGBB=", "dd", session_id, data_length, U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UHTCGBB:", NULL, NULL, "-d", pMoreToRead, U_CX_AT_UTIL_PARAM_LAST);
    {
        // Always call uCxAtClientCmdEnd() even if any previous function failed
        int32_t endRet = uCxAtClientCmdEnd(pAtClient);
        if (ret >= 0) {
            ret = endRet;
        }
    }
    return ret;
}

bool uCxHttpGetHttpBodyStringBegin(uCxHandle_t * puCxHandle, int32_t session_id, int32_t data_length, uCxHttpGetHttpBodyString_t * pHttpGetHttpBodyStringRsp)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UHTCGBS=", "dd", session_id, data_length, U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UHTCGBS:", NULL, NULL, "-dd$", &pHttpGetHttpBodyStringRsp->more_to_read, &pHttpGetHttpBodyStringRsp->data_length, &pHttpGetHttpBodyStringRsp->byte_array_data, U_CX_AT_UTIL_PARAM_LAST);
    return ret >= 0;
}

int32_t uCxHttpHeaderAddField(uCxHandle_t * puCxHandle, int32_t session_id, const char * field_name, const char * field_value)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UHTCRHAF=", "dss", session_id, field_name, field_value, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxHttpHeaderSetCustom(uCxHandle_t * puCxHandle, int32_t session_id, const char * header_data)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UHTCRHCS=", "ds", session_id, header_data, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxHttpHeaderClearCustom(uCxHandle_t * puCxHandle, int32_t session_id)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UHTCRHCC=", "d", session_id, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxHttpHttpSetPath(uCxHandle_t * puCxHandle, int32_t session_id, const char * path)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UHTCRP=", "ds", session_id, path, U_CX_AT_UTIL_PARAM_LAST);
}

bool uCxHttpHttpGetPathBegin(uCxHandle_t * puCxHandle, int32_t session_id, uCxHttpHttpGetPath_t * pHttpHttpGetPathRsp)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UHTCRP=", "d", session_id, U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UHTCRP:", NULL, NULL, "ds", &pHttpHttpGetPathRsp->session_id, &pHttpHttpGetPathRsp->path, U_CX_AT_UTIL_PARAM_LAST);
    return ret >= 0;
}

int32_t uCxHttpGetRequest(uCxHandle_t * puCxHandle, int32_t session_id)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UHTCRG=", "d", session_id, U_CX_AT_UTIL_PARAM_LAST);
}

bool uCxHttpGetRequestHeaderBegin(uCxHandle_t * puCxHandle, int32_t session_id, uCxHttpGetRequestHeader_t * pHttpGetRequestHeaderRsp)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UHTCRGH=", "d", session_id, U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UHTCRGH:", NULL, NULL, "ds", &pHttpGetRequestHeaderRsp->session_id, &pHttpGetRequestHeaderRsp->header_data, U_CX_AT_UTIL_PARAM_LAST);
    return ret >= 0;
}

int32_t uCxHttpDeleteRequest1(uCxHandle_t * puCxHandle, int32_t session_id)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UHTCRD=", "d", session_id, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxHttpDeleteRequest2(uCxHandle_t * puCxHandle, int32_t session_id, const char * byte_array_data, int32_t byte_array_data_len)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UHTCRD=", "d$", session_id, byte_array_data, byte_array_data_len, U_CX_AT_UTIL_PARAM_LAST);
}

bool uCxHttpDeleteRequestHeaderBegin(uCxHandle_t * puCxHandle, int32_t session_id, uCxHttpDeleteRequestHeader_t * pHttpDeleteRequestHeaderRsp)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UHTCRDH=", "d", session_id, U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UHTCRDH:", NULL, NULL, "ds", &pHttpDeleteRequestHeaderRsp->session_id, &pHttpDeleteRequestHeaderRsp->header_data, U_CX_AT_UTIL_PARAM_LAST);
    return ret >= 0;
}

int32_t uCxHttpPostRequestString(uCxHandle_t * puCxHandle, int32_t session_id, const char * byte_array_data, int32_t byte_array_data_len)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UHTCRPOS=", "d$", session_id, byte_array_data, byte_array_data_len, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxHttpPostRequestBinary(uCxHandle_t * puCxHandle, int32_t session_id, const uint8_t * binary_data, int32_t binary_data_len, uCxHttpPostRequestBinary_t * pHttpPostRequestBinaryRsp)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UHTCRPOB=", "dB", session_id, binary_data, binary_data_len, U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UHTCRPOB:", NULL, NULL, "dd", &pHttpPostRequestBinaryRsp->session_id, &pHttpPostRequestBinaryRsp->written_length, U_CX_AT_UTIL_PARAM_LAST);
    {
        // Always call uCxAtClientCmdEnd() even if any previous function failed
        int32_t endRet = uCxAtClientCmdEnd(pAtClient);
        if (ret >= 0) {
            ret = endRet;
        }
    }
    return ret;
}

bool uCxHttpPostRequestHeaderBegin(uCxHandle_t * puCxHandle, int32_t session_id, uCxHttpPostRequestHeader_t * pHttpPostRequestHeaderRsp)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UHTCRPOH=", "d", session_id, U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UHTCRPOH:", NULL, NULL, "ds", &pHttpPostRequestHeaderRsp->session_id, &pHttpPostRequestHeaderRsp->header_data, U_CX_AT_UTIL_PARAM_LAST);
    return ret >= 0;
}

int32_t uCxHttpPutRequestString(uCxHandle_t * puCxHandle, int32_t session_id, const char * byte_array_data, int32_t byte_array_data_len)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UHTCRPUS=", "d$", session_id, byte_array_data, byte_array_data_len, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxHttpPutRequestBinary(uCxHandle_t * puCxHandle, int32_t session_id, const uint8_t * binary_data, int32_t binary_data_len, uCxHttpPutRequestBinary_t * pHttpPutRequestBinaryRsp)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UHTCRPUB=", "dB", session_id, binary_data, binary_data_len, U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UHTCRPUB:", NULL, NULL, "dd", &pHttpPutRequestBinaryRsp->session_id, &pHttpPutRequestBinaryRsp->written_length, U_CX_AT_UTIL_PARAM_LAST);
    {
        // Always call uCxAtClientCmdEnd() even if any previous function failed
        int32_t endRet = uCxAtClientCmdEnd(pAtClient);
        if (ret >= 0) {
            ret = endRet;
        }
    }
    return ret;
}

bool uCxHttpPutRequestHeaderBegin(uCxHandle_t * puCxHandle, int32_t session_id, uCxHttpPutRequestHeader_t * pHttpPutRequestHeaderRsp)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UHTCRPUH=", "d", session_id, U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UHTCRPUH:", NULL, NULL, "ds", &pHttpPutRequestHeaderRsp->session_id, &pHttpPutRequestHeaderRsp->header_data, U_CX_AT_UTIL_PARAM_LAST);
    return ret >= 0;
}

void uCxHttpRegisterDisconnect(uCxHandle_t * puCxHandle, uUEHTCDC_t callback)
{
    puCxHandle->callbacks.UEHTCDC = callback;
}

void uCxHttpRegisterHttpRequestStatus(uCxHandle_t * puCxHandle, uUEHTCRS_t callback)
{
    puCxHandle->callbacks.UEHTCRS = callback;
}
