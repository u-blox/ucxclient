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
#ifndef _U_CX_BLUETOOTH_H_
#define _U_CX_BLUETOOTH_H_

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
    uBtLeAddress_t bd_addr;   /**< Bluetooth device address of the remote device. */
    int32_t rssi;             /**< Received signal strength in dBm. */
    const char * device_name; /**< Name of the discovered device. */
    int32_t data_type;
    uByteArray_t data;        /**< Complete advertise/scan response data received from the remote device. */
} uCxBluetoothDiscovery_t;

typedef struct
{
    uBtLeAddress_t bd_addr;   /**< Bluetooth device address of the remote device. */
    int32_t rssi;             /**< Received signal strength in dBm. */
    const char * device_name; /**< Name of the discovered device. */
    int32_t data_type;
    uByteArray_t data;        /**< Complete advertise/scan response data received from the remote device. */
} uCxBluetoothDiscoveryEx_t;

typedef struct
{
    int32_t conn_handle;    /**< Connection handle of the Bluetooth low energy connection. */
    uBtLeAddress_t bd_addr; /**< Bluetooth device address of the remote device. */
} uCxBluetoothListConnections_t;

typedef struct
{
    int32_t property_id;
    int32_t status_val;  /**< Value of the preceding property. */
} uCxBluetoothListConnectionStatus_t;

typedef struct
{
    int32_t property_id;
    int32_t status_val;  /**< Value of the preceding property. */
} uCxBluetoothGetConnectionStatus_t;

typedef struct
{
    int32_t characteristic_id;
    const char * characteristic_value; /**< Value of Device Information Service characterstic. */
} uCxBluetoothSetDeviceInfoServiceChar_t;

typedef struct
{
    int32_t characteristic_id;
    const char * characteristic_value; /**< Value of Device Information Service characterstic. */
} uCxBluetoothListDeviceInfoServiceChars_t;

typedef struct
{
    int32_t conn_handle; /**< Connection handle of the Bluetooth low energy connection. */
    int32_t tx_phy;      /**< Requested PHY for Transmitter:
                              0: Let other side decide
                              OR a bit field with three bits:
                              Bit 0: 1 Mbps preferred
                              Bit 1: 2 Mbps preferred
                              Bit 2: Coded PHY (S=8). Not supported by NORA-W36 */
    int32_t rx_phy;      /**< Requested PHY for Receiver
                              0: Let other side decide
                              OR a bit field with three bits:
                              Bit 0: 1 Mbps preferred
                              Bit 1: 2 Mbps preferred
                              Bit 2: Coded PHY (S=8). Not supported by NORA-W36 */
} uCxBluetoothGetPhy_t;


/* ------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * ---------------------------------------------------------- */

/**
 * Set Bluetooth Mode.
 * 
 * Output AT command:
 * > AT+UBTM=<bt_mode>
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      bt_mode:    
 */
int32_t uCxBluetoothSetMode(uCxHandle_t * puCxHandle, uBtMode_t bt_mode);

/**
 * Read Bluetooth Mode.
 * 
 * Output AT command:
 * > AT+UBTM?
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param[out] pBtMode:    
 */
int32_t uCxBluetoothGetMode(uCxHandle_t * puCxHandle, uBtMode_t * pBtMode);

/**
 * Initiate connection.
 * 
 * Output AT command:
 * > AT+UBTC=<bd_addr>
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      bd_addr:    Bluetooth device address of the remote device.
 */
int32_t uCxBluetoothConnect(uCxHandle_t * puCxHandle, uBtLeAddress_t * bd_addr);

/**
 * Close an existing ACL connection.
 * 
 * Output AT command:
 * > AT+UBTDC=<conn_handle>
 *
 * @param[in]  puCxHandle:  uCX API handle
 * @param      conn_handle: Connection handle of the Bluetooth low energy connection.
 */
int32_t uCxBluetoothDisconnect(uCxHandle_t * puCxHandle, int32_t conn_handle);

/**
 * Reads the local Bluetooth device name.
 * 
 * Output AT command:
 * > AT+UBTLN?
 *
 * @param[in]  puCxHandle:   uCX API handle
 * @param[out] ppDeviceName: For Bluetooth low energy the maximum size is 29 characters.
 */
