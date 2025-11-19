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
#include "u_cx_mqtt.h"

int32_t uCxMqttSetConnectionParams3(uCxHandle_t * puCxHandle, int32_t mqtt_id, const char * hostname, int32_t port)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UMQCP=", "dsd", mqtt_id, hostname, port, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxMqttSetConnectionParams4(uCxHandle_t * puCxHandle, int32_t mqtt_id, const char * hostname, int32_t port, const char * client_id)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UMQCP=", "dsds", mqtt_id, hostname, port, client_id, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxMqttSetConnectionParams5(uCxHandle_t * puCxHandle, int32_t mqtt_id, const char * hostname, int32_t port, const char * client_id, const char * username)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UMQCP=", "dsdss", mqtt_id, hostname, port, client_id, username, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxMqttSetConnectionParams6(uCxHandle_t * puCxHandle, int32_t mqtt_id, const char * hostname, int32_t port, const char * client_id, const char * username, const char * password)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UMQCP=", "dsdsss", mqtt_id, hostname, port, client_id, username, password, U_CX_AT_UTIL_PARAM_LAST);
}

bool uCxMqttGetConnectionParamsBegin(uCxHandle_t * puCxHandle, int32_t mqtt_id, uCxMqttGetConnectionParams_t * pMqttGetConnectionParamsRsp)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UMQCP=", "d", mqtt_id, U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UMQCP:", NULL, NULL, "-sdss", &pMqttGetConnectionParamsRsp->hostname, &pMqttGetConnectionParamsRsp->port, &pMqttGetConnectionParamsRsp->client_id, &pMqttGetConnectionParamsRsp->username, U_CX_AT_UTIL_PARAM_LAST);
    return ret >= 0;
}

int32_t uCxMqttConnect(uCxHandle_t * puCxHandle, int32_t mqtt_id)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UMQC=", "d", mqtt_id, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxMqttSetKeepAlive(uCxHandle_t * puCxHandle, int32_t mqtt_id, int32_t keep_alive)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UMQKA=", "dd", mqtt_id, keep_alive, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxMqttGetKeepAlive(uCxHandle_t * puCxHandle, int32_t mqtt_id, int32_t * pKeepAlive)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UMQKA=", "d", mqtt_id, U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UMQKA:", NULL, NULL, "-d", pKeepAlive, U_CX_AT_UTIL_PARAM_LAST);
    {
        // Always call uCxAtClientCmdEnd() even if any previous function failed
        int32_t endRet = uCxAtClientCmdEnd(pAtClient);
        if (ret >= 0) {
            ret = endRet;
        }
    }
    return ret;
}

int32_t uCxMqttSetLastWillAndTestament3(uCxHandle_t * puCxHandle, int32_t mqtt_id, const char * topic, const char * will_msg)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UMQLWT=", "dss", mqtt_id, topic, will_msg, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxMqttSetLastWillAndTestament4(uCxHandle_t * puCxHandle, int32_t mqtt_id, const char * topic, const char * will_msg, uMqttQos_t qos)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UMQLWT=", "dssd", mqtt_id, topic, will_msg, qos, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxMqttSetLastWillAndTestament5(uCxHandle_t * puCxHandle, int32_t mqtt_id, const char * topic, const char * will_msg, uMqttQos_t qos, uMqttRetain_t retain)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UMQLWT=", "dssdd", mqtt_id, topic, will_msg, qos, retain, U_CX_AT_UTIL_PARAM_LAST);
}

bool uCxMqttGetLastWillAndTestamentBegin(uCxHandle_t * puCxHandle, int32_t mqtt_id, uCxMqttGetLastWillAndTestament_t * pMqttGetLastWillAndTestamentRsp)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UMQLWT=", "d", mqtt_id, U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UMQLWT:", NULL, NULL, "-ssdd", &pMqttGetLastWillAndTestamentRsp->topic, &pMqttGetLastWillAndTestamentRsp->will_msg, &pMqttGetLastWillAndTestamentRsp->qos, &pMqttGetLastWillAndTestamentRsp->retain, U_CX_AT_UTIL_PARAM_LAST);
    return ret >= 0;
}

int32_t uCxMqttSetTLS2(uCxHandle_t * puCxHandle, int32_t mqtt_id, uWifiTlsVersion_t tls_version)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UMQTLS=", "dd", mqtt_id, tls_version, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxMqttSetTLS3(uCxHandle_t * puCxHandle, int32_t mqtt_id, uWifiTlsVersion_t tls_version, const char * ca_name)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UMQTLS=", "dds", mqtt_id, tls_version, ca_name, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxMqttSetTLS5(uCxHandle_t * puCxHandle, int32_t mqtt_id, uWifiTlsVersion_t tls_version, const char * ca_name, const char * client_cert_name, const char * client_key_name)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UMQTLS=", "ddsss", mqtt_id, tls_version, ca_name, client_cert_name, client_key_name, U_CX_AT_UTIL_PARAM_LAST);
}

