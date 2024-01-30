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
    U_INTERFACE_ID_BLUETOOTH,         /**< Bluetooth */
    U_INTERFACE_ID_WI_FI_STATION = 1, /**< Wi-Fi station */
    U_INTERFACE_ID_WI_FI_AP = 2       /**< Wi-Fi Access point */
} uInterfaceId_t;

typedef enum
{
    U_EXTENDED_ERRORS_OFF,   /**< (Factory default) Extended error codes will not be displayed */
    U_EXTENDED_ERRORS_ON = 1 /**< Extended error code will be displayed on every error */
} uExtendedErrors_t;

typedef enum
{
    U_ECHO_ON_OFF,   /**< Module does not echo the characters */
    U_ECHO_ON_ON = 1 /**< (Factory default) Module echoes the characters */
} uEchoOn_t;

typedef enum
{
    U_BT_MODE_DISABLED,              /**< Disabled. This is the factory default for NORA-W36. */
    U_BT_MODE_CENTRAL = 1,           /**< Bluetooth Low Energy Central.
                                          In this mode, starting advertisements, direct advertisements and other functions
                                          associated
                                          with the Peripheral role is not possible. */
    U_BT_MODE_PERIPHERAL = 2,        /**< Bluetooth Low Energy Peripheral.
                                          In this mode, initiating connections, discovery and other functions associated with
                                          the Central role is not possible. */
    U_BT_MODE_CENTRAL_PERIPHERAL = 3 /**< Bluetooth Low Energy Simultaneous Central and Peripheral. */
} uBtMode_t;

typedef enum
{
    U_DATA_TYPE_SCAN_RESPONSE,     /**< Scan response data. */
    U_DATA_TYPE_ADVERTISE_DATA = 1 /**< Advertise data. */
} uDataType_t;

typedef enum
{
    U_DISCOVERY_TYPE_DISCOVER_ALL,                  /**< All with no filter. Displays all found devices; devices can be displayed multiple times. */
    U_DISCOVERY_TYPE_DISCOVER_ALL_NO_DUPLICATES = 1 /**< General inquiry. Displays devices in General or Limited discoverability mode; each device
                                                         is displayed only once. */
} uDiscoveryType_t;

typedef enum
{
    U_DISCOVERY_MODE_ACTIVE,     /**< Active discovery. */
    U_DISCOVERY_MODE_PASSIVE = 1 /**< Passive, no scan response data will be received. */
} uDiscoveryMode_t;

typedef enum
{
    U_BACKGROUND_DISCOVERY_MODE_BACKGROUND_DISCOVERY_OFF,   /**< Set background discovery off */
    U_BACKGROUND_DISCOVERY_MODE_BACKGROUND_DISCOVERY_ON = 1 /**< Set background discovery on */
} uBackgroundDiscoveryMode_t;

typedef enum
{
    U_PROPERTY_ID_CONNECTION_INTERVAL,       /**< Connection interval used on this connection.
                                                   Range: 6 to 3200
                                                   Time = status_val * 1.25 ms
                                                   Time range: 7.5 ms to 4000 ms */
    U_PROPERTY_ID_PERIPHERAL_LATENCY = 1,    /**< Peripheral latency for the connection in number of connection events. Range: 0 to 499 */
    U_PROPERTY_ID_SUPERVISION_TIMEOUT = 2,   /**< Supervision timeout (in ms) for this connections. Range: 100 ms to 32000 ms */
    U_PROPERTY_ID_MTU_SIZE = 3,              /**< MTU size for this connections. */
    U_PROPERTY_ID_PDU_TX_PAYLOAD_LENGTH = 4, /**< Data Channel TX PDU Payload Length. */
    U_PROPERTY_ID_PDU_RX_PAYLOAD_LENGTH = 5, /**< Data Channel RX PDU Payload Length. */
    U_PROPERTY_ID_DATA_LENGTH_EXTENSION = 6, /**< Data Length Extension state. 0: Data Length Extension Off \ 1: Data Length Extension On */
    U_PROPERTY_ID_LOCAL_ROLE = 7,            /**< Local role in this connection. 1: Low Energy Central \ 2: Low Energy Peripheral */
    U_PROPERTY_ID_TX_PHY = 8,                /**< TX Phy used in this connection
                                                  Bit 0: 1 Mbps
                                                  Bit 1: 2 Mbps
                                                  Bit 2: reserved for future use */
    U_PROPERTY_ID_RX_PHY = 9                 /**< RX Phy used in this connection
                                                  Bit 0: 1 Mbps
                                                  Bit 1: 2 Mbps
                                                  Bit 2: reserved for future use */
} uPropertyId_t;