bool uCxBeginBluetoothGetLocalName(uCxHandle_t * puCxHandle, const char ** ppDeviceName);

/**
 * Writes the local Bluetooth device name.
 * 
 * Output AT command:
 * > AT+UBTLN=<device_name>
 *
 * @param[in]  puCxHandle:  uCX API handle
 * @param      device_name: For Bluetooth low energy the maximum size is 29 characters.
 */
int32_t uCxBluetoothSetLocalName(uCxHandle_t * puCxHandle, const char * device_name);

/**
 * Start discovery using default parameters
 * 
 * Output AT command:
 * > AT+UBTD
 *
 * @param[in]  puCxHandle: uCX API handle
 */
void uCxBeginBluetoothDiscovery(uCxHandle_t * puCxHandle);

/**
 * 
 *
 * @param[in]  puCxHandle:             uCX API handle
 * @param[out] pBluetoothDiscoveryRsp: Please see \ref uCxBluetoothDiscovery_t
 */
bool uCxBluetoothDiscoveryGetResponse(uCxHandle_t * puCxHandle, uCxBluetoothDiscovery_t * pBluetoothDiscoveryRsp);

/**
 * Start discovery.
 * 
 * Output AT command:
 * > AT+UBTD=<discovery_type>
 *
 * @param[in]  puCxHandle:     uCX API handle
 * @param      discovery_type: 
 */
void uCxBeginBluetoothDiscoveryEx1(uCxHandle_t * puCxHandle, uDiscoveryType_t discovery_type);

/**
 * 
 *
 * @param[in]  puCxHandle:               uCX API handle
 * @param[out] pBluetoothDiscoveryExRsp: Please see \ref uCxBluetoothDiscoveryEx_t
 */
bool uCxBluetoothDiscoveryExGetResponse1(uCxHandle_t * puCxHandle, uCxBluetoothDiscoveryEx_t * pBluetoothDiscoveryExRsp);

/**
 * Start discovery.
 * 
 * Output AT command:
 * > AT+UBTD=<discovery_type>,<discovery_mode>
 *
 * @param[in]  puCxHandle:     uCX API handle
 * @param      discovery_type: 
 * @param      discovery_mode: 
 */
void uCxBeginBluetoothDiscoveryEx2(uCxHandle_t * puCxHandle, uDiscoveryType_t discovery_type, uDiscoveryMode_t discovery_mode);

/**
 * 
 *
 * @param[in]  puCxHandle:               uCX API handle
 * @param[out] pBluetoothDiscoveryExRsp: Please see \ref uCxBluetoothDiscoveryEx_t
 */
bool uCxBluetoothDiscoveryExGetResponse2(uCxHandle_t * puCxHandle, uCxBluetoothDiscoveryEx_t * pBluetoothDiscoveryExRsp);

/**
 * Start discovery.
 * 
 * Output AT command:
 * > AT+UBTD=<discovery_type>,<discovery_mode>,<discovery_length>
 *
 * @param[in]  puCxHandle:       uCX API handle
 * @param      discovery_type:   
 * @param      discovery_mode:   
 * @param      discovery_length: Timeout measured in milliseconds. Time range: 10 ms - 40 s
 */
void uCxBeginBluetoothDiscoveryEx3(uCxHandle_t * puCxHandle, uDiscoveryType_t discovery_type, uDiscoveryMode_t discovery_mode, int32_t discovery_length);

/**
 * 
 *
 * @param[in]  puCxHandle:               uCX API handle
 * @param[out] pBluetoothDiscoveryExRsp: Please see \ref uCxBluetoothDiscoveryEx_t
 */
bool uCxBluetoothDiscoveryExGetResponse3(uCxHandle_t * puCxHandle, uCxBluetoothDiscoveryEx_t * pBluetoothDiscoveryExRsp);

/**
 * Start/Stop background discovery
 * 
 * Output AT command:
 * > AT+UBTBGD=<background_discovery_mode>
 *
 * @param[in]  puCxHandle:                uCX API handle
 * @param      background_discovery_mode: 
 */
int32_t uCxBluetoothSetBgDiscovery(uCxHandle_t * puCxHandle, uBackgroundDiscoveryMode_t background_discovery_mode);

