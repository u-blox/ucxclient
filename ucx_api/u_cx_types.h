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
#ifndef _U_CX_TYPES_H_
#define _U_CX_TYPES_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdint.h>
#include <stdbool.h>

/* ------------------------------------------------------------
 * ENUMS
 * ---------------------------------------------------------- */

typedef enum
{
    U_GENERAL_GREETING_MODE_OFF,   /**< Turn off the greeting text. */
    U_GENERAL_GREETING_MODE_ON = 1 /**< Turn on the greeting text. */
} uGeneralGreetingMode_t;

typedef enum
{
    U_INTERFACE_ID_BLUETOOTH,        /**< Bluetooth */
    U_INTERFACE_ID_WIFI_STATION = 1, /**< Wi-Fi station */
    U_INTERFACE_ID_WIFI_AP = 2       /**< Wi-Fi Access point */
} uInterfaceId_t;

typedef enum
{
    U_SYS_EXTENDED_ERRORS_OFF,   /**< (Factory default) Extended error codes will not be displayed */
    U_SYS_EXTENDED_ERRORS_ON = 1 /**< Extended error code will be displayed on every error */
} uSysExtendedErrors_t;

typedef enum
{
    U_SYS_ECHO_ON_OFF,   /**< Module does not echo the characters */
    U_SYS_ECHO_ON_ON = 1 /**< (Factory default) Module echoes the characters */
} uSysEchoOn_t;

typedef enum
{
    U_BT_MODE_DISABLED,              /**< Disabled. */
    U_BT_MODE_CENTRAL = 1,           /**< Bluetooth Low Energy Central.
                                          In this mode, starting advertisements, direct advertisements and other functions
                                          associated
                                          with the Peripheral role is not possible. */
    U_BT_MODE_PERIPHERAL = 2,        /**< Bluetooth Low Energy Peripheral.
                                          In this mode, initiating connections, discovery and other functions associated with
                                          the Central role is not possible. */
    U_BT_MODE_CENTRAL_PERIPHERAL = 3 /**< Bluetooth Low Energy Simultaneous Central and Peripheral. This is the factory default. */
} uBtMode_t;

typedef enum
{
    U_BT_DATA_TYPE_SCAN_RESPONSE,     /**< Scan response data. */
    U_BT_DATA_TYPE_ADVERTISE_DATA = 1 /**< Advertise data. */
} uBtDataType_t;

typedef enum
{
    U_BT_DISCOVERY_TYPE_ALL,                  /**< All with no filter. Displays all found devices; devices can be displayed multiple times. */
    U_BT_DISCOVERY_TYPE_ALL_NO_DUPLICATES = 1 /**< General inquiry. Displays devices in General or Limited discoverability mode; each device
                                                   is displayed only once. */
} uBtDiscoveryType_t;

typedef enum
{
    U_BT_DISCOVERY_MODE_ACTIVE,     /**< Active discovery. */
    U_BT_DISCOVERY_MODE_PASSIVE = 1 /**< Passive, no scan response data will be received. */
} uBtDiscoveryMode_t;

typedef enum
{
    U_BT_OUTPUT_EVENTS_DISABLED,   /**< Disable output events during background discovery */
    U_BT_OUTPUT_EVENTS_ENABLED = 1 /**< Enable output events during background discovery */
} uBtOutputEvents_t;

typedef enum
{
    U_BT_PROP_ID_CONNECTION_INTERVAL,       /**< Connection interval used on this connection.
                                                  Range: 6 to 3200
                                                  Time = status_val * 1.25 ms
                                                  Time range: 7.5 ms to 4000 ms */
    U_BT_PROP_ID_PERIPHERAL_LATENCY = 1,    /**< Peripheral latency for the connection in number of connection events. Range: 0 to 499 */
    U_BT_PROP_ID_SUPERVISION_TIMEOUT = 2,   /**< Supervision timeout (in ms) for this connections. Range: 100 ms to 32000 ms */
    U_BT_PROP_ID_MTU_SIZE = 3,              /**< MTU size for this connections. */
    U_BT_PROP_ID_PDU_TX_PAYLOAD_LENGTH = 4, /**< Data Channel TX PDU Payload Length. */
    U_BT_PROP_ID_PDU_RX_PAYLOAD_LENGTH = 5, /**< Data Channel RX PDU Payload Length. */
    U_BT_PROP_ID_DATA_LENGTH_EXTENSION = 6, /**< Data Length Extension state. 0: Data Length Extension Off \ 1: Data Length Extension On */
    U_BT_PROP_ID_LOCAL_ROLE = 7,            /**< Local role in this connection. 1: Low Energy Central \ 2: Low Energy Peripheral */
    U_BT_PROP_ID_TX_PHY = 8,                /**< TX Phy used in this connection
                                                 Bit 0: 1 Mbps
                                                 Bit 1: 2 Mbps
                                                 Bit 2: Coded */
    U_BT_PROP_ID_RX_PHY = 9                 /**< RX Phy used in this connection
                                                 Bit 0: 1 Mbps
                                                 Bit 1: 2 Mbps
                                                 Bit 2: Coded */
} uBtPropId_t;