typedef enum
{
    U_ADV_MODE_ADVERTISEMENTS_OFF,   /**< Set Bluetooth Advertisements off */
    U_ADV_MODE_ADVERTISEMENTS_ON = 1 /**< Set Bluetooth Advertisements on */
} uAdvMode_t;

typedef enum
{
    U_IO_CAPABILITIES_NO_INPUT_NO_OUTPUT,  /**< Set I/O Capabilities to No Input No Output. */
    U_IO_CAPABILITIES_DISPLAY_ONLY = 1,    /**< Set I/O Capabilities to Display Only. */
    U_IO_CAPABILITIES_DISPLAY_YES_NO = 2,  /**< Set I/O Capabilities to Display Yes/No */
    U_IO_CAPABILITIES_KEYBOARD_ONLY = 3,   /**< Set I/O Capabilities to Keyboard Only. */
    U_IO_CAPABILITIES_KEYBOARD_DISPLAY = 4 /**< Set I/O Capabilities to Keyboard Display. */
} uIoCapabilities_t;

typedef enum
{
    U_BT_SECURITY_MODE_NONE,                                    /**< Security Disabled. */
    U_BT_SECURITY_MODE_UNAUTHENTICATED = 1,                     /**< Allow unauthenticated bonding. */
    U_BT_SECURITY_MODE_AUTHENTICATED = 2,                       /**< Only allow authenticated bonding. */
    U_BT_SECURITY_MODE_AUTHENTICATED_SECURE_CONNECTION = 3,     /**< Only allow authenticated bonding with encrypted Bluetooth link. Fallback to simple pairing
                                                                     if the remote side does not support secure connections. */
    U_BT_SECURITY_MODE_AUTHENTICATED_SECURE_CONNECTION_ONLY = 4 /**< Only allow authenticated bonding with encrypted Bluetooth link. Strictly uses secure
                                                                     connections. */
} uBtSecurityMode_t;

typedef enum
{
    U_PAIRING_MODE_PAIRING_MODE_DISABLE,   /**< Disable pairing mode. */
    U_PAIRING_MODE_PAIRING_MODE_ENABLE = 1 /**< Enable pairing mode. */
} uPairingMode_t;

typedef enum
{
    U_YES_NO_NO,     /**< Deny bonding. */
    U_YES_NO_YES = 1 /**< Confirm bonding. */
} uYesNo_t;

typedef enum
{
    U_CHARACTERISTIC_ID_MANUFACTURER_NAME,     /**< Manufacturer name string. Maximum length of the custom string is 31 characters. */
    U_CHARACTERISTIC_ID_MODEL_NAME = 1,        /**< Model name string. Maximum length of the custom string is 20 characters. */
    U_CHARACTERISTIC_ID_FIRMWARE_REVISION = 2, /**< Firmware revision string. Maximum length of the custom string is 20 characters. */
    U_CHARACTERISTIC_ID_SOFTWARE_REVISION = 3  /**< Software revision string. Maximum length of the custom string is 20 characters. */
} uCharacteristicId_t;

typedef enum
{
    U_CONFIG_NONE,                     /**< None */
    U_CONFIG_ENABLE_NOTIFICATIONS = 1, /**< Enable notifications */
    U_CONFIG_ENABLE_INDICATIONS = 2,   /**< Enable indications */
    U_CONFIG_ENABLE_NOT_IND = 3        /**< Enable notifications and indications */
} uConfig_t;