/**
 * Read background discovery mode
 * 
 * Output AT command:
 * > AT+UBTBGD?
 *
 * @param[in]  puCxHandle:               uCX API handle
 * @param[out] pBackgroundDiscoveryMode: 
 */
int32_t uCxBluetoothGetBgDiscovery(uCxHandle_t * puCxHandle, uBackgroundDiscoveryMode_t * pBackgroundDiscoveryMode);

/**
 * Returns the current RSSI for a specified Bluetooth connection.
 * 
 * Output AT command:
 * > AT+UBTRSS=<conn_handle>
 *
 * @param[in]  puCxHandle:  uCX API handle
 * @param      conn_handle: Connection handle of the Bluetooth low energy connection.
 * @param[out] pRssi:       Received signal strength in dBm.
 */
int32_t uCxBluetoothRssi(uCxHandle_t * puCxHandle, int32_t conn_handle, int32_t * pRssi);

/**
 * List all Bluetooth low energy ACL connections.
 * 
 * Output AT command:
 * > AT+UBTCL
 *
 * @param[in]  puCxHandle: uCX API handle
 */
void uCxBeginBluetoothListConnections(uCxHandle_t * puCxHandle);

/**
 * 
 *
 * @param[in]  puCxHandle:                   uCX API handle
 * @param[out] pBluetoothListConnectionsRsp: Please see \ref uCxBluetoothListConnections_t
 */
bool uCxBluetoothListConnectionsGetResponse(uCxHandle_t * puCxHandle, uCxBluetoothListConnections_t * pBluetoothListConnectionsRsp);

/**
 * Read all properties of an existing Bluetooth low energy ACL connection.
 * 
 * Output AT command:
 * > AT+UBTCST=<conn_handle>
 *
 * @param[in]  puCxHandle:  uCX API handle
 * @param      conn_handle: Connection handle of the Bluetooth low energy connection.
 */
void uCxBeginBluetoothListConnectionStatus(uCxHandle_t * puCxHandle, int32_t conn_handle);

/**
 * 
 *
 * @param[in]  puCxHandle:                        uCX API handle
 * @param[out] pBluetoothListConnectionStatusRsp: Please see \ref uCxBluetoothListConnectionStatus_t
 */
bool uCxBluetoothListConnectionStatusGetResponse(uCxHandle_t * puCxHandle, uCxBluetoothListConnectionStatus_t * pBluetoothListConnectionStatusRsp);

/**
 * Read a specific property of an existing Bluetooth low energy ACL connection.
 * 
 * Output AT command:
 * > AT+UBTCST=<conn_handle>,<property_id>
 *
 * @param[in]  puCxHandle:                       uCX API handle
 * @param      conn_handle:                      Connection handle of the Bluetooth low energy connection.
 * @param      property_id:                      
 * @param[out] pBluetoothGetConnectionStatusRsp: Please see \ref uCxBluetoothGetConnectionStatus_t
 */
int32_t uCxBluetoothGetConnectionStatus(uCxHandle_t * puCxHandle, int32_t conn_handle, uPropertyId_t property_id, uCxBluetoothGetConnectionStatus_t * pBluetoothGetConnectionStatusRsp);

/**
 * Write custom advertising data.
 * 
 * Output AT command:
 * > AT+UBTAD=<adv_data>,<adv_data_len>
 *
 * @param[in]  puCxHandle:   uCX API handle
 * @param      adv_data:     
 * @param      adv_data_len: length of adv_data
 */
int32_t uCxBluetoothSetAdvertiseData(uCxHandle_t * puCxHandle, const uint8_t * adv_data, int32_t adv_data_len);

/**
 * Read custom advertising data.
 * 
 * Output AT command:
 * > AT+UBTAD?
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param[out] pAdvData:   
 */
bool uCxBeginBluetoothGetAdvertiseData(uCxHandle_t * puCxHandle, uByteArray_t * pAdvData);

/**
 * Write scan response data.
 * 
 * Output AT command:
 * > AT+UBTSD=<scan_rsp_data>,<scan_rsp_data_len>
 *
 * @param[in]  puCxHandle:        uCX API handle
 * @param      scan_rsp_data:     
 * @param      scan_rsp_data_len: length of scan_rsp_data
 */
