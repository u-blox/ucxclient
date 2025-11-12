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

int32_t uCxHttpSetTLS2(uCxHandle_t * puCxHandle, int32_t session_id, uTlsVersion_t tls_version)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UHTCTLS=", "dd", session_id, tls_version, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxHttpSetTLS3(uCxHandle_t * puCxHandle, int32_t session_id, uTlsVersion_t tls_version, const char * ca_name)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UHTCTLS=", "dds", session_id, tls_version, ca_name, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxHttpSetTLS5(uCxHandle_t * puCxHandle, int32_t session_id, uTlsVersion_t tls_version, const char * ca_name, const char * client_cert_name, const char * client_key_name)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UHTCTLS=", "ddsss", session_id, tls_version, ca_name, client_cert_name, client_key_name, U_CX_AT_UTIL_PARAM_LAST);
}

bool uCxHttpGetTLSBegin(uCxHandle_t * puCxHandle, int32_t session_id, uCxHttpGetTLS_t * pHttpGetTLSRsp)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UHTCTLS=", "d", session_id, U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UHTCTLS:", NULL, NULL, "ddsss", &pHttpGetTLSRsp->session_id, &pHttpGetTLSRsp->tls_version, &pHttpGetTLSRsp->ca_name, &pHttpGetTLSRsp->client_cert_name, &pHttpGetTLSRsp->client_key_name, U_CX_AT_UTIL_PARAM_LAST);
    return ret >= 0;
}

int32_t uCxHttpDisconnect(uCxHandle_t * puCxHandle, int32_t session_id)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UHTCDC=", "d", session_id, U_CX_AT_UTIL_PARAM_LAST);
}

bool uCxHttpGetHeader1Begin(uCxHandle_t * puCxHandle, int32_t session_id, uCxHttpGetHeader_t * pHttpGetHeaderRsp)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UHTCGH=", "d", session_id, U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UHTCGH:", NULL, NULL, "dd$", &pHttpGetHeaderRsp->session_id, &pHttpGetHeaderRsp->more_to_read, &pHttpGetHeaderRsp->byte_array_data, U_CX_AT_UTIL_PARAM_LAST);
    return ret >= 0;
}

bool uCxHttpGetHeader2Begin(uCxHandle_t * puCxHandle, int32_t session_id, int32_t data_length, uCxHttpGetHeader_t * pHttpGetHeaderRsp)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UHTCGH=", "dd", session_id, data_length, U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UHTCGH:", NULL, NULL, "dd$", &pHttpGetHeaderRsp->session_id, &pHttpGetHeaderRsp->more_to_read, &pHttpGetHeaderRsp->byte_array_data, U_CX_AT_UTIL_PARAM_LAST);
    return ret >= 0;
}

int32_t uCxHttpGetBody(uCxHandle_t * puCxHandle, int32_t session_id, int32_t data_length, int32_t * pMoreToRead)
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

int32_t uCxHttpAddHeaderField(uCxHandle_t * puCxHandle, int32_t session_id, const char * field_name, const char * field_value)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UHTCRHAF=", "dss", session_id, field_name, field_value, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxHttpSetCustomHeader(uCxHandle_t * puCxHandle, int32_t session_id, const char * header_data)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UHTCRHCS=", "ds", session_id, header_data, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxHttpClearHeaderCustom(uCxHandle_t * puCxHandle, int32_t session_id)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UHTCRHCC=", "d", session_id, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxHttpSetRequestPath(uCxHandle_t * puCxHandle, int32_t session_id, const char * path)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UHTCRP=", "ds", session_id, path, U_CX_AT_UTIL_PARAM_LAST);
}

bool uCxHttpGetRequestPathBegin(uCxHandle_t * puCxHandle, int32_t session_id, uCxHttpGetRequestPath_t * pHttpGetRequestPathRsp)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UHTCRP=", "d", session_id, U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UHTCRP:", NULL, NULL, "ds", &pHttpGetRequestPathRsp->session_id, &pHttpGetRequestPathRsp->path, U_CX_AT_UTIL_PARAM_LAST);
    return ret >= 0;
}