bool uCxMqttGetTLSBegin(uCxHandle_t * puCxHandle, int32_t mqtt_id, uCxMqttGetTLS_t * pMqttGetTLSRsp)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    char *pParamsLine;
    int32_t rspSyntaxVal;
    size_t  paramsLen;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UMQTLS=", "d", mqtt_id, U_CX_AT_UTIL_PARAM_LAST);
    pParamsLine = uCxAtClientCmdGetRspParamLine(pAtClient, "+UMQTLS:", NULL, NULL);
    if (pParamsLine == NULL) {
        return false;
    }
    paramsLen = strlen(pParamsLine);
    if (uCxAtUtilParseParamsF(pParamsLine, "-d", &rspSyntaxVal) != 1) {
        return false;
    }
    uCxAtUtilReplaceChar(pParamsLine, paramsLen, 0, ',');
    switch (rspSyntaxVal)
    {
        case 1:
        case 2:
        case 3:
        case 4:
            pMqttGetTLSRsp->type = U_CX_MQTT_GET_TLS_RSP_TYPE_TLS_VERSION_STR_STR_STR;
            ret = uCxAtUtilParseParamsF(pParamsLine, "-dsss", &pMqttGetTLSRsp->rsp.TlsVersionStrStrStr.tls_version, &pMqttGetTLSRsp->rsp.TlsVersionStrStrStr.ca_name, &pMqttGetTLSRsp->rsp.TlsVersionStrStrStr.client_cert_name, &pMqttGetTLSRsp->rsp.TlsVersionStrStrStr.client_key_name, U_CX_AT_UTIL_PARAM_LAST);
            break;
        case 0:
            pMqttGetTLSRsp->type = U_CX_MQTT_GET_TLS_RSP_TYPE_TLS_VERSION;
            ret = uCxAtUtilParseParamsF(pParamsLine, "-d", &pMqttGetTLSRsp->rsp.TlsVersion.tls_version, U_CX_AT_UTIL_PARAM_LAST);
            break;
        default:
            return false;
    } /* ~switch (rspSyntaxVal) */
    return ret >= 0;
}

int32_t uCxMqttDisconnect(uCxHandle_t * puCxHandle, int32_t mqtt_id)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UMQDC=", "d", mqtt_id, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxMqttPublish(uCxHandle_t * puCxHandle, int32_t mqtt_id, uMqttQos_t qos, uMqttRetain_t retain, const char * topic, const uint8_t * binary_data, int32_t binary_data_len)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t packet_id;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UMQPB=", "dddsB", mqtt_id, qos, retain, topic, binary_data, binary_data_len, U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UMQPB:", NULL, NULL, "-d", &packet_id, U_CX_AT_UTIL_PARAM_LAST);
    {
        // Always call uCxAtClientCmdEnd() even if any previous function failed
        int32_t endRet = uCxAtClientCmdEnd(pAtClient);
        if (ret >= 0) {
            ret = endRet;
        }
    }
    if (ret >= 0) {
        ret = packet_id;
    }
    return ret;
}

int32_t uCxMqttSubscribe3(uCxHandle_t * puCxHandle, int32_t mqtt_id, uMqttSubscribeAction_t subscribe_action, const char * topic)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UMQS=", "dds", mqtt_id, subscribe_action, topic, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxMqttSubscribe4(uCxHandle_t * puCxHandle, int32_t mqtt_id, uMqttSubscribeAction_t subscribe_action, const char * topic, uMqttQos_t qos)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UMQS=", "ddsd", mqtt_id, subscribe_action, topic, qos, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxMqttReadBegin(uCxHandle_t * puCxHandle, int32_t mqtt_id, uint8_t * pDataBuf, uint16_t dataBufLength, const char ** ppTopic)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    uint8_t *pBinBuffer = pDataBuf;
    uint16_t binBufferLen = dataBufLength;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UMQRB=", "d", mqtt_id, U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UMQRB:", pBinBuffer, &binBufferLen, "-s", ppTopic, U_CX_AT_UTIL_PARAM_LAST);
    return (ret < 0) ? ret : (int32_t)binBufferLen;
}

void uCxMqttRegisterConnect(uCxHandle_t * puCxHandle, uUEMQC_t callback)
{
    puCxHandle->callbacks.UEMQC = callback;
}

void uCxMqttRegisterDisconnect(uCxHandle_t * puCxHandle, uUEMQDC_t callback)
{
    puCxHandle->callbacks.UEMQDC = callback;
}

void uCxMqttRegisterDataAvailable(uCxHandle_t * puCxHandle, uUEMQDA_t callback)
{
    puCxHandle->callbacks.UEMQDA = callback;
}

void uCxMqttRegisterDataDropped(uCxHandle_t * puCxHandle, uUEMQDD_t callback)
{
    puCxHandle->callbacks.UEMQDD = callback;
}

void uCxMqttRegisterPublishCompleted(uCxHandle_t * puCxHandle, uUEMQPC_t callback)
{
    puCxHandle->callbacks.UEMQPC = callback;
}

void uCxMqttRegisterSubscribeCompleted(uCxHandle_t * puCxHandle, uUEMQSC_t callback)
{
    puCxHandle->callbacks.UEMQSC = callback;
}