int32_t uCxBluetoothSetScanResponseData(uCxHandle_t * puCxHandle, const uint8_t * scan_rsp_data, int32_t scan_rsp_data_len);

/**
 * Read scan response data.
 * 
 * Output AT command:
 * > AT+UBTSD?
 *
 * @param[in]  puCxHandle:   uCX API handle
 * @param[out] pScanRspData: 
 */
bool uCxBeginBluetoothGetScanResponseData(uCxHandle_t * puCxHandle, uByteArray_t * pScanRspData);

/**
 * Set advertisements on or off.
 * 
 * Output AT command:
 * > AT+UBTA=<adv_mode>
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      adv_mode:   
 */
int32_t uCxBluetoothSetAdvertisements(uCxHandle_t * puCxHandle, uAdvMode_t adv_mode);

/**
 * Read advertisement mode.
 * 
 * Output AT command:
 * > AT+UBTA?
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param[out] pAdvMode:   
 */
int32_t uCxBluetoothGetAdvertisements(uCxHandle_t * puCxHandle, uAdvMode_t * pAdvMode);

/**
 * Starts directed advertisements to Bluetooth Address. If bd_addr is FFFFFFFFFFFF, direct advertisements will be disabled.
 * By default the timeout is 1280 ms, and uses High Duty Cycle Advertising. A timeout greater than this will result in
 * Low Duty Cycle Advertising as High Duty Cycle Advertising has a limited use of only 1280 ms. Setting timeout to 0 will
 * let the
 * device advertise indefinitely in Low Duty Cycle mode.
 * 
 * Output AT command:
 * > AT+UBTDA=<bd_addr>
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      bd_addr:    Bluetooth device address of the remote device.
 */
int32_t uCxBluetoothDirectedAdvertisement1(uCxHandle_t * puCxHandle, uBtLeAddress_t * bd_addr);

/**
 * Starts directed advertisements to Bluetooth Address. If bd_addr is FFFFFFFFFFFF, direct advertisements will be disabled.
 * By default the timeout is 1280 ms, and uses High Duty Cycle Advertising. A timeout greater than this will result in
 * Low Duty Cycle Advertising as High Duty Cycle Advertising has a limited use of only 1280 ms. Setting timeout to 0 will
 * let the
 * device advertise indefinitely in Low Duty Cycle mode.
 * 
 * Output AT command:
 * > AT+UBTDA=<bd_addr>,<timeout>
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      bd_addr:    Bluetooth device address of the remote device.
 * @param      timeout:    Timeout for Directed Advertisements.
 */
int32_t uCxBluetoothDirectedAdvertisement2(uCxHandle_t * puCxHandle, uBtLeAddress_t * bd_addr, int32_t timeout);

/**
 * Write connection interval minimum.
 * 
 * Output AT command:
 * > AT+UBTCS0=<connection_interval_minimum>
 *
 * @param[in]  puCxHandle:                  uCX API handle
 * @param      connection_interval_minimum: Connection interval minimum (must be <= Connection interval maximum). Final results will
 *                                          be a result of negotiation between devices.
 *                                           Default: 24.
 *                                           Calculation: connection_interval_minimum * 1.25. ms
 */
int32_t uCxBluetoothSetConnectionIntervalMin(uCxHandle_t * puCxHandle, int32_t connection_interval_minimum);

/**
 * Read Connection Interval minium.
 * 
 * Output AT command:
 * > AT+UBTCS0?
 *
 * @param[in]  puCxHandle:                 uCX API handle
 * @param[out] pConnectionIntervalMinimum: Connection interval minimum (must be <= Connection interval maximum). Final results will
 *                                         be a result of negotiation between devices.
 *                                          Default: 24.
 *                                          Calculation: connection_interval_minimum * 1.25. ms
 */
int32_t uCxBluetoothGetConnectionIntervalMin(uCxHandle_t * puCxHandle, int32_t * pConnectionIntervalMinimum);

/**
 * Write connection interval maximum.
 * 
 * Output AT command:
 * > AT+UBTCS1=<connection_interval_maximum>
 *
 * @param[in]  puCxHandle:                  uCX API handle
 * @param      connection_interval_maximum: Connection interval maximum (must be >= Connection interval minimum). Final results will
 *                                          be a result of negotiation between devices.
 *                                           Default: 40.
 *                                           Calculation: connection_interval_maximum * 1.25 ms.
 */
