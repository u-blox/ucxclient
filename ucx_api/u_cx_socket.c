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
#include "u_cx_socket.h"

int32_t uCxSocketCreate1(uCxHandle_t * puCxHandle, uProtocol_t protocol, int32_t * pSocketHandle)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+USOCR=", "d", protocol, U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+USOCR:", NULL, NULL, "d", pSocketHandle, U_CX_AT_UTIL_PARAM_LAST);
    {
        // Always call uCxAtClientCmdEnd() even if any previous function failed
        int32_t endRet = uCxAtClientCmdEnd(pAtClient);
        if (ret >= 0) {
            ret = endRet;
        }
    }
    return ret;
}

int32_t uCxSocketCreate2(uCxHandle_t * puCxHandle, uProtocol_t protocol, uPreferredProtocolType_t preferred_protocol_type, int32_t * pSocketHandle)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+USOCR=", "dd", protocol, preferred_protocol_type, U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+USOCR:", NULL, NULL, "d", pSocketHandle, U_CX_AT_UTIL_PARAM_LAST);
    {
        // Always call uCxAtClientCmdEnd() even if any previous function failed
        int32_t endRet = uCxAtClientCmdEnd(pAtClient);
        if (ret >= 0) {
            ret = endRet;
        }
    }
    return ret;
}

int32_t uCxSocketSetTLS2(uCxHandle_t * puCxHandle, int32_t socket_handle, uTlsVersion_t tls_version)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+USOTLS=", "dd", socket_handle, tls_version, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxSocketSetTLS3(uCxHandle_t * puCxHandle, int32_t socket_handle, uTlsVersion_t tls_version, const char * ca_name)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+USOTLS=", "dds", socket_handle, tls_version, ca_name, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxSocketSetTLS5(uCxHandle_t * puCxHandle, int32_t socket_handle, uTlsVersion_t tls_version, const char * ca_name, const char * client_cert_name, const char * client_key_name)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+USOTLS=", "ddsss", socket_handle, tls_version, ca_name, client_cert_name, client_key_name, U_CX_AT_UTIL_PARAM_LAST);
}

bool uCxSocketGetTLSBegin(uCxHandle_t * puCxHandle, int32_t socket_handle, uCxSocketGetTLS_t * pSocketGetTLSRsp)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+USOTLS=", "d", socket_handle, U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+USOTLS:", NULL, NULL, "ddsss", &pSocketGetTLSRsp->socket_handle, &pSocketGetTLSRsp->tls_version, &pSocketGetTLSRsp->ca_name, &pSocketGetTLSRsp->client_cert_name, &pSocketGetTLSRsp->client_key_name, U_CX_AT_UTIL_PARAM_LAST);
    return ret >= 0;
}

int32_t uCxSocketConnect(uCxHandle_t * puCxHandle, int32_t socket_handle, const char * host_address, int32_t remote_port)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+USOC=", "dsd", socket_handle, host_address, remote_port, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxSocketSetReadMode(uCxHandle_t * puCxHandle, uReadMode_t read_mode)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+USORM=", "d", read_mode, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxSocketGetReadMode(uCxHandle_t * puCxHandle, uReadMode_t * pReadMode)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+USORM?", "", U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+USORM:", NULL, NULL, "d", pReadMode, U_CX_AT_UTIL_PARAM_LAST);
    {
        // Always call uCxAtClientCmdEnd() even if any previous function failed
        int32_t endRet = uCxAtClientCmdEnd(pAtClient);
        if (ret >= 0) {
            ret = endRet;
        }
    }
    return ret;
}

int32_t uCxSocketWrite(uCxHandle_t * puCxHandle, int32_t socket_handle, const uint8_t * binary_data, int32_t binary_data_len)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t written_length;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+USOWB=", "dB", socket_handle, binary_data, binary_data_len, U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+USOWB:", NULL, NULL, "-d", &written_length, U_CX_AT_UTIL_PARAM_LAST);
    {
        // Always call uCxAtClientCmdEnd() even if any previous function failed
        int32_t endRet = uCxAtClientCmdEnd(pAtClient);
        if (ret >= 0) {
            ret = endRet;
        }
    }
    if (ret >= 0) {
        ret = written_length;
    }
    return ret;
}

int32_t uCxSocketClose(uCxHandle_t * puCxHandle, int32_t socket_handle)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+USOCL=", "d", socket_handle, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxSocketRead(uCxHandle_t * puCxHandle, int32_t socket_handle, int32_t length, uint8_t * pDataBuf)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    uint8_t *pBinBuffer = pDataBuf;
    size_t binBufferLen = length;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+USORB=", "dd", socket_handle, length, U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+USORB:", pBinBuffer, &binBufferLen, "-", U_CX_AT_UTIL_PARAM_LAST);
    {
        // Always call uCxAtClientCmdEnd() even if any previous function failed
        int32_t endRet = uCxAtClientCmdEnd(pAtClient);
        if (ret >= 0) {
            ret = endRet;
        }
    }
    if (ret >= 0) {
        ret = binBufferLen;
    }
    return ret;
}

int32_t uCxSocketGetLastError(uCxHandle_t * puCxHandle, int32_t * pErrorCode)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+USOE", "", U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+USOE:", NULL, NULL, "d", pErrorCode, U_CX_AT_UTIL_PARAM_LAST);
    {
        // Always call uCxAtClientCmdEnd() even if any previous function failed
        int32_t endRet = uCxAtClientCmdEnd(pAtClient);
        if (ret >= 0) {
            ret = endRet;
        }
    }
    return ret;
}