typedef enum
{
    U_RELIABLE_NOT_RELIABLE, /**< Not reliable */
    U_RELIABLE_RELIABLE = 1  /**< Reliable */
} uReliable_t;

typedef enum
{
    U_FLAG_FINAL_DATA,             /**< Final data */
    U_FLAG_MORE_DATA = 1,          /**< More data */
    U_FLAG_CANCEL_DATA_WRITING = 2 /**< Cancel */
} uFlag_t;

typedef enum
{
    U_SECURITY_READ_NONE = 1,            /**< No encryption required. */
    U_SECURITY_READ_UNAUTHENTICATED = 2, /**< Unauthenticated encryption required. */
    U_SECURITY_READ_AUTHENTICATED = 3    /**< Authenticated encryption required. */
} uSecurityRead_t;

typedef enum
{
    U_SECURITY_WRITE_NONE = 1,            /**< No encryption required. */
    U_SECURITY_WRITE_UNAUTHENTICATED = 2, /**< Unauthenticated encryption required. */
    U_SECURITY_WRITE_AUTHENTICATED = 3    /**< Authenticated encryption required. */
} uSecurityWrite_t;

typedef enum
{
    U_SPS_SERVICE_OPTION_DISABLE_SPS_SERVICE,   /**< This option disables the SPS service after saving the configuration and restarting the
                                                     device. (Default) */
    U_SPS_SERVICE_OPTION_ENABLE_SPS_SERVICE = 1 /**< This option enables the SPS service directly.
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
    U_WPA_THRESHOLD_WPA2,    /**< Only connect to access points that support WPA2 or up */
    U_WPA_THRESHOLD_WPA3 = 1 /**< Only connect to access points that support WPA3 */
} uWpaThreshold_t;

typedef enum
{
    U_STATUS_ID_IPV4,         /**< The current IPv4 address. */
    U_STATUS_ID_SUBNET = 1,   /**< The current subnet mask */
    U_STATUS_ID_GATE_WAY = 2, /**< The current gateway */
    U_STATUS_ID_PRIM_DNS = 3, /**< The current primary DNS server */
    U_STATUS_ID_SEC_DNS = 4,  /**< The current secondary DNS server */
    U_STATUS_ID_IPV6 = 5      /**< The current IPv6 link local address */
} uStatusId_t;

typedef enum
{
    U_SCAN_MODE_ACTIVE,     /**< Active */
    U_SCAN_MODE_PASSIVE = 1 /**< Passive */
} uScanMode_t;

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
    U_CHANNEL1 = 1,   /**< 1 */
    U_CHANNEL2 = 2,   /**< 2 */
    U_CHANNEL3 = 3,   /**< 3 */
    U_CHANNEL4 = 4,   /**< 4 */
    U_CHANNEL5 = 5,   /**< 5 */
    U_CHANNEL6 = 6,   /**< 6 */
    U_CHANNEL7 = 7,   /**< 7 */
    U_CHANNEL8 = 8,   /**< 8 */
    U_CHANNEL9 = 9,   /**< 9 */
    U_CHANNEL10 = 10, /**< 10 */
    U_CHANNEL11 = 11, /**< 11 */
    U_CHANNEL36 = 36, /**< 36 */
    U_CHANNEL40 = 40, /**< 40 */
    U_CHANNEL44 = 44, /**< 44 */
    U_CHANNEL48 = 48  /**< 48 */
} uChannel_t;

typedef enum
{
    U_WPA_VERSION_WPA2 /**< WPA 2 */
} uWpaVersion_t;

typedef enum
{
    U_PROTOCOL_TCP = 6, /**< TCP */
    U_PROTOCOL_UDP = 17 /**< UDP */
} uProtocol_t;

typedef enum
{
    U_PREFERRED_PROTOCOL_TYPE_IP_V4,    /**< IPv4 address. */
    U_PREFERRED_PROTOCOL_TYPE_IP_V6 = 1 /**< IPv6 address. */
} uPreferredProtocolType_t;