int32_t uCxBluetoothSetConnectionIntervalMax(uCxHandle_t * puCxHandle, int32_t connection_interval_maximum);

/**
 * Read Connection Interval maximum.
 * 
 * Output AT command:
 * > AT+UBTCS1?
 *
 * @param[in]  puCxHandle:                 uCX API handle
 * @param[out] pConnectionIntervalMaximum: Connection interval maximum (must be >= Connection interval minimum). Final results will
 *                                         be a result of negotiation between devices.
 *                                          Default: 40.
 *                                          Calculation: connection_interval_maximum * 1.25 ms.
 */
int32_t uCxBluetoothGetConnectionIntervalMax(uCxHandle_t * puCxHandle, int32_t * pConnectionIntervalMaximum);

/**
 * Write connection peripheral latency.
 * 
 * Output AT command:
 * > AT+UBTCS2=<connection_peripheral_latency>
 *
 * @param[in]  puCxHandle:                    uCX API handle
 * @param      connection_peripheral_latency: Connection peripheral latency.
 *                                             Default: 0
 *                                             Calculation: Number of connection events.
 */
int32_t uCxBluetoothSetConnectionPeripheralLatency(uCxHandle_t * puCxHandle, int32_t connection_peripheral_latency);

/**
 * Read connection peripheral latency.
 * 
 * Output AT command:
 * > AT+UBTCS2?
 *
 * @param[in]  puCxHandle:                   uCX API handle
 * @param[out] pConnectionPeripheralLatency: Connection peripheral latency.
 *                                            Default: 0
 *                                            Calculation: Number of connection events.
 */
int32_t uCxBluetoothGetConnectionPeripheralLatency(uCxHandle_t * puCxHandle, int32_t * pConnectionPeripheralLatency);

/**
 * Write connection linkloss timeout.
 * 
 * Output AT command:
 * > AT+UBTCS3=<connection_linkloss_timeout>
 *
 * @param[in]  puCxHandle:                  uCX API handle
 * @param      connection_linkloss_timeout: Connection linkloss timeout.
 *                                           Default: 2000
 *                                           Calculation: connection_linkloss_timeout ms
 */
int32_t uCxBluetoothSetConnectionLinklossTimeout(uCxHandle_t * puCxHandle, int32_t connection_linkloss_timeout);

/**
 * Read connection linkloss timeout.
 * 
 * Output AT command:
 * > AT+UBTCS3?
 *
 * @param[in]  puCxHandle:                 uCX API handle
 * @param[out] pConnectionLinklossTimeout: Connection linkloss timeout.
 *                                          Default: 2000
 *                                          Calculation: connection_linkloss_timeout ms
 */
int32_t uCxBluetoothGetConnectionLinklossTimeout(uCxHandle_t * puCxHandle, int32_t * pConnectionLinklossTimeout);

/**
 * Write Preferred TX PHY.
 * 
 * Output AT command:
 * > AT+UBTCS4=<preferred_tx_phy>
 *
 * @param[in]  puCxHandle:       uCX API handle
 * @param      preferred_tx_phy: Preferred Transmitter PHY
 *                               0: Let other side decide
 *                               OR a bit field with three bits:
 *                               Bit 0: 1 Mbps preferred
 *                               Bit 1: 2 Mbps preferred
 *                               Bit 2: reserved for future use
 */
int32_t uCxBluetoothSetPreferredTxPhy(uCxHandle_t * puCxHandle, int32_t preferred_tx_phy);

/**
 * Read Preferred TX PHY.
 * 
 * Output AT command:
 * > AT+UBTCS4?
 *
 * @param[in]  puCxHandle:      uCX API handle
 * @param[out] pPreferredTxPhy: Preferred Transmitter PHY
 *                              0: Let other side decide
 *                              OR a bit field with three bits:
 *                              Bit 0: 1 Mbps preferred
 *                              Bit 1: 2 Mbps preferred
 *                              Bit 2: reserved for future use
 */
int32_t uCxBluetoothGetPreferredTxPhy(uCxHandle_t * puCxHandle, int32_t * pPreferredTxPhy);

