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
#ifndef _U_CX_MQTT_H_
#define _U_CX_MQTT_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "u_cx_types.h"
#include "u_cx.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* ------------------------------------------------------------
 * RESPONSES
 * ---------------------------------------------------------- */

typedef struct
{
    int32_t mqtt_id;        /**< MQTT Config ID */
    const char * hostname;  /**< Hostname or IP address of the broker */
    int32_t port;           /**< The port of the broker */
    const char * client_id; /**< Client ID. Can be left empty to let the broker decide */
    const char * username;
} uCxMqttGetConnectionParams_t;

typedef struct
{
    int32_t mqtt_id;    /**< MQTT Config ID */
    int32_t keep_alive; /**< MQTT keepalive in seconds. If set to 0, no keepalive is used */
} uCxMqttGetKeepAlive_t;

typedef struct
{
    int32_t mqtt_id;       /**< MQTT Config ID */
    const char * topic;    /**< Topic name or filter (wildcard allowed) */
    const char * will_msg;
    int32_t qos;           /**< Quality of Service (QoS) for the message or topic */
    int32_t retain;        /**< Retain flag for message */
} uCxMqttGetLastWillAndTestament_t;

typedef struct
{
    int32_t mqtt_id;               /**< MQTT Config ID */
    int32_t tls_version;           /**< Minimum TLS version to use */
    const char * ca_name;          /**< Name of the certificate authority (CA) certificate to use */
    const char * client_cert_name; /**< Name of the client certificate to use */
    const char * client_key_name;  /**< Name of the private key for client certificate */
} uCxMqttGetTlsConfig_t;

typedef struct
{
    int32_t mqtt_id;      /**< MQTT Config ID */
    int32_t num_unread;   /**< Number of available MQTT messages */
    const char * topic;   /**< Topic name or filter (wildcard allowed) */
    int32_t message_len;  /**< Length of the MQTT message */
    const char * message; /**< MQTT message */
} uCxMqttReadMessage_t;


/* ------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * ---------------------------------------------------------- */

/**
 * Set the MQTT login parameters.
 * Note that empty strings for username and password mean they will not be used during the connection.
 * 
 * Output AT command:
 * > AT+UMQCP=<mqtt_id>,<hostname>,<port>
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      mqtt_id:    MQTT Config ID
 * @param      hostname:   Hostname or IP address of the broker
 * @param      port:       The port of the broker
 */
int32_t uCxMqttSetConnectionParams3(uCxHandle_t * puCxHandle, int32_t mqtt_id, const char * hostname, int32_t port);

/**
 * Set the MQTT login parameters.
 * Note that empty strings for username and password mean they will not be used during the connection.
 * 
 * Output AT command:
 * > AT+UMQCP=<mqtt_id>,<hostname>,<port>,<client_id>
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      mqtt_id:    MQTT Config ID
 * @param      hostname:   Hostname or IP address of the broker
 * @param      port:       The port of the broker
 * @param      client_id:  Client ID. Can be left empty to let the broker decide
 */
int32_t uCxMqttSetConnectionParams4(uCxHandle_t * puCxHandle, int32_t mqtt_id, const char * hostname, int32_t port, const char * client_id);

/**
 * Set the MQTT login parameters.
 * Note that empty strings for username and password mean they will not be used during the connection.
 * 
 * Output AT command:
 * > AT+UMQCP=<mqtt_id>,<hostname>,<port>,<client_id>,<username>
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      mqtt_id:    MQTT Config ID
 * @param      hostname:   Hostname or IP address of the broker
 * @param      port:       The port of the broker
 * @param      client_id:  Client ID. Can be left empty to let the broker decide
 * @param      username:   
 */
int32_t uCxMqttSetConnectionParams5(uCxHandle_t * puCxHandle, int32_t mqtt_id, const char * hostname, int32_t port, const char * client_id, const char * username);

/**
 * Set the MQTT login parameters.
 * Note that empty strings for username and password mean they will not be used during the connection.
 * 
 * Output AT command:
 * > AT+UMQCP=<mqtt_id>,<hostname>,<port>,<client_id>,<username>,<password>
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      mqtt_id:    MQTT Config ID
 * @param      hostname:   Hostname or IP address of the broker
 * @param      port:       The port of the broker
 * @param      client_id:  Client ID. Can be left empty to let the broker decide
 * @param      username:   
 * @param      password:   
 */
int32_t uCxMqttSetConnectionParams6(uCxHandle_t * puCxHandle, int32_t mqtt_id, const char * hostname, int32_t port, const char * client_id, const char * username, const char * password);