typedef enum
{
    U_TLS_VERSION_NO_TLS,    /**< Disable TLS */
    U_TLS_VERSION_TLS1_2 = 1 /**< TLS 1.2 or up */
} uTlsVersion_t;

typedef enum
{
    U_SOCKET_STATUS_NOT_CONNECTED, /**< Not Connected */
    U_SOCKET_STATUS_LISTENING = 1, /**< Listening */
    U_SOCKET_STATUS_CONNECTED = 2  /**< Connected */
} uSocketStatus_t;

typedef enum
{
    U_OPTION_NO_DELAY,       /**< Turn on/off No delay feature for TCP sockets.
                                  Integer flag: 0 = off (i.e. Nagle algorithm enabled), 1 = on  (i.e. Nagle algorithm
                                  disabled)
                                  Default: to 0 */
    U_OPTION_BLOCK = 1,      /**< Set socket to be blocking or non blocking.
                                  Integer flag: 0 = off, 1 = on.
                                  Sockets are non-blocking by default (Note that read/write will always be non-blocking).
                                  Can only be set while the socket is in a non connected state.
                                  Note: Only valid for non-persistent TCP sockets, will have no effect on UDP sockets */
    U_OPTION_KEEP_ALIVE = 2, /**< Keep connections alive by sending keepalive probes.
                                  Integer flag: 0 = off, 1 = on.
                                  To calculate the keepalive time us this formula KeepIdle + (KeepIntvl * KeepCnt).
                                  Defaults to 1.
                                  Note: Only valid for TCP sockets. */
    U_OPTION_KEEP_IDLE = 3,  /**< Set Keep Idle value for the socket.
                                  This specifies the amount of time (in sec) that the connection must be idle before sending
                                  keepalive probes (if keepalive is enabled).
                                  Defaults to 3.
                                  Note: Only valid for TCP sockets. */
    U_OPTION_KEEP_INTVL = 4, /**< Set keep alive interval value for the socket. This is the time in seconds between two
                                  successive keepalive retransmissions.
                                  Defaults to 3.
                                  Note: Only valid for TCP sockets. */
    U_OPTION_KEEP_CNT = 5    /**< Set keep alive counter value for the socket.
                                  The number of unanswered probes required to force closure of the socket.
                                  Defaults to 3.
                                  Note: Only valid for TCP sockets. */
} uOption_t;

typedef enum
{
    U_QOS_AT_MOST_ONCE,      /**< At most once */
    U_QOS_AT_LEAST_ONCE = 1, /**< At least once */
    U_QOS_EXACTLY_ONCE = 2   /**< Exactly once */
} uQos_t;

typedef enum
{
    U_RETAIN_NO_RETAIN, /**< Do not retain message on broker */
    U_RETAIN_RETAIN = 1 /**< Retain message on broker */
} uRetain_t;

typedef enum
{
    U_SUBSCRIBE_ACTION_SUBSCRIBE,      /**< Subscribe to topic */
    U_SUBSCRIBE_ACTION_UNSUBSCRIBE = 1 /**< Unsubscribe from topic */
} uSubscribeAction_t;

typedef enum
{
    U_CERT_TYPEROOT,       /**< Root certificate */
    U_CERT_TYPECLIENT = 1, /**< Client certificate */
    U_CERT_TYPEKEY = 2     /**< Client private key */
} uCertType_t;

typedef enum
{
    U_EXTENSION_SNI,              /**< Server Name Extension */
    U_EXTENSION_FRAGMENTATION = 1 /**< Handshake fragmentation */
} uExtension_t;

typedef enum
{
    U_ENABLED_DISABLED,   /**< Disabled */
    U_ENABLED_ENABLED = 1 /**< Enabled */
} uEnabled_t;

typedef enum
{
    U_IPERF_ACTION_START = 1, /**< Start iperf */
    U_IPERF_ACTION_STOP = 2   /**< Stop iperf */
} uIperfAction_t;

typedef enum
{
    U_PROTOCOL_TYPE_TCP = 1, /**< TCP */
    U_PROTOCOL_TYPE_UDP = 2  /**< UDP */
} uProtocolType_t;