typedef enum
{
    U_BT_LEGACY_ADV_DISABLED,   /**< Legacy Advertisement Not Running */
    U_BT_LEGACY_ADV_ENABLED = 1 /**< Legacy Advertisement Running */
} uBtLegacyAdv_t;

typedef enum
{
    U_BT_DIRECTED_ADV_DISABLED,   /**< Directed Advertisement Not Running */
    U_BT_DIRECTED_ADV_ENABLED = 1 /**< Directed Advertisement Running */
} uBtDirectedAdv_t;

typedef enum
{
    U_BT_IO_CAP_NO_INPUT_NO_OUTPUT,  /**< Set I/O Capabilities to No Input No Output. */
    U_BT_IO_CAP_DISPLAY_ONLY = 1,    /**< Set I/O Capabilities to Display Only. */
    U_BT_IO_CAP_DISPLAY_YES_NO = 2,  /**< Set I/O Capabilities to Display Yes/No */
    U_BT_IO_CAP_KEYBOARD_ONLY = 3,   /**< Set I/O Capabilities to Keyboard Only. */
    U_BT_IO_CAP_KEYBOARD_DISPLAY = 4 /**< Set I/O Capabilities to Keyboard Display. */
} uBtIoCap_t;

typedef enum
{
    U_BT_SECURITY_MODE_NONE,                                    /**< Security not required. No encryption enforced. */
    U_BT_SECURITY_MODE_UNAUTHENTICATED = 1,                     /**< Require at least unauthenticated bonding. */
    U_BT_SECURITY_MODE_AUTHENTICATED = 2,                       /**< Require authenticated bonding. No secure connections. */
    U_BT_SECURITY_MODE_AUTHENTICATED_SECURE_CONNECTION = 3,     /**< Require authenticated bonding. Support secure connections. Fallback to simple pairing if
                                                                     the remote side does not support secure connections. */
    U_BT_SECURITY_MODE_AUTHENTICATED_SECURE_CONNECTION_ONLY = 4 /**< Require authenticated bonding. Strictly uses secure connections. */
} uBtSecurityMode_t;

typedef enum
{
    U_BT_PAIRING_MODE_DISABLE,   /**< Disable pairing mode. */
    U_BT_PAIRING_MODE_ENABLE = 1 /**< Enable pairing mode. */
} uBtPairingMode_t;

typedef enum
{
    U_BT_CONFIRM_NO,     /**< Deny bonding. */
    U_BT_CONFIRM_YES = 1 /**< Confirm bonding. */
} uBtConfirm_t;

typedef enum
{
    U_BT_CHAR_ID_MANUFACTURER_NAME,     /**< Manufacturer name string. Maximum length of the custom string is 31 characters. */
    U_BT_CHAR_ID_MODEL_NAME = 1,        /**< Model name string. Maximum length of the custom string is 20 characters. */
    U_BT_CHAR_ID_FIRMWARE_REVISION = 2, /**< Firmware revision string. Maximum length of the custom string is 20 characters. */
    U_BT_CHAR_ID_SOFTWARE_REVISION = 3  /**< Software revision string. Maximum length of the custom string is 20 characters. */
} uBtCharId_t;

typedef enum
{
    U_GATT_CLIENT_CONFIG_NONE,                     /**< None */
    U_GATT_CLIENT_CONFIG_ENABLE_NOTIFICATIONS = 1, /**< Enable notifications */
    U_GATT_CLIENT_CONFIG_ENABLE_INDICATIONS = 2,   /**< Enable indications */
    U_GATT_CLIENT_CONFIG_ENABLE_NOT_IND = 3        /**< Enable notifications and indications */
} uGattClientConfig_t;

typedef enum
{
    U_GATT_CLIENT_RELIABLE_NO,     /**< Not reliable */
    U_GATT_CLIENT_RELIABLE_YES = 1 /**< Reliable */
} uGattClientReliable_t;

typedef enum
{
    U_GATT_CLIENT_FLAG_FINAL_DATA,    /**< Final data */
    U_GATT_CLIENT_FLAG_MORE_DATA = 1, /**< More data */
    U_GATT_CLIENT_FLAG_CANCEL = 2     /**< Cancel data writing */
} uGattClientFlag_t;