int32_t uCxSocketBind(uCxHandle_t * puCxHandle, int32_t socket_handle, int32_t local_port)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+USOB=", "dd", socket_handle, local_port, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxSocketListen1(uCxHandle_t * puCxHandle, int32_t socket_handle)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+USOL=", "d", socket_handle, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxSocketListen2(uCxHandle_t * puCxHandle, int32_t socket_handle, int32_t port)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+USOL=", "dd", socket_handle, port, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxSocketReceiveFrom(uCxHandle_t * puCxHandle, int32_t socket_handle, int32_t length, uint8_t * pDataBuf, uCxSocketReceiveFrom_t * pSocketReceiveFromRsp)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    uint8_t *pBinBuffer = pDataBuf;
    size_t binBufferLen = length;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+USORFB=", "dd", socket_handle, length, U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+USORFB:", pBinBuffer, &binBufferLen, "-id", &pSocketReceiveFromRsp->remote_ip, &pSocketReceiveFromRsp->remote_port, U_CX_AT_UTIL_PARAM_LAST);
    {
        // Always call uCxAtClientCmdEnd() even if any previous function failed
        int32_t endRet = uCxAtClientCmdEnd(pAtClient);
        if (ret >= 0) {
            ret = endRet;
        }
    }
    if (ret >= 0) {
        ret = binBufferLen;
    }
    return ret;
}

int32_t uCxSocketGetPeerAddress(uCxHandle_t * puCxHandle, int32_t socket_handle, uCxSocketGetPeerAddress_t * pSocketGetPeerAddressRsp)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+USOPA=", "d", socket_handle, U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+USOPA:", NULL, NULL, "-id", &pSocketGetPeerAddressRsp->remote_ip, &pSocketGetPeerAddressRsp->remote_port, U_CX_AT_UTIL_PARAM_LAST);
    {
        // Always call uCxAtClientCmdEnd() even if any previous function failed
        int32_t endRet = uCxAtClientCmdEnd(pAtClient);
        if (ret >= 0) {
            ret = endRet;
        }
    }
    return ret;
}

void uCxSocketListStatusBegin(uCxHandle_t * puCxHandle)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    uCxAtClientCmdBeginF(pAtClient, "AT+USOST?", "", U_CX_AT_UTIL_PARAM_LAST);
}

bool uCxSocketListStatusGetNext(uCxHandle_t * puCxHandle, uCxSocketListStatus_t * pSocketListStatusRsp)
{
    int32_t ret;
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+USOST:", NULL, NULL, "ddd", &pSocketListStatusRsp->socket_handle, &pSocketListStatusRsp->protocol, &pSocketListStatusRsp->socket_status, U_CX_AT_UTIL_PARAM_LAST);
    return ret >= 0;
}

int32_t uCxSocketGetStatus(uCxHandle_t * puCxHandle, int32_t socket_handle, uCxSocketGetStatus_t * pSocketGetStatusRsp)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+USOST=", "d", socket_handle, U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+USOST:", NULL, NULL, "-dd", &pSocketGetStatusRsp->protocol, &pSocketGetStatusRsp->socket_status, U_CX_AT_UTIL_PARAM_LAST);
    {
        // Always call uCxAtClientCmdEnd() even if any previous function failed
        int32_t endRet = uCxAtClientCmdEnd(pAtClient);
        if (ret >= 0) {
            ret = endRet;
        }
    }
    return ret;
}

int32_t uCxSocketSetOption(uCxHandle_t * puCxHandle, int32_t socket_handle, uOption_t option, int32_t value)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+USOO=", "ddd", socket_handle, option, value, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxSocketGetOption(uCxHandle_t * puCxHandle, int32_t socket_handle, uOption_t option, int32_t * pValue)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+USOO=", "dd", socket_handle, option, U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+USOO:", NULL, NULL, "--d", pValue, U_CX_AT_UTIL_PARAM_LAST);
    {
        // Always call uCxAtClientCmdEnd() even if any previous function failed
        int32_t endRet = uCxAtClientCmdEnd(pAtClient);
        if (ret >= 0) {
            ret = endRet;
        }
    }
    return ret;
}

int32_t uCxSocketGetHostByName(uCxHandle_t * puCxHandle, const char * host_name, uSockIpAddress_t * pHostIp)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+USOH=", "s", host_name, U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+USOH:", NULL, NULL, "i", pHostIp, U_CX_AT_UTIL_PARAM_LAST);
    {
        // Always call uCxAtClientCmdEnd() even if any previous function failed
        int32_t endRet = uCxAtClientCmdEnd(pAtClient);
        if (ret >= 0) {
            ret = endRet;
        }
    }
    return ret;
}

void uCxSocketRegisterConnect(uCxHandle_t * puCxHandle, uUESOC_t callback)
{
    puCxHandle->callbacks.UESOC = callback;
}

void uCxSocketRegisterDataAvailable(uCxHandle_t * puCxHandle, uUESODA_t callback)
{
    puCxHandle->callbacks.UESODA = callback;
}

void uCxSocketRegisterClosed(uCxHandle_t * puCxHandle, uUESOCL_t callback)
{
    puCxHandle->callbacks.UESOCL = callback;
}

void uCxSocketRegisterIncomingConnection(uCxHandle_t * puCxHandle, uUESOIC_t callback)
{
    puCxHandle->callbacks.UESOIC = callback;
}