/**
 * Write Preferred RX PHY.
 * 
 * Output AT command:
 * > AT+UBTCS5=<preferred_rx_phy>
 *
 * @param[in]  puCxHandle:       uCX API handle
 * @param      preferred_rx_phy: Preferred Receiver PHY
 *                               0: Let other side decide
 *                               OR a bit field with three bits:
 *                               Bit 0: 1 Mbps preferred
 *                               Bit 1: 2 Mbps preferred
 *                               Bit 2: reserved for future use
 */
int32_t uCxBluetoothSetPreferredRxPhy(uCxHandle_t * puCxHandle, int32_t preferred_rx_phy);

/**
 * Read Preferred RX PHY.
 * 
 * Output AT command:
 * > AT+UBTCS5?
 *
 * @param[in]  puCxHandle:      uCX API handle
 * @param[out] pPreferredRxPhy: Preferred Receiver PHY
 *                              0: Let other side decide
 *                              OR a bit field with three bits:
 *                              Bit 0: 1 Mbps preferred
 *                              Bit 1: 2 Mbps preferred
 *                              Bit 2: reserved for future use
 */
int32_t uCxBluetoothGetPreferredRxPhy(uCxHandle_t * puCxHandle, int32_t * pPreferredRxPhy);

/**
 * Write advertisement interval minimum.
 * 
 * Output AT command:
 * > AT+UBTAS0=<advertisement_interval_minimum>
 *
 * @param[in]  puCxHandle:                     uCX API handle
 * @param      advertisement_interval_minimum: Advertising interval minimum (must be <= Advertising interval maximum. 
 *                                              Default: 1600.
 *                                              Calculation: advertisement_interval_minimum * 0.625 ms)
 */
int32_t uCxBluetoothSetAdvIntervalMin(uCxHandle_t * puCxHandle, int32_t advertisement_interval_minimum);

/**
 * Read advertisement Interval miniumum.
 * 
 * Output AT command:
 * > AT+UBTAS0?
 *
 * @param[in]  puCxHandle:                    uCX API handle
 * @param[out] pAdvertisementIntervalMinimum: Advertising interval minimum (must be <= Advertising interval maximum. 
 *                                             Default: 1600.
 *                                             Calculation: advertisement_interval_minimum * 0.625 ms)
 */
int32_t uCxBluetoothGetAdvIntervalMin(uCxHandle_t * puCxHandle, int32_t * pAdvertisementIntervalMinimum);

/**
 * Write advertisement interval maximum.
 * 
 * Output AT command:
 * > AT+UBTAS1=<advertisement_interval_maximum>
 *
 * @param[in]  puCxHandle:                     uCX API handle
 * @param      advertisement_interval_maximum: Advertising interval maximum (must be >= Advertising interval minimum. 
 *                                              Default: 2000.
 *                                              Calculation: advertisement_interval_maximum * 0.625 ms)
 */
int32_t uCxBluetoothSetAdvIntervalMax(uCxHandle_t * puCxHandle, int32_t advertisement_interval_maximum);

/**
 * Read advertisement Interval maximum.
 * 
 * Output AT command:
 * > AT+UBTAS1?
 *
 * @param[in]  puCxHandle:                    uCX API handle
 * @param[out] pAdvertisementIntervalMaximum: Advertising interval maximum (must be >= Advertising interval minimum. 
 *                                             Default: 2000.
 *                                             Calculation: advertisement_interval_maximum * 0.625 ms)
 */
int32_t uCxBluetoothGetAdvIntervalMax(uCxHandle_t * puCxHandle, int32_t * pAdvertisementIntervalMaximum);

/**
 * Set I/O Capabilities
 * 
 * Output AT command:
 * > AT+UBTIOC=<io_capabilities>
 *
 * @param[in]  puCxHandle:      uCX API handle
 * @param      io_capabilities: 
 */
int32_t uCxBluetoothSetIoCapabilities(uCxHandle_t * puCxHandle, uIoCapabilities_t io_capabilities);

/**
 * Read I/O Capabilities
 * 
 * Output AT command:
 * > AT+UBTIOC?
 *
 * @param[in]  puCxHandle:      uCX API handle
 * @param[out] pIoCapabilities: 
 */