typedef enum
{
    U_GATT_SERVER_READ_SECURITY_NONE = 1,            /**< No encryption required. */
    U_GATT_SERVER_READ_SECURITY_UNAUTHENTICATED = 2, /**< Unauthenticated encryption required. */
    U_GATT_SERVER_READ_SECURITY_AUTHENTICATED = 3    /**< Authenticated encryption required. */
} uGattServerReadSecurity_t;

typedef enum
{
    U_GATT_SERVER_WRITE_SECURITY_NONE = 1,            /**< No encryption required. */
    U_GATT_SERVER_WRITE_SECURITY_UNAUTHENTICATED = 2, /**< Unauthenticated encryption required. */
    U_GATT_SERVER_WRITE_SECURITY_AUTHENTICATED = 3    /**< Authenticated encryption required. */
} uGattServerWriteSecurity_t;

typedef enum
{
    U_SPS_SERVICE_OPTION_DISABLE,   /**< This option disables the SPS service after saving the configuration and restarting the
                                         device. (Default) */
    U_SPS_SERVICE_OPTION_ENABLE = 1 /**< This option enables the SPS service directly.
                                         If this option is set, and the configuration is saved,
                                         SPS will be enabled after reboot. */
} uSpsServiceOption_t;

typedef enum
{
    U_READ_MODE_BUFFERED,          /**< Buffered mode */
    U_READ_MODE_DIRECT_STRING = 1, /**< Direct String mode */
    U_READ_MODE_DIRECT_BINARY = 2  /**< Direct Binary Mode */
} uReadMode_t;

typedef enum
{
    U_WIFI_TLS_VERSION_NO_TLS,              /**< Disable TLS */
    U_WIFI_TLS_VERSION_TLS1_2 = 1,          /**< TLS 1.2 */
    U_WIFI_TLS_VERSION_TLS1_3 = 2,          /**< TLS 1.3 */
    U_WIFI_TLS_VERSION_TLS1_2_OR_TLS1_3 = 3 /**< TLS 1.2 or 1.3 (negotiate highest) */
} uWifiTlsVersion_t;

typedef enum
{
    U_WIFI_SECURITY_MODE_OPEN,    /**< Open security */
    U_WIFI_SECURITY_MODE_WPA = 1, /**< WPA security */
    U_WIFI_SECURITY_MODE_EAP = 2, /**< EAP-TLS security */
    U_WIFI_SECURITY_MODE_PEAP = 3 /**< PEAP security */
} uWifiSecurityMode_t;

typedef enum
{
    U_WIFI_WPA_THRESHOLD_WPA2,    /**< Only connect to access points that support WPA2 or up */
    U_WIFI_WPA_THRESHOLD_WPA3 = 1 /**< Only connect to access points that support WPA3 */
} uWifiWpaThreshold_t;

typedef enum
{
    U_WIFI_IP_MODE_DHCP,      /**< DHCP */
    U_WIFI_IP_MODE_STATIC = 1 /**< Static IP */
} uWifiIpMode_t;

typedef enum
{
    U_WIFI_NET_STATUS_ID_IPV4,         /**< The current IPv4 address.
                                            Note: If network is down this will be 0.0.0.0 regardless of IP setting */
    U_WIFI_NET_STATUS_ID_SUBNET = 1,   /**< The current subnet mask
                                            Note: If network is down this will be 0.0.0.0 regardless of IP setting */
    U_WIFI_NET_STATUS_ID_GATE_WAY = 2, /**< The current gateway
                                            Note: If network is down this will be 0.0.0.0 regardless of IP setting */
    U_WIFI_NET_STATUS_ID_PRIM_DNS = 3, /**< The current primary DNS server
                                            Note: If network is down this will be 0.0.0.0 regardless of IP setting */
    U_WIFI_NET_STATUS_ID_SEC_DNS = 4,  /**< The current secondary DNS server
                                            Note: If network is down this will be 0.0.0.0 regardless of IP setting */
    U_WIFI_NET_STATUS_ID_IPV6 = 5      /**< The current IPv6 link local address
                                            Note: If network is down this will be [0000:0000:0000:0000:0000:0000:0000:0000] regardless
                                            of IP setting */
} uWifiNetStatusId_t;