/**
 * Get the MQTT login parameters.
 * Note that empty strings for username and password mean they will not be used during the connection.
 * 
 * Output AT command:
 * > AT+UMQCP=<mqtt_id>
 *
 * @param[in]  puCxHandle:                  uCX API handle
 * @param      mqtt_id:                     MQTT Config ID
 * @param[out] pMqttGetConnectionParamsRsp: Please see \ref uCxMqttGetConnectionParams_t
 */
bool uCxBeginMqttGetConnectionParams(uCxHandle_t * puCxHandle, int32_t mqtt_id, uCxMqttGetConnectionParams_t * pMqttGetConnectionParamsRsp);

/**
 * Connect to a broker using the MQTT config ID.
 * 
 * Output AT command:
 * > AT+UMQC=<mqtt_id>
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      mqtt_id:    MQTT Config ID
 */
int32_t uCxMqttConnect(uCxHandle_t * puCxHandle, int32_t mqtt_id);

/**
 * Set keepalive timeout for MQTT the MQTT config
 * 
 * Output AT command:
 * > AT+UMQKA=<mqtt_id>,<keep_alive>
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      mqtt_id:    MQTT Config ID
 * @param      keep_alive: MQTT keepalive in seconds. If set to 0, no keepalive is used
 */
int32_t uCxMqttSetKeepAlive(uCxHandle_t * puCxHandle, int32_t mqtt_id, int32_t keep_alive);

/**
 * Get keepalive timeout for MQTT config
 * 
 * Output AT command:
 * > AT+UMQKA=<mqtt_id>
 *
 * @param[in]  puCxHandle:           uCX API handle
 * @param      mqtt_id:              MQTT Config ID
 * @param[out] pMqttGetKeepAliveRsp: Please see \ref uCxMqttGetKeepAlive_t
 */
int32_t uCxMqttGetKeepAlive(uCxHandle_t * puCxHandle, int32_t mqtt_id, uCxMqttGetKeepAlive_t * pMqttGetKeepAliveRsp);

/**
 * Add last will and testament configuration for the client
 * 
 * Output AT command:
 * > AT+UMQLWT=<mqtt_id>,<topic>,<will_msg>
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      mqtt_id:    MQTT Config ID
 * @param      topic:      Topic name or filter (wildcard allowed)
 * @param      will_msg:   
 */
int32_t uCxMqttSetLastWillAndTestament3(uCxHandle_t * puCxHandle, int32_t mqtt_id, const char * topic, const char * will_msg);

/**
 * Add last will and testament configuration for the client
 * 
 * Output AT command:
 * > AT+UMQLWT=<mqtt_id>,<topic>,<will_msg>,<qos>
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      mqtt_id:    MQTT Config ID
 * @param      topic:      Topic name or filter (wildcard allowed)
 * @param      will_msg:   
 * @param      qos:        Quality of Service (QoS) for the message or topic
 */
int32_t uCxMqttSetLastWillAndTestament4(uCxHandle_t * puCxHandle, int32_t mqtt_id, const char * topic, const char * will_msg, uQos_t qos);

/**
 * Add last will and testament configuration for the client
 * 
 * Output AT command:
 * > AT+UMQLWT=<mqtt_id>,<topic>,<will_msg>,<qos>,<retain>
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      mqtt_id:    MQTT Config ID
 * @param      topic:      Topic name or filter (wildcard allowed)
 * @param      will_msg:   
 * @param      qos:        Quality of Service (QoS) for the message or topic
 * @param      retain:     Retain flag for message
 */
int32_t uCxMqttSetLastWillAndTestament5(uCxHandle_t * puCxHandle, int32_t mqtt_id, const char * topic, const char * will_msg, uQos_t qos, uRetain_t retain);

/**
 * Get last will and testament configuration for the client
 * 
 * Output AT command:
 * > AT+UMQLWT=<mqtt_id>
 *
 * @param[in]  puCxHandle:                      uCX API handle
 * @param      mqtt_id:                         MQTT Config ID
 * @param[out] pMqttGetLastWillAndTestamentRsp: Please see \ref uCxMqttGetLastWillAndTestament_t
 */
bool uCxBeginMqttGetLastWillAndTestament(uCxHandle_t * puCxHandle, int32_t mqtt_id, uCxMqttGetLastWillAndTestament_t * pMqttGetLastWillAndTestamentRsp);

/**
 * Setup MQTT TLS config. Certs do not have to be uploaded until connection.
 * 
 * Output AT command:
 * > AT+UMQTLS=<mqtt_id>,<tls_version>,<ca_name>
 *
 * @param[in]  puCxHandle:  uCX API handle
 * @param      mqtt_id:     MQTT Config ID
 * @param      tls_version: Minimum TLS version to use
 * @param      ca_name:     Name of the certificate authority (CA) certificate to use
 */
