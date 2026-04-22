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
    U_INTERFACE_ID_BLUETOOTH /**< Bluetooth */
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
    U_BT_DATA_TYPE_SCAN_RESPONSE,              /**< Scan response data. */
    U_BT_DATA_TYPE_ADVERTISE_DATA = 1,         /**< Advertise data. */
    U_BT_DATA_TYPE_EXTENDED_ADVERTISE_DATA = 2 /**< Extended advertise data. */
} uBtDataType_t;

typedef enum
{
    U_BT_DISCOVERY_TYPE_ALL,                          /**< Displays all found devices; devices can be displayed multiple times. */
    U_BT_DISCOVERY_TYPE_ALL_NO_DUPLICATES = 1,        /**< Displays all found devices; each device is displayed only once. */
    U_BT_DISCOVERY_TYPE_DATA_FILTER = 2,              /**< Filter discovery results using the configured data filters. Only devices matching a data
                                                           filter configured with AT+UBTDFD will be reported. Devices can be displayed multiple
                                                           times. */
    U_BT_DISCOVERY_TYPE_DATA_FILTER_NO_DUPLICATES = 3 /**< Filter discovery results using the configured data filters. Only devices matching a data
                                                           filter configured with AT+UBTDFD will be reported. Each device is displayed only once. */
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
    U_BT_DISCOVERY_STATE_NOT_ACTIVE, /**< Background discovery is not active */
    U_BT_DISCOVERY_STATE_ACTIVE = 1  /**< Background discovery is active */
} uBtDiscoveryState_t;

typedef enum
{
    U_BT_CONNECT_TO_DIRECTED_ADVERTISEMENTS_DISABLED,   /**< Connect to directed advertisements during background discovery disabled */
    U_BT_CONNECT_TO_DIRECTED_ADVERTISEMENTS_ENABLED = 1 /**< Connect to directed advertisements during background discovery enabled */
} uBtConnectToDirectedAdvertisements_t;

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
    U_BT_ADV_TYPE_CONNECTABLE_SCANNABLE,            /**< Connectable and Scannable */
    U_BT_ADV_TYPE_NON_CONNECTABLE_SCANNABLE = 1,    /**< Non-Connectable Scannable */
    U_BT_ADV_TYPE_NON_CONNECTABLE_NON_SCANNABLE = 2 /**< Non-Connectable Non-Scannable */
} uBtAdvType_t;

typedef enum
{
    U_BT_EXT_ADV_TYPE_CONNECTABLE,        /**< Connectable */
    U_BT_EXT_ADV_TYPE_NON_CONNECTABLE = 1 /**< Non-Connectable */
} uBtExtAdvType_t;

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
    U_BT_CHANNEL_SOUNDING_MODE_DISABLE,      /**< Disable Channel Sounding. */
    U_BT_CHANNEL_SOUNDING_MODE_REFLECTOR = 1 /**< Enable Channel Sounding as reflector. */
} uBtChannelSoundingMode_t;

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
    U_POWER_WAKEUP_MODE_WAKEUP_GPIO,   /**< Wakeup by pulling the module wakeup pin low. */
    U_POWER_WAKEUP_MODE_RESET_PIN = 1, /**< Wakeup by module reset pin. */
    U_POWER_WAKEUP_MODE_UART_RX = 2    /**< Wakeup by module UART RX pin. */
} uPowerWakeupMode_t;

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
    U_BT_CHANNEL_SOUNDING_EVENT_CAPABILITY_EXCHANGE,   /**< CS capability exchange completed. */
    U_BT_CHANNEL_SOUNDING_EVENT_SECURITY_ENABLED = 1,  /**< CS security enabled. */
    U_BT_CHANNEL_SOUNDING_EVENT_PROCEDURE_ENABLED = 2, /**< CS procedure enabled. */
    U_BT_CHANNEL_SOUNDING_EVENT_PROCEDURE_DISABLED = 3 /**< CS procedure disabled. */
} uBtChannelSoundingEvent_t;

typedef enum
{
    U_BT_HCI_STATUS_SUCCESS,                       /**< Operation successful. */
    U_BT_HCI_STATUS_UNKNOWN_CONN_ID = 2,           /**< Unknown connection identifier. */
    U_BT_HCI_STATUS_CMD_DISALLOWED = 12,           /**< Command disallowed in current state. */
    U_BT_HCI_STATUS_UNSUPP_FEATURE_PARAM_VAL = 17, /**< Unsupported feature or parameter value. */
    U_BT_HCI_STATUS_UNSUPP_REMOTE_FEATURE = 26,    /**< Unsupported remote feature. */
    U_BT_HCI_STATUS_UNSPECIFIED = 31               /**< Unspecified error. */
} uBtHciStatus_t;

typedef enum
{
    U_GATT_SERVER_OPTIONS_WRITE_WITH_OUT_RESPONSE, /**< Write without Response performed */
    U_GATT_SERVER_OPTIONS_WRITE_WITH_RESPONSE = 1, /**< Write with Response performed */
    U_GATT_SERVER_OPTIONS_WRITE_LONG = 2           /**< Write long performed */
} uGattServerOptions_t;

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
typedef void (*uUEBTCSS_t)(struct uCxHandle *puCxHandle, int32_t conn_handle, uBtChannelSoundingEvent_t channel_sounding_event, uBtHciStatus_t hci_status);
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
    uUEBTCSS_t UEBTCSS;
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
} uUrcCallbacks;
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _U_CX_TYPES_H_ */