typedef enum
{
    U_WIFI_REG_DOMAIN_WORLD,    /**< World
                                     Supported Channels;
                                     1-11
                                     36,40,44,48
                                     52,56,60,64 */
    U_WIFI_REG_DOMAIN_ETSI = 1, /**< ETSI
                                     Supported Channels;
                                     1-13
                                     36,40,44,48
                                     52,56,60,64
                                     100,104,108,112,116,120,124,128,132,136,140
                                     149,153,157,161,165 */
    U_WIFI_REG_DOMAIN_FCC = 2,  /**< FCC
                                     Supported Channels;
                                     1-11
                                     36,40,44,48
                                     52,56,60,64
                                     100,104,108,112,116,120,124,128,132,136,140
                                     144
                                     149,153,157,161,165 */
    U_WIFI_REG_DOMAIN_IC = 3,   /**< IC/ISED
                                     Supported Channels;
                                     1-11
                                     36,40,44,48
                                     52,56,60,64
                                     100,104,108,112,116,132,136,140
                                     144
                                     149,153,157,161,165 */
    U_WIFI_REG_DOMAIN_NZ = 4,   /**< NZ
                                     Supported Channels;
                                     1-13
                                     36,40,44,48
                                     52,56,60,64
                                     100,104,108,112,116,120,124,128,132,136,140
                                     149,153,157,161,165 */
    U_WIFI_REG_DOMAIN_MKK = 5,  /**< MKK/Japan
                                     Supported Channels;
                                     1-14
                                     36,40,44,48
                                     52,56,60,64
                                     100,104,108,112,116,120,124,128,132,136,140
                                     144 */
    U_WIFI_REG_DOMAIN_NCC = 6,  /**< NCC/Taiwan
                                     Supported Channels;
                                     1-11
                                     36,40,44,48
                                     52,56,60,64
                                     100,104,108,112,116,120,124,128,132,136,140
                                     144
                                     149,153,157,161,165 */
    U_WIFI_REG_DOMAIN_ACMA = 7, /**< ACMA/AU
                                     Supported Channels;
                                     1-13
                                     36,40,44,48
                                     52,56,60,64
                                     100,104,108,112,116,132,136,140
                                     149,153,157,161,165 */
    U_WIFI_REG_DOMAIN_KCC = 8,  /**< KCC/South Korea
                                     Supported Channels;
                                     1-13
                                     36,40,44,48
                                     52,56,60,64
                                     100,104,108,112,116,120,124,128,132,136,140
                                     144
                                     149,153,157,161,165 */
    U_WIFI_REG_DOMAIN_SA = 9,   /**< SA/South Africa
                                     Supported Channels;
                                     1-13
                                     36,40,44,48
                                     52,56,60,64
                                     100,104,108,112,116,120,124,128,132,136,140 */
    U_WIFI_REG_DOMAIN_BR = 10   /**< Brazil
                                     Supported Channels;
                                     1-13
                                     36,40,44,48
                                     52,56,60,64
                                     100,104,108,112,116,120,124,128,132,136,140
                                     149,153,157,161,165 */
} uWifiRegDomain_t;

typedef enum
{
    U_WIFI_SCAN_MODE_ACTIVE,     /**< Active */
    U_WIFI_SCAN_MODE_PASSIVE = 1 /**< Passive */
} uWifiScanMode_t;

typedef enum
{
    U_WIFI_STATUS_ID_SSID,           /**< SSID of the connected AP */
    U_WIFI_STATUS_ID_BSSID = 1,      /**< BSSID of the connected AP */
    U_WIFI_STATUS_ID_CHANNEL = 2,    /**< Active channel */
    U_WIFI_STATUS_ID_CONNECTION = 3, /**< Connection status, 1 = not connected, 2 = Connected */
    U_WIFI_STATUS_ID_RSSI = 4        /**< RSSI value of the current connection; will return -32768, if not connected. */
} uWifiStatusId_t;

typedef enum
{
    U_WIFI_CHANNEL_1 = 1,   /**< 1 */
    U_WIFI_CHANNEL_2 = 2,   /**< 2 */
    U_WIFI_CHANNEL_3 = 3,   /**< 3 */
    U_WIFI_CHANNEL_4 = 4,   /**< 4 */
    U_WIFI_CHANNEL_5 = 5,   /**< 5 */
    U_WIFI_CHANNEL_6 = 6,   /**< 6 */
    U_WIFI_CHANNEL_7 = 7,   /**< 7 */
    U_WIFI_CHANNEL_8 = 8,   /**< 8 */
    U_WIFI_CHANNEL_9 = 9,   /**< 9 */
    U_WIFI_CHANNEL_10 = 10, /**< 10 */
    U_WIFI_CHANNEL_11 = 11, /**< 11 */
    U_WIFI_CHANNEL_36 = 36, /**< 36 */
    U_WIFI_CHANNEL_40 = 40, /**< 40 */
    U_WIFI_CHANNEL_44 = 44, /**< 44 */
    U_WIFI_CHANNEL_48 = 48  /**< 48 */
} uWifiChannel_t;

typedef enum
{
    U_WIFI_WPA_VERSION_WPA2 /**< WPA 2 */
} uWifiWpaVersion_t;

typedef enum
{
    U_WIFI_ROAMING_DISABLE,   /**< Disable roaming */
    U_WIFI_ROAMING_ENABLE = 1 /**< Enable roaming */
} uWifiRoaming_t;