int32_t uCxMqttSetTlsConfig3(uCxHandle_t * puCxHandle, int32_t mqtt_id, uTlsVersion_t tls_version, const char * ca_name);

/**
 * Setup MQTT TLS config. Certs do not have to be uploaded until connection.
 * 
 * Output AT command:
 * > AT+UMQTLS=<mqtt_id>,<tls_version>,<ca_name>,<client_cert_name>,<client_key_name>
 *
 * @param[in]  puCxHandle:       uCX API handle
 * @param      mqtt_id:          MQTT Config ID
 * @param      tls_version:      Minimum TLS version to use
 * @param      ca_name:          Name of the certificate authority (CA) certificate to use
 * @param      client_cert_name: Name of the client certificate to use
 * @param      client_key_name:  Name of the private key for client certificate
 */
int32_t uCxMqttSetTlsConfig5(uCxHandle_t * puCxHandle, int32_t mqtt_id, uTlsVersion_t tls_version, const char * ca_name, const char * client_cert_name, const char * client_key_name);

/**
 * Get TLS config
 * 
 * Output AT command:
 * > AT+UMQTLS=<mqtt_id>
 *
 * @param[in]  puCxHandle:           uCX API handle
 * @param      mqtt_id:              MQTT Config ID
 * @param[out] pMqttGetTlsConfigRsp: Please see \ref uCxMqttGetTlsConfig_t
 */
bool uCxBeginMqttGetTlsConfig(uCxHandle_t * puCxHandle, int32_t mqtt_id, uCxMqttGetTlsConfig_t * pMqttGetTlsConfigRsp);

/**
 * Disconnect the MQTT client from the broker.
 * Note that the disconnection is not complete until the +UEMQDC URC arrives.
 * 
 * Output AT command:
 * > AT+UMQDC=<mqtt_id>
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      mqtt_id:    MQTT Config ID
 */
int32_t uCxMqttDisconnect(uCxHandle_t * puCxHandle, int32_t mqtt_id);

/**
 * Publish an MQTT message to the specified topic.
 * 
 * Output AT command:
 * > AT+UMQP=<mqtt_id>,<qos>,<retain>,<topic>,<message>
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      mqtt_id:    MQTT Config ID
 * @param      qos:        Quality of Service (QoS) for the message or topic
 * @param      retain:     Retain flag for message
 * @param      topic:      Topic name or filter (wildcard allowed)
 * @param      message:    MQTT message
 */
int32_t uCxMqttPublish(uCxHandle_t * puCxHandle, int32_t mqtt_id, uQos_t qos, uRetain_t retain, const char * topic, const char * message);

/**
 * Subscribe or unsubscribe to/from MQTT topic.
 * 
 * Output AT command:
 * > AT+UMQS=<mqtt_id>,<subscribe_action>,<topic>
 *
 * @param[in]  puCxHandle:       uCX API handle
 * @param      mqtt_id:          MQTT Config ID
 * @param      subscribe_action: 
 * @param      topic:            Topic name or filter (wildcard allowed)
 */
int32_t uCxMqttSubscribe3(uCxHandle_t * puCxHandle, int32_t mqtt_id, uSubscribeAction_t subscribe_action, const char * topic);

/**
 * Subscribe or unsubscribe to/from MQTT topic.
 * 
 * Output AT command:
 * > AT+UMQS=<mqtt_id>,<subscribe_action>,<topic>,<qos>
 *
 * @param[in]  puCxHandle:       uCX API handle
 * @param      mqtt_id:          MQTT Config ID
 * @param      subscribe_action: 
 * @param      topic:            Topic name or filter (wildcard allowed)
 * @param      qos:              Quality of Service (QoS) for the message or topic
 */
int32_t uCxMqttSubscribe4(uCxHandle_t * puCxHandle, int32_t mqtt_id, uSubscribeAction_t subscribe_action, const char * topic, uQos_t qos);

/**
 * Read available MQTT message.
 * 
 * Output AT command:
 * > AT+UMQR=<mqtt_id>
 *
 * @param[in]  puCxHandle:          uCX API handle
 * @param      mqtt_id:             MQTT Config ID
 * @param[out] pMqttReadMessageRsp: Please see \ref uCxMqttReadMessage_t
 */
bool uCxBeginMqttReadMessage(uCxHandle_t * puCxHandle, int32_t mqtt_id, uCxMqttReadMessage_t * pMqttReadMessageRsp);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _U_CX_MQTT_H_ */