int32_t uCxBluetoothGetIoCapabilities(uCxHandle_t * puCxHandle, uIoCapabilities_t * pIoCapabilities);

/**
 * Writes the security mode
 * 
 * Output AT command:
 * > AT+UBTBSM=<bt_security_mode>
 *
 * @param[in]  puCxHandle:       uCX API handle
 * @param      bt_security_mode: 
 */
int32_t uCxBluetoothSetSecurityMode(uCxHandle_t * puCxHandle, uBtSecurityMode_t bt_security_mode);

/**
 * Reads the security mode
 * 
 * Output AT command:
 * > AT+UBTBSM?
 *
 * @param[in]  puCxHandle:      uCX API handle
 * @param[out] pBtSecurityMode: 
 */
int32_t uCxBluetoothGetSecurityMode(uCxHandle_t * puCxHandle, uBtSecurityMode_t * pBtSecurityMode);

/**
 * Writes the pairing mode.
 * 
 * Output AT command:
 * > AT+UBTPM=<pairing_mode>
 *
 * @param[in]  puCxHandle:   uCX API handle
 * @param      pairing_mode: 
 */
int32_t uCxBluetoothSetPairingMode(uCxHandle_t * puCxHandle, uPairingMode_t pairing_mode);

/**
 * Read current pairing mode.
 * 
 * Output AT command:
 * > AT+UBTPM?
 *
 * @param[in]  puCxHandle:   uCX API handle
 * @param[out] pPairingMode: 
 */
int32_t uCxBluetoothGetPairingMode(uCxHandle_t * puCxHandle, uPairingMode_t * pPairingMode);

/**
 * Respond to +UEUBTUC and confirm/deny bonding.
 * 
 * Output AT command:
 * > AT+UBTUC=<bd_addr>,<yes_no>
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      bd_addr:    Bluetooth device address of the remote device.
 * @param      yes_no:     
 */
int32_t uCxBluetoothUserConfirmation(uCxHandle_t * puCxHandle, uBtLeAddress_t * bd_addr, uYesNo_t yes_no);

/**
 * Respond to +UEBTUPE event and confirm/deny bonding.
 * 
 * Output AT command:
 * > AT+UBTUPE=<bd_addr>,<yes_no>
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      bd_addr:    Bluetooth device address of the remote device.
 * @param      yes_no:     
 */
int32_t uCxBluetoothUserPasskeyEntry2(uCxHandle_t * puCxHandle, uBtLeAddress_t * bd_addr, uYesNo_t yes_no);

/**
 * Respond to +UEBTUPE event and confirm/deny bonding.
 * 
 * Output AT command:
 * > AT+UBTUPE=<bd_addr>,<yes_no>,<passkey>
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      bd_addr:    Bluetooth device address of the remote device.
 * @param      yes_no:     
 * @param      passkey:    Passkey used to confirm bonding, if yes_no is set to no, this can be omitted.
 */
int32_t uCxBluetoothUserPasskeyEntry3(uCxHandle_t * puCxHandle, uBtLeAddress_t * bd_addr, uYesNo_t yes_no, int32_t passkey);

/**
 * Initiate bonding. To perform the bonding, the remote device must be in a pairable and connectable mode. Bond Event
 * +UEBTB is genereated once the bond is complete.
 * 
 * Output AT command:
 * > AT+UBTB=<bd_addr>
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      bd_addr:    Bluetooth device address of the remote device.
 */
int32_t uCxBluetoothBond(uCxHandle_t * puCxHandle, uBtLeAddress_t * bd_addr);

/**
 * Removes a previously bonded device. The address FFFFFFFFFFFF will remove all the bonded devices.
 * 
 * Output AT command:
 * > AT+UBTUB=<bd_addr>
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      bd_addr:    Bluetooth device address of the remote device.
 */
int32_t uCxBluetoothUnbond(uCxHandle_t * puCxHandle, uBtLeAddress_t * bd_addr);

/**
 * Read list of bonded devices.
 * 
 * Output AT command:
 * > AT+UBTBDL
 *
 * @param[in]  puCxHandle: uCX API handle
 */
void uCxBeginBluetoothListBondedDevices(uCxHandle_t * puCxHandle);

/**
 * 
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param[out] pBdAddr:    Bluetooth device address of the remote device.
 */
