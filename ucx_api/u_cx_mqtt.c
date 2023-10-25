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

bool uCxBeginMqttGetConnectionParams(uCxHandle_t * puCxHandle, int32_t mqtt_id, uCxMqttGetConnectionParams_t * pMqttGetConnectionParamsRsp)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UMQCP=", "d", mqtt_id, U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UMQCP:", NULL, NULL, "dsdss", &pMqttGetConnectionParamsRsp->mqtt_id, &pMqttGetConnectionParamsRsp->hostname, &pMqttGetConnectionParamsRsp->port, &pMqttGetConnectionParamsRsp->client_id, &pMqttGetConnectionParamsRsp->username, U_CX_AT_UTIL_PARAM_LAST);
    return ret > 0;
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

int32_t uCxMqttGetKeepAlive(uCxHandle_t * puCxHandle, int32_t mqtt_id, uCxMqttGetKeepAlive_t * pMqttGetKeepAliveRsp)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UMQKA=", "d", mqtt_id, U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UMQKA:", NULL, NULL, "dd", &pMqttGetKeepAliveRsp->mqtt_id, &pMqttGetKeepAliveRsp->keep_alive, U_CX_AT_UTIL_PARAM_LAST);
    {
        // Always call uCxAtClientCmdEnd() even any previous function failed
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

int32_t uCxMqttSetLastWillAndTestament4(uCxHandle_t * puCxHandle, int32_t mqtt_id, const char * topic, const char * will_msg, uQos_t qos)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UMQLWT=", "dssd", mqtt_id, topic, will_msg, qos, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxMqttSetLastWillAndTestament5(uCxHandle_t * puCxHandle, int32_t mqtt_id, const char * topic, const char * will_msg, uQos_t qos, uRetain_t retain)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UMQLWT=", "dssdd", mqtt_id, topic, will_msg, qos, retain, U_CX_AT_UTIL_PARAM_LAST);
}

bool uCxBeginMqttGetLastWillAndTestament(uCxHandle_t * puCxHandle, int32_t mqtt_id, uCxMqttGetLastWillAndTestament_t * pMqttGetLastWillAndTestamentRsp)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UMQLWT=", "d", mqtt_id, U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UMQLWT", NULL, NULL, "dssdd", &pMqttGetLastWillAndTestamentRsp->mqtt_id, &pMqttGetLastWillAndTestamentRsp->topic, &pMqttGetLastWillAndTestamentRsp->will_msg, &pMqttGetLastWillAndTestamentRsp->qos, &pMqttGetLastWillAndTestamentRsp->retain, U_CX_AT_UTIL_PARAM_LAST);
    return ret > 0;
}

int32_t uCxMqttSetTlsConfig3(uCxHandle_t * puCxHandle, int32_t mqtt_id, uTlsVersion_t tls_version, const char * ca_name)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UMQTLS=", "dds", mqtt_id, tls_version, ca_name, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxMqttSetTlsConfig5(uCxHandle_t * puCxHandle, int32_t mqtt_id, uTlsVersion_t tls_version, const char * ca_name, const char * client_cert_name, const char * client_key_name)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UMQTLS=", "ddsss", mqtt_id, tls_version, ca_name, client_cert_name, client_key_name, U_CX_AT_UTIL_PARAM_LAST);
}

bool uCxBeginMqttGetTlsConfig(uCxHandle_t * puCxHandle, int32_t mqtt_id, uCxMqttGetTlsConfig_t * pMqttGetTlsConfigRsp)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UMQTLS=", "d", mqtt_id, U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UMQTLS:", NULL, NULL, "ddsss", &pMqttGetTlsConfigRsp->mqtt_id, &pMqttGetTlsConfigRsp->tls_version, &pMqttGetTlsConfigRsp->ca_name, &pMqttGetTlsConfigRsp->client_cert_name, &pMqttGetTlsConfigRsp->client_key_name, U_CX_AT_UTIL_PARAM_LAST);
    return ret > 0;
}

int32_t uCxMqttDisconnect(uCxHandle_t * puCxHandle, int32_t mqtt_id)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UMQDC=", "d", mqtt_id, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxMqttPublish(uCxHandle_t * puCxHandle, int32_t mqtt_id, uQos_t qos, uRetain_t retain, const char * topic, const char * message)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UMQP=", "dddss", mqtt_id, qos, retain, topic, message, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxMqttSubscribe3(uCxHandle_t * puCxHandle, int32_t mqtt_id, uSubscribeAction_t subscribe_action, const char * topic)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UMQS=", "dds", mqtt_id, subscribe_action, topic, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxMqttSubscribe4(uCxHandle_t * puCxHandle, int32_t mqtt_id, uSubscribeAction_t subscribe_action, const char * topic, uQos_t qos)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UMQS=", "ddsd", mqtt_id, subscribe_action, topic, qos, U_CX_AT_UTIL_PARAM_LAST);
}

bool uCxBeginMqttReadMessage(uCxHandle_t * puCxHandle, int32_t mqtt_id, uCxMqttReadMessage_t * pMqttReadMessageRsp)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+UMQR=", "d", mqtt_id, U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+UMQR:", NULL, NULL, "ddsds", &pMqttReadMessageRsp->mqtt_id, &pMqttReadMessageRsp->num_unread, &pMqttReadMessageRsp->topic, &pMqttReadMessageRsp->message_len, &pMqttReadMessageRsp->message, U_CX_AT_UTIL_PARAM_LAST);
    return ret > 0;
}