int32_t uCxHttpGetRequest(uCxHandle_t * puCxHandle, int32_t session_id)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UHTCRG=", "d", session_id, U_CX_AT_UTIL_PARAM_LAST);
}

bool uCxHttpGetGetRequestHeaderBegin(uCxHandle_t * puCxHandle, int32_t session_id, uCxHttpGetGetRequestHeader_t * pHttpGetGetRequestHeaderRsp)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UHTCRGH=", "d", session_id, U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UHTCRGH:", NULL, NULL, "ds", &pHttpGetGetRequestHeaderRsp->session_id, &pHttpGetGetRequestHeaderRsp->header_data, U_CX_AT_UTIL_PARAM_LAST);
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

bool uCxHttpGetDeleteRequestHeaderBegin(uCxHandle_t * puCxHandle, int32_t session_id, uCxHttpGetDeleteRequestHeader_t * pHttpGetDeleteRequestHeaderRsp)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UHTCRDH=", "d", session_id, U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UHTCRDH:", NULL, NULL, "ds", &pHttpGetDeleteRequestHeaderRsp->session_id, &pHttpGetDeleteRequestHeaderRsp->header_data, U_CX_AT_UTIL_PARAM_LAST);
    return ret >= 0;
}

int32_t uCxHttpPostRequest(uCxHandle_t * puCxHandle, int32_t session_id, const uint8_t * binary_data, int32_t binary_data_len, uCxHttpPostRequest_t * pHttpPostRequestRsp)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UHTCRPOB=", "dB", session_id, binary_data, binary_data_len, U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UHTCRPOB:", NULL, NULL, "dd", &pHttpPostRequestRsp->session_id, &pHttpPostRequestRsp->written_length, U_CX_AT_UTIL_PARAM_LAST);
    {
        // Always call uCxAtClientCmdEnd() even if any previous function failed
        int32_t endRet = uCxAtClientCmdEnd(pAtClient);
        if (ret >= 0) {
            ret = endRet;
        }
    }
    return ret;
}

bool uCxHttpGetPostRequestHeaderBegin(uCxHandle_t * puCxHandle, int32_t session_id, uCxHttpGetPostRequestHeader_t * pHttpGetPostRequestHeaderRsp)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UHTCRPOH=", "d", session_id, U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UHTCRPOH:", NULL, NULL, "ds", &pHttpGetPostRequestHeaderRsp->session_id, &pHttpGetPostRequestHeaderRsp->header_data, U_CX_AT_UTIL_PARAM_LAST);
    return ret >= 0;
}

int32_t uCxHttpPutRequest(uCxHandle_t * puCxHandle, int32_t session_id, const uint8_t * binary_data, int32_t binary_data_len, uCxHttpPutRequest_t * pHttpPutRequestRsp)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UHTCRPUB=", "dB", session_id, binary_data, binary_data_len, U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UHTCRPUB:", NULL, NULL, "dd", &pHttpPutRequestRsp->session_id, &pHttpPutRequestRsp->written_length, U_CX_AT_UTIL_PARAM_LAST);
    {
        // Always call uCxAtClientCmdEnd() even if any previous function failed
        int32_t endRet = uCxAtClientCmdEnd(pAtClient);
        if (ret >= 0) {
            ret = endRet;
        }
    }
    return ret;
}

bool uCxHttpGetPutRequestHeaderBegin(uCxHandle_t * puCxHandle, int32_t session_id, uCxHttpGetPutRequestHeader_t * pHttpGetPutRequestHeaderRsp)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UHTCRPUH=", "d", session_id, U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UHTCRPUH:", NULL, NULL, "ds", &pHttpGetPutRequestHeaderRsp->session_id, &pHttpGetPutRequestHeaderRsp->header_data, U_CX_AT_UTIL_PARAM_LAST);
    return ret >= 0;
}

void uCxHttpRegisterDisconnect(uCxHandle_t * puCxHandle, uUEHTCDC_t callback)
{
    puCxHandle->callbacks.UEHTCDC = callback;
}

void uCxHttpRegisterRequestStatus(uCxHandle_t * puCxHandle, uUEHTCRS_t callback)
{
    puCxHandle->callbacks.UEHTCRS = callback;
}