bool uCxBluetoothListBondedDevicesGetResponse(uCxHandle_t * puCxHandle, uBtLeAddress_t * pBdAddr);

/**
 * Set a characterstic value.
 * 
 * Output AT command:
 * > AT+UBTDIS=<characteristic_id>
 *
 * @param[in]  puCxHandle:                            uCX API handle
 * @param      characteristic_id:                     
 * @param[out] pBluetoothSetDeviceInfoServiceCharRsp: Please see \ref uCxBluetoothSetDeviceInfoServiceChar_t
 */
bool uCxBeginBluetoothSetDeviceInfoServiceChar1(uCxHandle_t * puCxHandle, uCharacteristicId_t characteristic_id, uCxBluetoothSetDeviceInfoServiceChar_t * pBluetoothSetDeviceInfoServiceCharRsp);

/**
 * Set a characterstic value.
 * 
 * Output AT command:
 * > AT+UBTDIS=<characteristic_id>,<characteristic_value>
 *
 * @param[in]  puCxHandle:                            uCX API handle
 * @param      characteristic_id:                     
 * @param      characteristic_value:                  Value of Device Information Service characterstic.
 * @param[out] pBluetoothSetDeviceInfoServiceCharRsp: Please see \ref uCxBluetoothSetDeviceInfoServiceChar_t
 */
bool uCxBeginBluetoothSetDeviceInfoServiceChar2(uCxHandle_t * puCxHandle, uCharacteristicId_t characteristic_id, const char * characteristic_value, uCxBluetoothSetDeviceInfoServiceChar_t * pBluetoothSetDeviceInfoServiceCharRsp);

/**
 * Read all individual characterstic of the Device Information Service characteristics.
 * 
 * Output AT command:
 * > AT+UBTDIS?
 *
 * @param[in]  puCxHandle: uCX API handle
 */
void uCxBeginBluetoothListDeviceInfoServiceChars(uCxHandle_t * puCxHandle);

/**
 * 
 *
 * @param[in]  puCxHandle:                              uCX API handle
 * @param[out] pBluetoothListDeviceInfoServiceCharsRsp: Please see \ref uCxBluetoothListDeviceInfoServiceChars_t
 */
bool uCxBluetoothListDeviceInfoServiceCharsGetResponse(uCxHandle_t * puCxHandle, uCxBluetoothListDeviceInfoServiceChars_t * pBluetoothListDeviceInfoServiceCharsRsp);

/**
 * Requests a Bluetooth Low Energy PHY update.
 * 
 * Output AT command:
 * > AT+UBTPHYR=<conn_handle>,<tx_phy>,<rx_phy>
 *
 * @param[in]  puCxHandle:  uCX API handle
 * @param      conn_handle: Connection handle of the Bluetooth low energy connection.
 * @param      tx_phy:      Requested PHY for Transmitter:
 *                          0: Let other side decide
 *                          OR a bit field with three bits:
 *                          Bit 0: 1 Mbps preferred
 *                          Bit 1: 2 Mbps preferred
 *                          Bit 2: Coded PHY (S=8). Not supported by NORA-W36
 * @param      rx_phy:      Requested PHY for Receiver
 *                          0: Let other side decide
 *                          OR a bit field with three bits:
 *                          Bit 0: 1 Mbps preferred
 *                          Bit 1: 2 Mbps preferred
 *                          Bit 2: Coded PHY (S=8). Not supported by NORA-W36
 */
int32_t uCxBluetoothRequestPhy(uCxHandle_t * puCxHandle, int32_t conn_handle, int32_t tx_phy, int32_t rx_phy);

/**
 * Reads currect PHYs for a connection.
 * 
 * Output AT command:
 * > AT+UBTPHYR=<conn_handle>
 *
 * @param[in]  puCxHandle:          uCX API handle
 * @param      conn_handle:         Connection handle of the Bluetooth low energy connection.
 * @param[out] pBluetoothGetPhyRsp: Please see \ref uCxBluetoothGetPhy_t
 */
int32_t uCxBluetoothGetPhy(uCxHandle_t * puCxHandle, int32_t conn_handle, uCxBluetoothGetPhy_t * pBluetoothGetPhyRsp);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _U_CX_BLUETOOTH_H_ */