typedef enum
{
    U_WIFI_ROAMING_AGGRESSIVE_DISABLE,   /**< Disable aggressive roaming */
    U_WIFI_ROAMING_AGGRESSIVE_ENABLE = 1 /**< Enable aggressive roaming */
} uWifiRoamingAggressive_t;

typedef enum
{
    U_WIFI_ROAMING_CHANNELS_CURRENT_CHANNEL, /**< Roaming on current channel */
    U_WIFI_ROAMING_CHANNELS_ALL = 1          /**< Roaming on all channels */
} uWifiRoamingChannels_t;

typedef enum
{
    U_SOCKET_PROTOCOL_TCP = 6, /**< TCP */
    U_SOCKET_PROTOCOL_UDP = 17 /**< UDP */
} uSocketProtocol_t;

typedef enum
{
    U_SOCKET_PREF_IP_VER_IPV4,    /**< IPv4 address. */
    U_SOCKET_PREF_IP_VER_IPV6 = 1 /**< IPv6 address. */
} uSocketPrefIpVer_t;

typedef enum
{
    U_SOCKET_STATUS_NOT_CONNECTED, /**< Not Connected */
    U_SOCKET_STATUS_LISTENING = 1, /**< Listening */
    U_SOCKET_STATUS_CONNECTED = 2  /**< Connected */
} uSocketStatus_t;

typedef enum
{
    U_SOCKET_OPTION_NO_DELAY,       /**< Turn on/off No delay feature for TCP sockets.
                                         Integer flag: 0 = off (i.e. Nagle algorithm enabled), 1 = on  (i.e. Nagle algorithm
                                         disabled)
                                         Default: to 0 */
    U_SOCKET_OPTION_BLOCK = 1,      /**< Set socket to be blocking or non blocking.
                                         Integer flag: 0 = off, 1 = on.
                                         Sockets are non-blocking by default (Note that read/write will always be non-blocking).
                                         Can only be set while the socket is in a non connected state.
                                         Note: Only valid for non-persistent TCP sockets, will have no effect on UDP sockets */
    U_SOCKET_OPTION_KEEP_ALIVE = 2, /**< Keep connections alive by sending keepalive probes.
                                         Integer flag: 0 = off, 1 = on.
                                         To calculate the keepalive time us this formula KeepIdle + (KeepIntvl * KeepCnt).
                                         Defaults to 1.
                                         Note: Only valid for TCP sockets. */
    U_SOCKET_OPTION_KEEP_IDLE = 3,  /**< Set Keep Idle value for the socket.
                                         This specifies the amount of time (in sec) that the connection must be idle before sending
                                         keepalive probes (if keepalive is enabled).
                                         Defaults to 3.
                                         Note: Only valid for TCP sockets. */
    U_SOCKET_OPTION_KEEP_INTVL = 4, /**< Set keep alive interval value for the socket. This is the time in seconds between two
                                         successive keepalive retransmissions.
                                         Defaults to 3.
                                         Note: Only valid for TCP sockets. */
    U_SOCKET_OPTION_KEEP_CNT = 5,   /**< Set keep alive counter value for the socket.
                                         The number of unanswered probes required to force closure of the socket.
                                         Defaults to 3.
                                         Note: Only valid for TCP sockets. */
    U_SOCKET_OPTION_BROADCAST = 6   /**< Set broadcast capability for UDP sockets.
                                         Integer flag: 0 = off, 1 = on.
                                         When enabled, allows sending and receiving UDP packets to/from broadcast addresses.
                                         Defaults to 0 (disabled).
                                         Note: Only valid for UDP sockets. */
} uSocketOption_t;

typedef enum
{
    U_MQTT_QOS_AT_MOST_ONCE,      /**< At most once */
    U_MQTT_QOS_AT_LEAST_ONCE = 1, /**< At least once */
    U_MQTT_QOS_EXACTLY_ONCE = 2   /**< Exactly once */
} uMqttQos_t;

typedef enum
{
    U_MQTT_RETAIN_NO,     /**< Do not retain message on broker */
    U_MQTT_RETAIN_YES = 1 /**< Retain message on broker */
} uMqttRetain_t;

typedef enum
{
    U_MQTT_SUBSCRIBE_ACTION_SUBSCRIBE,      /**< Subscribe to topic */
    U_MQTT_SUBSCRIBE_ACTION_UNSUBSCRIBE = 1 /**< Unsubscribe from topic */
} uMqttSubscribeAction_t;