typedef enum
{
    U_ROLE_SERVER = 1, /**< Server */
    U_ROLE_CLIENT = 2  /**< Client */
} uRole_t;

typedef enum
{
    U_BIDIRECTIONAL_OFF,   /**< Off */
    U_BIDIRECTIONAL_ON = 1 /**< On
                                When starting bidirectional TCP test, start a server on both tester and DUT, then start a
                                client with bidirectional flag on the DUT.
                                If doing bidirectional UDP test, start a server on both DUT and tester and then start a
                                client with a bidirectional flag on both. */
} uBidirectional_t;

typedef enum
{
    U_BOND_STATUS_BONDING_SUCCEEDED,          /**< Bonding procedure succeeded. */
    U_BOND_STATUS_BONDING_FAILED_TIMEOUT = 1, /**< Bonding procedure failed due to page timeout. */
    U_BOND_STATUS_BONDING_FAILED_AUTH = 2,    /**< Bonding failed because of authentication or pairing failed. This could be due to incorrect
                                                   PIN/passkey. */
    U_BOND_STATUS_BONDING_FAILED_MITM = 3     /**< Bonding failed because the protection against Man-In-The-Middle attack could not be
                                                   guaranteed; the generated link key was too weak. */
} uBondStatus_t;

typedef enum
{
    U_OPTIONS_WRITE_WITH_OUT_RESPONSE, /**< Write without Response performed */
    U_OPTIONS_WRITE_WITH_RESPONSE = 1, /**< Write with Response performed */
    U_OPTIONS_WRITE_LONG = 2           /**< Write long performed */
} uOptions_t;

typedef enum
{
    U_PING_RESPONSE_FALSE,   /**< Ping failed. */
    U_PING_RESPONSE_TRUE = 1 /**< Ping succeeded. */
} uPingResponse_t;

/* ------------------------------------------------------------
 * CALLBACK TYPES
 * ---------------------------------------------------------- */