typedef enum
{
    U_NTP_CLIENT_STATUS_DISABLE,           /**< (Factory default) Disable NTP client */
    U_NTP_CLIENT_STATUS_ENABLE_MANUAL = 1, /**< Enable NTP client using NTP servers configured by AT+UNTSC */
    U_NTP_CLIENT_STATUS_ENABLE_AUTO = 2    /**< Enable NTP client using NTP servers configured by DHCP if exists, otherwise use NTP
                                                servers configured by AT+UNTSC */
} uNtpClientStatus_t;

typedef enum
{
    U_NTP_REACHABLE_NO,     /**< NTP server is unreachable */
    U_NTP_REACHABLE_YES = 1 /**< NTP server is reachable */
} uNtpReachable_t;

typedef enum
{
    U_SEC_CERT_TYPE_ROOT,       /**< Root certificate */
    U_SEC_CERT_TYPE_CLIENT = 1, /**< Client certificate */
    U_SEC_CERT_TYPE_KEY = 2     /**< Client private key */
} uSecCertType_t;

typedef enum
{
    U_SEC_CERT_DETAIL_ID_FINGERPRINT,          /**< The fingerprint of the certificate, returns hex_value */
    U_SEC_CERT_DETAIL_ID_CERTIFICATE_SIZE = 1, /**< The size of the certificate, returns int_value */
    U_SEC_CERT_DETAIL_ID_NOT_BEFORE_DATE = 2,  /**< Certificate not valid before date, returns hex_value */
    U_SEC_CERT_DETAIL_ID_NOT_AFTER_DATE = 3    /**< Certificate not valid after date, returns hex_value */
} uSecCertDetailId_t;

typedef enum
{
    U_SEC_EXTENSION_SNI,              /**< Server Name Extension */
    U_SEC_EXTENSION_FRAGMENTATION = 1 /**< Handshake fragmentation */
} uSecExtension_t;

typedef enum
{
    U_ENABLED_NO,     /**< Disabled */
    U_ENABLED_YES = 1 /**< Enabled */
} uEnabled_t;

typedef enum
{
    U_POWER_WAKEUP_MODE_WAKEUP_GPIO /**< Wakeup by pulling the module wakeup pin low. */
} uPowerWakeupMode_t;

typedef enum
{
    U_DIAG_IPERF_ACTION_START = 1, /**< Start iperf */
    U_DIAG_IPERF_ACTION_STOP = 2   /**< Stop iperf */
} uDiagIperfAction_t;

typedef enum
{
    U_DIAG_PROTOCOL_TYPE_TCP = 1, /**< TCP */
    U_DIAG_PROTOCOL_TYPE_UDP = 2  /**< UDP */
} uDiagProtocolType_t;

typedef enum
{
    U_DIAG_ROLE_SERVER = 1, /**< Server */
    U_DIAG_ROLE_CLIENT = 2  /**< Client */
} uDiagRole_t;

typedef enum
{
    U_DIAG_BIDIRECTIONAL_OFF,   /**< Off */
    U_DIAG_BIDIRECTIONAL_ON = 1 /**< On
                                     When starting bidirectional TCP test, start a server on both tester and DUT, then start a
                                     client with bidirectional flag on the DUT.
                                     If doing bidirectional UDP test, start a server on both DUT and tester and then start a
                                     client with a bidirectional flag on both. */
} uDiagBidirectional_t;

typedef enum
{
    U_BT_BOND_STATUS_BONDING_SUCCEEDED,                /**< Bonding procedure succeeded. */
    U_BT_BOND_STATUS_BONDING_FAILED_TIMEOUT = 1,       /**< Bonding procedure failed due to page timeout. */
    U_BT_BOND_STATUS_BONDING_FAILED_AUTH = 2,          /**< Bonding failed because of authentication or pairing failed. This could be due to incorrect
                                                            PIN/passkey. */
    U_BT_BOND_STATUS_BONDING_FAILED_MITM = 3,          /**< Bonding failed because the protection against Man-In-The-Middle attack could not be
                                                            guaranteed; the generated link key was too weak. */
    U_BT_BOND_STATUS_BONDING_FAILED_PEER_LOST_BOND = 4 /**< Bonding failed because peer have lost the bonding info. Use AT+UBTUB to delete the local
                                                            bond to allow re-bonding. */
} uBtBondStatus_t;

typedef enum
{
    U_GATT_SERVER_OPTIONS_WRITE_WITH_OUT_RESPONSE, /**< Write without Response performed */
    U_GATT_SERVER_OPTIONS_WRITE_WITH_RESPONSE = 1, /**< Write with Response performed */
    U_GATT_SERVER_OPTIONS_WRITE_LONG = 2           /**< Write long performed */
} uGattServerOptions_t;

typedef enum
{
    U_DIAG_PING_RESPONSE_FALSE,   /**< Ping failed. */
    U_DIAG_PING_RESPONSE_TRUE = 1 /**< Ping succeeded. */
} uDiagPingResponse_t;

/* ------------------------------------------------------------
 * CALLBACK TYPES
 * ---------------------------------------------------------- */

struct uCxHandle;
typedef void (*uSTARTUP_t)(struct uCxHandle *puCxHandle);
typedef void (*uUEBTC_t)(struct uCxHandle *puCxHandle, int32_t conn_handle, uBtLeAddress_t *bd_addr);
typedef void (*uUEBTDC_t)(struct uCxHandle *puCxHandle, int32_t conn_handle);
typedef void (*uUEBTB_t)(struct uCxHandle *puCxHandle, uBtLeAddress_t *bd_addr, uBtBondStatus_t bond_status);
typedef void (*uUEBTUC_t)(struct uCxHandle *puCxHandle, uBtLeAddress_t *bd_addr, int32_t numeric_value);
typedef void (*uUEBTUPD_t)(struct uCxHandle *puCxHandle, uBtLeAddress_t *bd_addr, int32_t numeric_value);
typedef void (*uUEBTUPE_t)(struct uCxHandle *puCxHandle, uBtLeAddress_t *bd_addr);
typedef void (*uUEBTPHYU_t)(struct uCxHandle *puCxHandle, int32_t conn_handle, int32_t phy_status, int32_t tx_phy, int32_t rx_phy);
typedef void (*uUEBTBGD_t)(struct uCxHandle *puCxHandle, uBtLeAddress_t *bd_addr, int32_t rssi, const char * device_name, uBtDataType_t data_type, uByteArray_t *data);
typedef void (*uUEBTGCN_t)(struct uCxHandle *puCxHandle, int32_t conn_handle, int32_t value_handle, uByteArray_t *hex_data);
typedef void (*uUEBTGCI_t)(struct uCxHandle *puCxHandle, int32_t conn_handle, int32_t value_handle, uByteArray_t *hex_data);
typedef void (*uUEBTGCW_t)(struct uCxHandle *puCxHandle, int32_t conn_handle, int32_t value_handle, uByteArray_t *value, uGattServerOptions_t options);
typedef void (*uUEBTGRR_t)(struct uCxHandle *puCxHandle, int32_t conn_handle, int32_t value_handle);
typedef void (*uUEBTGIC_t)(struct uCxHandle *puCxHandle, int32_t conn_handle, int32_t char_handle);
typedef void (*uUESPSC_t)(struct uCxHandle *puCxHandle, int32_t conn_handle);
typedef void (*uUESPSDC_t)(struct uCxHandle *puCxHandle, int32_t conn_handle);
typedef void (*uUESPSDS_t)(struct uCxHandle *puCxHandle, int32_t conn_handle, uByteArray_t *string_data);
typedef void (*uUESPSDB_t)(struct uCxHandle *puCxHandle, int32_t conn_handle);
typedef void (*uUESPSDA_t)(struct uCxHandle *puCxHandle, int32_t conn_handle, int32_t number_bytes);
typedef void (*uUEWLU_t)(struct uCxHandle *puCxHandle, int32_t wlan_handle, uMacAddress_t *bssid, int32_t channel);
typedef void (*uUEWLD_t)(struct uCxHandle *puCxHandle, int32_t wlan_handle, int32_t reason);
typedef void (*uUEWSNU_t)(struct uCxHandle *puCxHandle);
typedef void (*uUEWSND_t)(struct uCxHandle *puCxHandle);
typedef void (*uUEWSRSI_t)(struct uCxHandle *puCxHandle);
typedef void (*uUEWSRSF_t)(struct uCxHandle *puCxHandle);
typedef void (*uUEWSRSC_t)(struct uCxHandle *puCxHandle, int32_t wlan_handle, uMacAddress_t *bssid, int32_t channel);
typedef void (*uUEWAPNU_t)(struct uCxHandle *puCxHandle);
typedef void (*uUEWAPND_t)(struct uCxHandle *puCxHandle);
typedef void (*uUEWAPU_t)(struct uCxHandle *puCxHandle);
typedef void (*uUEWAPD_t)(struct uCxHandle *puCxHandle);
typedef void (*uUEWAPSA_t)(struct uCxHandle *puCxHandle, uMacAddress_t *mac);
typedef void (*uUEWAPSDA_t)(struct uCxHandle *puCxHandle, uMacAddress_t *mac);
typedef void (*uUESOC_t)(struct uCxHandle *puCxHandle, int32_t socket_handle);
typedef void (*uUESODA_t)(struct uCxHandle *puCxHandle, int32_t socket_handle, int32_t number_bytes);
typedef void (*uUESODS_t)(struct uCxHandle *puCxHandle, int32_t socket_handle, uByteArray_t *string_data);
typedef void (*uUESODSF_t)(struct uCxHandle *puCxHandle, int32_t socket_handle, uSockIpAddress_t *remote_ip, int32_t remote_port, uByteArray_t *string_data);
typedef void (*uUESODB_t)(struct uCxHandle *puCxHandle, int32_t socket_handle);
typedef void (*uUESODBF_t)(struct uCxHandle *puCxHandle, int32_t socket_handle, uSockIpAddress_t *remote_ip, int32_t remote_port);
typedef void (*uUESOCL_t)(struct uCxHandle *puCxHandle, int32_t socket_handle);
typedef void (*uUESOIC_t)(struct uCxHandle *puCxHandle, int32_t socket_handle, uSockIpAddress_t *remote_ip, int32_t listening_socket_handle);
typedef void (*uUEMQC_t)(struct uCxHandle *puCxHandle, int32_t mqtt_id);
typedef void (*uUEMQDC_t)(struct uCxHandle *puCxHandle, int32_t mqtt_id, int32_t disconnect_reason);
typedef void (*uUEMQDA_t)(struct uCxHandle *puCxHandle, int32_t mqtt_id, int32_t message_len);
typedef void (*uUEMQDD_t)(struct uCxHandle *puCxHandle, int32_t mqtt_id, int32_t message_len);
typedef void (*uUEMQPC_t)(struct uCxHandle *puCxHandle, int32_t mqtt_id, int32_t packet_id, int32_t message_len);
typedef void (*uUEMQSC_t)(struct uCxHandle *puCxHandle, int32_t mqtt_id, uMqttSubscribeAction_t subscribe_action);
typedef void (*uUEHTCDC_t)(struct uCxHandle *puCxHandle, int32_t session_id);
typedef void (*uUEHTCRS_t)(struct uCxHandle *puCxHandle, int32_t session_id, int32_t status_code, const char * description);
typedef void (*uUEDGPC_t)(struct uCxHandle *puCxHandle, int32_t transmitted_packets, int32_t received_packets, int32_t packet_loss_rate, int32_t avg_response_time);
typedef void (*uUEDGP_t)(struct uCxHandle *puCxHandle, uDiagPingResponse_t ping_response, int32_t response_time);
typedef void (*uUEDGI_t)(struct uCxHandle *puCxHandle, const char * iperf_output);