struct uCxHandle;
typedef void (*uUEBTC_t)(struct uCxHandle *puCxHandle, int32_t conn_handle, uBtLeAddress_t *bd_addr);
typedef void (*uUEBTDC_t)(struct uCxHandle *puCxHandle, int32_t conn_handle);
typedef void (*uUEBTB_t)(struct uCxHandle *puCxHandle, uBtLeAddress_t *bd_addr, uBondStatus_t bond_status);
typedef void (*uUEBTUC_t)(struct uCxHandle *puCxHandle, uBtLeAddress_t *bd_addr, int32_t numeric_value);
typedef void (*uUEBTUPD_t)(struct uCxHandle *puCxHandle, uBtLeAddress_t *bd_addr, int32_t numeric_value);
typedef void (*uUEBTUPE_t)(struct uCxHandle *puCxHandle, uBtLeAddress_t *bd_addr);
typedef void (*uUEBTPHYU_t)(struct uCxHandle *puCxHandle, int32_t conn_handle, int32_t phy_status, int32_t tx_phy, int32_t rx_phy);
typedef void (*uUEBTGCN_t)(struct uCxHandle *puCxHandle, int32_t conn_handle, int32_t value_handle, uByteArray_t *hex_data);
typedef void (*uUEBTGCI_t)(struct uCxHandle *puCxHandle, int32_t conn_handle, int32_t value_handle, uByteArray_t *hex_data);
typedef void (*uUEBTGCW_t)(struct uCxHandle *puCxHandle, int32_t conn_handle, int32_t value_handle, uByteArray_t *value, uOptions_t options);
typedef void (*uUEBTGRR_t)(struct uCxHandle *puCxHandle, int32_t conn_handle, int32_t value_handle);
typedef void (*uUEBTGIC_t)(struct uCxHandle *puCxHandle, int32_t conn_handle, int32_t char_handle);
typedef void (*uUESPSC_t)(struct uCxHandle *puCxHandle, int32_t conn_handle);
typedef void (*uUESPSDC_t)(struct uCxHandle *puCxHandle, int32_t conn_handle);
typedef void (*uUESPSDS_t)(struct uCxHandle *puCxHandle, int32_t conn_handle, const char * string_data);
typedef void (*uUESPSDB_t)(struct uCxHandle *puCxHandle, int32_t conn_handle);
typedef void (*uUESPSDA_t)(struct uCxHandle *puCxHandle, int32_t conn_handle, int32_t number_bytes);
typedef void (*uUEWLU_t)(struct uCxHandle *puCxHandle, int32_t wlan_handle, uMacAddress_t *bssid, int32_t channel);
typedef void (*uUEWLD_t)(struct uCxHandle *puCxHandle, int32_t wlan_handle, int32_t reason);
typedef void (*uUEWSNU_t)(struct uCxHandle *puCxHandle);
typedef void (*uUEWSND_t)(struct uCxHandle *puCxHandle);
typedef void (*uUEWAPNU_t)(struct uCxHandle *puCxHandle);
typedef void (*uUEWAPND_t)(struct uCxHandle *puCxHandle);
typedef void (*uUEWAPU_t)(struct uCxHandle *puCxHandle);
typedef void (*uUEWAPD_t)(struct uCxHandle *puCxHandle);
typedef void (*uUEWAPSA_t)(struct uCxHandle *puCxHandle, uMacAddress_t *mac);
typedef void (*uUEWAPSDA_t)(struct uCxHandle *puCxHandle, uMacAddress_t *mac);
typedef void (*uUESOC_t)(struct uCxHandle *puCxHandle, int32_t socket_handle);
typedef void (*uUESODA_t)(struct uCxHandle *puCxHandle, int32_t socket_handle, int32_t number_bytes);
typedef void (*uUESODS_t)(struct uCxHandle *puCxHandle, int32_t socket_handle, const char * string_data);
typedef void (*uUESODSF_t)(struct uCxHandle *puCxHandle, int32_t socket_handle, uSockIpAddress_t *remote_ip, int32_t remote_port, const char * string_data);
typedef void (*uUESODB_t)(struct uCxHandle *puCxHandle, int32_t socket_handle);
typedef void (*uUESODBF_t)(struct uCxHandle *puCxHandle, int32_t socket_handle, uSockIpAddress_t *remote_ip, int32_t remote_port);
typedef void (*uUESOCL_t)(struct uCxHandle *puCxHandle, int32_t socket_handle);
typedef void (*uUESOIC_t)(struct uCxHandle *puCxHandle, int32_t socket_handle, uSockIpAddress_t *remote_ip, int32_t listening_socket_handle);
typedef void (*uUEMQC_t)(struct uCxHandle *puCxHandle, int32_t mqtt_id);
typedef void (*uUEMQDC_t)(struct uCxHandle *puCxHandle, int32_t mqtt_id, int32_t disconnect_reason);
typedef void (*uUEMQDA_t)(struct uCxHandle *puCxHandle, int32_t mqtt_id, int32_t message_len);
typedef void (*uUEDGPC_t)(struct uCxHandle *puCxHandle, int32_t transmitted_packets, int32_t received_packets, int32_t packet_loss_rate, int32_t avg_response_time);
typedef void (*uUEDGP_t)(struct uCxHandle *puCxHandle, uPingResponse_t ping_response, int32_t response_time);
typedef void (*uUEDGI_t)(struct uCxHandle *puCxHandle, const char * iperf_output);

/* ------------------------------------------------------------
 * INTERNAL CALLBACK STRUCT
 * ---------------------------------------------------------- */

typedef struct
{
    uUEBTC_t UEBTC;
    uUEBTDC_t UEBTDC;
    uUEBTB_t UEBTB;
    uUEBTUC_t UEBTUC;
    uUEBTUPD_t UEBTUPD;
    uUEBTUPE_t UEBTUPE;
    uUEBTPHYU_t UEBTPHYU;
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
    uUEDGPC_t UEDGPC;
    uUEDGP_t UEDGP;
    uUEDGI_t UEDGI;
} uUrcCallbacks;
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _U_CX_TYPES_H_ */