/* ------------------------------------------------------------
 * INTERNAL CALLBACK STRUCT
 * ---------------------------------------------------------- */

typedef struct
{
    uSTARTUP_t STARTUP;
    uUEBTC_t UEBTC;
    uUEBTDC_t UEBTDC;
    uUEBTB_t UEBTB;
    uUEBTUC_t UEBTUC;
    uUEBTUPD_t UEBTUPD;
    uUEBTUPE_t UEBTUPE;
    uUEBTPHYU_t UEBTPHYU;
    uUEBTBGD_t UEBTBGD;
    uUEBTGCN_t UEBTGCN;
    uUEBTGCI_t UEBTGCI;
    uUEBTGCW_t UEBTGCW;
    uUEBTGRR_t UEBTGRR;
    uUEBTGIC_t UEBTGIC;
    uUESPSC_t UESPSC;
    uUESPSDC_t UESPSDC;
    uUESPSDS_t UESPSDS;
    uUESPSDB_t UESPSDB;
    uUESPSDA_t UESPSDA;
    uUEWLU_t UEWLU;
    uUEWLD_t UEWLD;
    uUEWSNU_t UEWSNU;
    uUEWSND_t UEWSND;
    uUEWSRSI_t UEWSRSI;
    uUEWSRSF_t UEWSRSF;
    uUEWSRSC_t UEWSRSC;
    uUEWAPNU_t UEWAPNU;
    uUEWAPND_t UEWAPND;
    uUEWAPU_t UEWAPU;
    uUEWAPD_t UEWAPD;
    uUEWAPSA_t UEWAPSA;
    uUEWAPSDA_t UEWAPSDA;
    uUESOC_t UESOC;
    uUESODA_t UESODA;
    uUESODS_t UESODS;
    uUESODSF_t UESODSF;
    uUESODB_t UESODB;
    uUESODBF_t UESODBF;
    uUESOCL_t UESOCL;
    uUESOIC_t UESOIC;
    uUEMQC_t UEMQC;
    uUEMQDC_t UEMQDC;
    uUEMQDA_t UEMQDA;
    uUEMQDD_t UEMQDD;
    uUEMQPC_t UEMQPC;
    uUEMQSC_t UEMQSC;
    uUEHTCDC_t UEHTCDC;
    uUEHTCRS_t UEHTCRS;
    uUEDGPC_t UEDGPC;
    uUEDGP_t UEDGP;
    uUEDGI_t UEDGI;
} uUrcCallbacks;
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _U_CX_TYPES_H_ */