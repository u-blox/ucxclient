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
    int32_t data_type;        /**< Type of advertising data received. */
    uByteArray_t data;        /**< Complete advertise/scan response data received from the remote device. */
} uCxBluetoothDiscoveryDefault_t;

typedef struct
{
    uBtLeAddress_t bd_addr;   /**< Bluetooth device address of the remote device. */
    int32_t rssi;             /**< Received signal strength in dBm. */
    const char * device_name; /**< Name of the discovered device. */
    int32_t data_type;        /**< Type of advertising data received. */
    uByteArray_t data;        /**< Complete advertise/scan response data received from the remote device. */
} uCxBluetoothDiscovery_t;

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
    int32_t legacy_advertisement;
    int32_t directed_advertisement;
    uIntList_t enabled_extended_advertisements;
} uCxBluetoothGetAdvertiseInformation_t;

typedef struct
{
    int32_t advertisement_interval_minimum; /**< Advertising interval minimum (must be <= Advertising interval maximum. 
                                                  Default: 1600.
                                                  Calculation: advertisement_interval_minimum * 0.625 ms) */
    int32_t advertisement_interval_maximum; /**< Advertising interval maximum (must be >= Advertising interval minimum. 
                                                  Default: 2000.
                                                  Calculation: advertisement_interval_maximum * 0.625 ms) */
} uCxBluetoothGetAdvertismentLegacyConfiguration_t;

typedef struct
{
    int32_t characteristic_id;
    const char * characteristic_value; /**< Value of Device Information Service characteristic. */
} uCxBluetoothListDeviceInfoServiceChars_t;

typedef struct
{
    int32_t tx_phy; /**< Requested PHY for Transmitter:
                         0: Let other side decide
                         OR a bit field with bits:
                         Bit 0: 1 Mbps preferred
                         Bit 1: 2 Mbps preferred */
    int32_t rx_phy; /**< Requested PHY for Receiver
                         0: Let other side decide
                         OR a bit field with bits:
                         Bit 0: 1 Mbps preferred
                         Bit 1: 2 Mbps preferred */
} uCxBluetoothGetPhy_t;


/* ------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * ---------------------------------------------------------- */

/**
 * Set Bluetooth Mode.
 * 
 * Notes:
 * Requires AT&W and a reboot before taking effect.
 * 
 * Output AT command:
 * > AT+UBTM=<bt_mode>
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      bt_mode:    
 * @return                 0 on success, negative value on error.
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
 * @return                 0 on success, negative value on error.
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
 * @return                 0 on success, negative value on error.
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
 * @return                  0 on success, negative value on error.
 */
int32_t uCxBluetoothDisconnect(uCxHandle_t * puCxHandle, int32_t conn_handle);

/**
 * Reads the local Bluetooth device name.
 * 
 * Output AT command:
 * > AT+UBTLN?
 *
 * @param[in]  puCxHandle:   uCX API handle
 * @param[out] ppDeviceName: Default local name is the MODEL-xxxxxx where xxxxxx are the last 3 bytes of the device MAC
 *                           address in hexadecimal format. If the local name is set to "" it is cleared but will
 *                           revert to the default name after a restart.
 * @return                   true on success, false on error (error code will be returned by uCxEnd()).
 *
 * NOTES:
 * Must be terminated by calling uCxEnd()
 */
bool uCxBluetoothGetLocalNameBegin(uCxHandle_t * puCxHandle, const char ** ppDeviceName);

/**
 * Writes the local Bluetooth device name.
 * 
 * Notes:
 * Can be stored using AT&W.
 * 
 * Output AT command:
 * > AT+UBTLN=<device_name>
 *
 * @param[in]  puCxHandle:  uCX API handle
 * @param      device_name: Default local name is the MODEL-xxxxxx where xxxxxx are the last 3 bytes of the device MAC
 *                          address in hexadecimal format. If the local name is set to "" it is cleared but will
 *                          revert to the default name after a restart.
 * @return                  0 on success, negative value on error.
 */
int32_t uCxBluetoothSetLocalName(uCxHandle_t * puCxHandle, const char * device_name);

/**
 * Start discovery using default parameters
 * 
 * Output AT command:
 * > AT+UBTD
 *
 * @param[in]  puCxHandle: uCX API handle
 * @return                 true on success, false on error (error code will be returned by uCxEnd()).
 *
 * NOTES:
 * Must be terminated by calling uCxEnd()
 */
void uCxBluetoothDiscoveryDefaultBegin(uCxHandle_t * puCxHandle);

/**
 * 
 *
 * @param[in]  puCxHandle:                    uCX API handle
 * @param[out] pBluetoothDiscoveryDefaultRsp: Please see \ref uCxBluetoothDiscoveryDefault_t
 * @return                                    true on success, false when there are no more entries or on error (uCxEnd() will return
 *                                            error code in this case).
 */
bool uCxBluetoothDiscoveryDefaultGetNext(uCxHandle_t * puCxHandle, uCxBluetoothDiscoveryDefault_t * pBluetoothDiscoveryDefaultRsp);

/**
 * Start discovery.
 * 
 * Output AT command:
 * > AT+UBTD=<discovery_type>
 *
 * @param[in]  puCxHandle:     uCX API handle
 * @param      discovery_type: 
 * @return                     true on success, false on error (error code will be returned by uCxEnd()).
 *
 * NOTES:
 * Must be terminated by calling uCxEnd()
 */
void uCxBluetoothDiscovery1Begin(uCxHandle_t * puCxHandle, uDiscoveryType_t discovery_type);

/**
 * 
 *
 * @param[in]  puCxHandle:             uCX API handle
 * @param[out] pBluetoothDiscoveryRsp: Please see \ref uCxBluetoothDiscovery_t
 * @return                             true on success, false when there are no more entries or on error (uCxEnd() will return
 *                                     error code in this case).
 */
bool uCxBluetoothDiscovery1GetNext(uCxHandle_t * puCxHandle, uCxBluetoothDiscovery_t * pBluetoothDiscoveryRsp);

/**
 * Start discovery.
 * 
 * Output AT command:
 * > AT+UBTD=<discovery_type>,<discovery_mode>
 *
 * @param[in]  puCxHandle:     uCX API handle
 * @param      discovery_type: 
 * @param      discovery_mode: 
 * @return                     true on success, false on error (error code will be returned by uCxEnd()).
 *
 * NOTES:
 * Must be terminated by calling uCxEnd()
 */
void uCxBluetoothDiscovery2Begin(uCxHandle_t * puCxHandle, uDiscoveryType_t discovery_type, uDiscoveryMode_t discovery_mode);

/**
 * 
 *
 * @param[in]  puCxHandle:             uCX API handle
 * @param[out] pBluetoothDiscoveryRsp: Please see \ref uCxBluetoothDiscovery_t
 * @return                             true on success, false when there are no more entries or on error (uCxEnd() will return
 *                                     error code in this case).
 */
bool uCxBluetoothDiscovery2GetNext(uCxHandle_t * puCxHandle, uCxBluetoothDiscovery_t * pBluetoothDiscoveryRsp);

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
 * @return                       true on success, false on error (error code will be returned by uCxEnd()).
 *
 * NOTES:
 * Must be terminated by calling uCxEnd()
 */
void uCxBluetoothDiscovery3Begin(uCxHandle_t * puCxHandle, uDiscoveryType_t discovery_type, uDiscoveryMode_t discovery_mode, int32_t discovery_length);

/**
 * 
 *
 * @param[in]  puCxHandle:             uCX API handle
 * @param[out] pBluetoothDiscoveryRsp: Please see \ref uCxBluetoothDiscovery_t
 * @return                             true on success, false when there are no more entries or on error (uCxEnd() will return
 *                                     error code in this case).
 */
bool uCxBluetoothDiscovery3GetNext(uCxHandle_t * puCxHandle, uCxBluetoothDiscovery_t * pBluetoothDiscoveryRsp);

/**
 * Start background discovery
 * 
 * Output AT command:
 * > AT+UBTBGD
 *
 * @param[in]  puCxHandle: uCX API handle
 * @return                 0 on success, negative value on error.
 */
int32_t uCxBluetoothSetBgDiscoveryDefault(uCxHandle_t * puCxHandle);

/**
 * Start background discovery
 * 
 * Output AT command:
 * > AT+UBTBGD=<discovery_type>
 *
 * @param[in]  puCxHandle:     uCX API handle
 * @param      discovery_type: 
 * @return                     0 on success, negative value on error.
 */
int32_t uCxBluetoothSetBgDiscovery1(uCxHandle_t * puCxHandle, uDiscoveryType_t discovery_type);

/**
 * Start background discovery
 * 
 * Output AT command:
 * > AT+UBTBGD=<discovery_type>,<discovery_mode>
 *
 * @param[in]  puCxHandle:     uCX API handle
 * @param      discovery_type: 
 * @param      discovery_mode: 
 * @return                     0 on success, negative value on error.
 */
int32_t uCxBluetoothSetBgDiscovery2(uCxHandle_t * puCxHandle, uDiscoveryType_t discovery_type, uDiscoveryMode_t discovery_mode);

/**
 * Start background discovery
 * 
 * Output AT command:
 * > AT+UBTBGD=<discovery_type>,<discovery_mode>,<output_events>
 *
 * @param[in]  puCxHandle:     uCX API handle
 * @param      discovery_type: 
 * @param      discovery_mode: 
 * @param      output_events:  
 * @return                     0 on success, negative value on error.
 */
int32_t uCxBluetoothSetBgDiscovery3(uCxHandle_t * puCxHandle, uDiscoveryType_t discovery_type, uDiscoveryMode_t discovery_mode, uOutputEvents_t output_events);

/**
 * Stop background discovery
 * 
 * Output AT command:
 * > AT+UBTBGDS
 *
 * @param[in]  puCxHandle: uCX API handle
 * @return                 0 on success, negative value on error.
 */
int32_t uCxBluetoothStopBgDiscovery(uCxHandle_t * puCxHandle);

/**
 * Returns the current RSSI for a specified Bluetooth connection.
 * 
 * Output AT command:
 * > AT+UBTRSS=<conn_handle>
 *
 * @param[in]  puCxHandle:  uCX API handle
 * @param      conn_handle: Connection handle of the Bluetooth low energy connection.
 * @param[out] pRssi:       Received signal strength in dBm.
 * @return                  0 on success, negative value on error.
 */
int32_t uCxBluetoothRssi(uCxHandle_t * puCxHandle, int32_t conn_handle, int32_t * pRssi);

/**
 * List all Bluetooth low energy ACL connections.
 * 
 * Output AT command:
 * > AT+UBTCL
 *
 * @param[in]  puCxHandle: uCX API handle
 * @return                 true on success, false on error (error code will be returned by uCxEnd()).
 *
 * NOTES:
 * Must be terminated by calling uCxEnd()
 */
void uCxBluetoothListConnectionsBegin(uCxHandle_t * puCxHandle);

/**
 * 
 *
 * @param[in]  puCxHandle:                   uCX API handle
 * @param[out] pBluetoothListConnectionsRsp: Please see \ref uCxBluetoothListConnections_t
 * @return                                   true on success, false when there are no more entries or on error (uCxEnd() will return
 *                                           error code in this case).
 */
bool uCxBluetoothListConnectionsGetNext(uCxHandle_t * puCxHandle, uCxBluetoothListConnections_t * pBluetoothListConnectionsRsp);

/**
 * Read all properties of an existing Bluetooth low energy ACL connection.
 * 
 * Output AT command:
 * > AT+UBTCST=<conn_handle>
 *
 * @param[in]  puCxHandle:  uCX API handle
 * @param      conn_handle: Connection handle of the Bluetooth low energy connection.
 * @return                  true on success, false on error (error code will be returned by uCxEnd()).
 *
 * NOTES:
 * Must be terminated by calling uCxEnd()
 */
void uCxBluetoothListConnectionStatusBegin(uCxHandle_t * puCxHandle, int32_t conn_handle);

/**
 * 
 *
 * @param[in]  puCxHandle:                        uCX API handle
 * @param[out] pBluetoothListConnectionStatusRsp: Please see \ref uCxBluetoothListConnectionStatus_t
 * @return                                        true on success, false when there are no more entries or on error (uCxEnd() will return
 *                                                error code in this case).
 */
bool uCxBluetoothListConnectionStatusGetNext(uCxHandle_t * puCxHandle, uCxBluetoothListConnectionStatus_t * pBluetoothListConnectionStatusRsp);

/**
 * Read a specific property of an existing Bluetooth low energy ACL connection.
 * 
 * Output AT command:
 * > AT+UBTCST=<conn_handle>,<property_id>
 *
 * @param[in]  puCxHandle:  uCX API handle
 * @param      conn_handle: Connection handle of the Bluetooth low energy connection.
 * @param      property_id: 
 * @param[out] pStatusVal:  Value of the preceding property.
 * @return                  0 on success, negative value on error.
 */
int32_t uCxBluetoothGetConnectionStatus(uCxHandle_t * puCxHandle, int32_t conn_handle, uPropertyId_t property_id, int32_t * pStatusVal);

/**
 * Read the current advertisements
 * 
 * Output AT command:
 * > AT+UBTA?
 *
 * @param[in]  puCxHandle:                           uCX API handle
 * @param[out] pBluetoothGetAdvertiseInformationRsp: Please see \ref uCxBluetoothGetAdvertiseInformation_t
 * @return                                           0 on success, negative value on error.
 */
int32_t uCxBluetoothGetAdvertiseInformation(uCxHandle_t * puCxHandle, uCxBluetoothGetAdvertiseInformation_t * pBluetoothGetAdvertiseInformationRsp);

/**
 * Write custom advertising data.
 * 
 * Notes:
 * Can be stored using AT&W.
 * 
 * Output AT command:
 * > AT+UBTADL=<adv_data>,<adv_data_len>
 *
 * @param[in]  puCxHandle:   uCX API handle
 * @param      adv_data:     
 * @param      adv_data_len: length of adv_data
 * @return                   0 on success, negative value on error.
 */
int32_t uCxBluetoothSetAdvertiseData(uCxHandle_t * puCxHandle, const uint8_t * adv_data, int32_t adv_data_len);

/**
 * Read custom advertising data.
 * 
 * Output AT command:
 * > AT+UBTADL?
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param[out] pAdvData:   
 * @return                 true on success, false on error (error code will be returned by uCxEnd()).
 *
 * NOTES:
 * Must be terminated by calling uCxEnd()
 */
bool uCxBluetoothGetAdvertiseDataBegin(uCxHandle_t * puCxHandle, uByteArray_t * pAdvData);

/**
 * Clear the custom legacy advertise data.
 * 
 * Output AT command:
 * > AT+UBTADLC
 *
 * @param[in]  puCxHandle: uCX API handle
 * @return                 0 on success, negative value on error.
 */
int32_t uCxBluetoothClearAdvertiseDataLegacy(uCxHandle_t * puCxHandle);

/**
 * Write custom scan response data.
 * 
 * Notes:
 * Can be stored using AT&W.
 * 
 * Output AT command:
 * > AT+UBTASD=<scan_rsp_data>,<scan_rsp_data_len>
 *
 * @param[in]  puCxHandle:        uCX API handle
 * @param      scan_rsp_data:     
 * @param      scan_rsp_data_len: length of scan_rsp_data
 * @return                        0 on success, negative value on error.
 */
int32_t uCxBluetoothSetScanResponseData(uCxHandle_t * puCxHandle, const uint8_t * scan_rsp_data, int32_t scan_rsp_data_len);

/**
 * Read custom scan response data.
 * 
 * Output AT command:
 * > AT+UBTASD?
 *
 * @param[in]  puCxHandle:   uCX API handle
 * @param[out] pScanRspData: 
 * @return                   true on success, false on error (error code will be returned by uCxEnd()).
 *
 * NOTES:
 * Must be terminated by calling uCxEnd()
 */
bool uCxBluetoothGetScanResponseDataBegin(uCxHandle_t * puCxHandle, uByteArray_t * pScanRspData);

/**
 * Clear the custom scan response data.
 * 
 * Output AT command:
 * > AT+UBTASDC
 *
 * @param[in]  puCxHandle: uCX API handle
 * @return                 0 on success, negative value on error.
 */
int32_t uCxBluetoothClearScanData(uCxHandle_t * puCxHandle);

/**
 * Start legacy advertisements if not started.
 * 
 * Notes:
 * Can be stored using AT&W.
 * 
 * Output AT command:
 * > AT+UBTAL
 *
 * @param[in]  puCxHandle: uCX API handle
 * @return                 0 on success, negative value on error.
 */
int32_t uCxBluetoothSetLegacyAdvertisements(uCxHandle_t * puCxHandle);

/**
 * Stop legacy advertisements if started.
 * 
 * Notes:
 * Can be stored using AT&W.
 * 
 * Output AT command:
 * > AT+UBTALD
 *
 * @param[in]  puCxHandle: uCX API handle
 * @return                 0 on success, negative value on error.
 */
int32_t uCxBluetoothLegacyAdvertisementStop(uCxHandle_t * puCxHandle);

/**
 * Starts directed advertisements to Bluetooth Address.
 * By default the timeout is 1280 ms, and uses High Duty Cycle Advertising. A timeout greater than this will result in
 * Low Duty Cycle Advertising as High Duty Cycle Advertising has a limited use of only 1280 ms. Setting timeout to 0 will
 * let the
 * device advertise indefinitely in Low Duty Cycle mode.
 * 
 * Output AT command:
 * > AT+UBTAD=<bd_addr>
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      bd_addr:    Bluetooth device address of the remote device.
 * @return                 0 on success, negative value on error.
 */
int32_t uCxBluetoothDirectedAdvertisementStart1(uCxHandle_t * puCxHandle, uBtLeAddress_t * bd_addr);

/**
 * Starts directed advertisements to Bluetooth Address.
 * By default the timeout is 1280 ms, and uses High Duty Cycle Advertising. A timeout greater than this will result in
 * Low Duty Cycle Advertising as High Duty Cycle Advertising has a limited use of only 1280 ms. Setting timeout to 0 will
 * let the
 * device advertise indefinitely in Low Duty Cycle mode.
 * 
 * Output AT command:
 * > AT+UBTAD=<bd_addr>,<timeout>
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      bd_addr:    Bluetooth device address of the remote device.
 * @param      timeout:    Timeout for Directed Advertisements.
 * @return                 0 on success, negative value on error.
 */
int32_t uCxBluetoothDirectedAdvertisementStart2(uCxHandle_t * puCxHandle, uBtLeAddress_t * bd_addr, int32_t timeout);

/**
 * Stop directed advertisements.
 * 
 * Output AT command:
 * > AT+UBTADD
 *
 * @param[in]  puCxHandle: uCX API handle
 * @return                 0 on success, negative value on error.
 */
int32_t uCxBluetoothDirectedAdvertisementStop(uCxHandle_t * puCxHandle);

/**
 * Write connection interval minimum.
 * 
 * Notes:
 * Can be stored using AT&W.
 * 
 * Output AT command:
 * > AT+UBTCS0=<connection_interval_minimum>
 *
 * @param[in]  puCxHandle:                  uCX API handle
 * @param      connection_interval_minimum: Connection interval minimum (must be <= Connection interval maximum). Final results will
 *                                          be a result of negotiation between devices.
 *                                           Default: 24.
 *                                           Calculation: connection_interval_minimum * 1.25. ms
 * @return                                  0 on success, negative value on error.
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
 * @return                                 0 on success, negative value on error.
 */
int32_t uCxBluetoothGetConnectionIntervalMin(uCxHandle_t * puCxHandle, int32_t * pConnectionIntervalMinimum);

/**
 * Write connection interval maximum.
 * 
 * Notes:
 * Can be stored using AT&W.
 * 
 * Output AT command:
 * > AT+UBTCS1=<connection_interval_maximum>
 *
 * @param[in]  puCxHandle:                  uCX API handle
 * @param      connection_interval_maximum: Connection interval maximum (must be >= Connection interval minimum). Final results will
 *                                          be a result of negotiation between devices.
 *                                           Default: 40.
 *                                           Calculation: connection_interval_maximum * 1.25 ms.
 * @return                                  0 on success, negative value on error.
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
 * @return                                 0 on success, negative value on error.
 */
int32_t uCxBluetoothGetConnectionIntervalMax(uCxHandle_t * puCxHandle, int32_t * pConnectionIntervalMaximum);

/**
 * Write connection peripheral latency.
 * 
 * Notes:
 * Can be stored using AT&W.
 * 
 * Output AT command:
 * > AT+UBTCS2=<connection_peripheral_latency>
 *
 * @param[in]  puCxHandle:                    uCX API handle
 * @param      connection_peripheral_latency: Connection peripheral latency.
 *                                             Default: 0
 *                                             Calculation: Number of connection events.
 * @return                                    0 on success, negative value on error.
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
 * @return                                   0 on success, negative value on error.
 */
int32_t uCxBluetoothGetConnectionPeripheralLatency(uCxHandle_t * puCxHandle, int32_t * pConnectionPeripheralLatency);

/**
 * Write connection linkloss timeout.
 * 
 * Notes:
 * Can be stored using AT&W.
 * 
 * Output AT command:
 * > AT+UBTCS3=<connection_linkloss_timeout>
 *
 * @param[in]  puCxHandle:                  uCX API handle
 * @param      connection_linkloss_timeout: Connection linkloss timeout.
 *                                           Default: 2000
 *                                           Calculation: connection_linkloss_timeout ms
 * @return                                  0 on success, negative value on error.
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
 * @return                                 0 on success, negative value on error.
 */
int32_t uCxBluetoothGetConnectionLinklossTimeout(uCxHandle_t * puCxHandle, int32_t * pConnectionLinklossTimeout);

/**
 * Write Preferred TX PHY.
 * 
 * Notes:
 * Can be stored using AT&W.
 * 
 * Output AT command:
 * > AT+UBTCS4=<preferred_tx_phy>
 *
 * @param[in]  puCxHandle:       uCX API handle
 * @param      preferred_tx_phy: Preferred Transmitter PHY
 *                               0: Let other side decide
 *                               OR a bit field with bits:
 *                               Bit 0: 1 Mbps preferred
 *                               Bit 1: 2 Mbps preferred
 * @return                       0 on success, negative value on error.
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
 *                              OR a bit field with bits:
 *                              Bit 0: 1 Mbps preferred
 *                              Bit 1: 2 Mbps preferred
 * @return                      0 on success, negative value on error.
 */
int32_t uCxBluetoothGetPreferredTxPhy(uCxHandle_t * puCxHandle, int32_t * pPreferredTxPhy);

/**
 * Write Preferred RX PHY.
 * 
 * Notes:
 * Can be stored using AT&W.
 * 
 * Output AT command:
 * > AT+UBTCS5=<preferred_rx_phy>
 *
 * @param[in]  puCxHandle:       uCX API handle
 * @param      preferred_rx_phy: Preferred PHY for Receiver
 *                               0: Let other side decide
 *                               OR a bit field with bits:
 *                               Bit 0: 1 Mbps preferred
 *                               Bit 1: 2 Mbps preferred
 * @return                       0 on success, negative value on error.
 */
int32_t uCxBluetoothSetPreferredRxPhy(uCxHandle_t * puCxHandle, int32_t preferred_rx_phy);

/**
 * Read Preferred RX PHY.
 * 
 * Output AT command:
 * > AT+UBTCS5?
 *
 * @param[in]  puCxHandle:      uCX API handle
 * @param[out] pPreferredRxPhy: Preferred PHY for Receiver
 *                              0: Let other side decide
 *                              OR a bit field with bits:
 *                              Bit 0: 1 Mbps preferred
 *                              Bit 1: 2 Mbps preferred
 * @return                      0 on success, negative value on error.
 */
int32_t uCxBluetoothGetPreferredRxPhy(uCxHandle_t * puCxHandle, int32_t * pPreferredRxPhy);

/**
 * Configure advertisement parameters for legacy advertisements
 * 
 * Output AT command:
 * > AT+UBTALS=<advertisement_interval_minimum>,<advertisement_interval_maximum>
 *
 * @param[in]  puCxHandle:                     uCX API handle
 * @param      advertisement_interval_minimum: Advertising interval minimum (must be <= Advertising interval maximum. 
 *                                              Default: 1600.
 *                                              Calculation: advertisement_interval_minimum * 0.625 ms)
 * @param      advertisement_interval_maximum: Advertising interval maximum (must be >= Advertising interval minimum. 
 *                                              Default: 2000.
 *                                              Calculation: advertisement_interval_maximum * 0.625 ms)
 * @return                                     0 on success, negative value on error.
 */
int32_t uCxBluetoothSetAdvertismentLegacyConfiguration(uCxHandle_t * puCxHandle, int32_t advertisement_interval_minimum, int32_t advertisement_interval_maximum);

/**
 * Read advertisement parameters for legacy advertisements
 * 
 * Output AT command:
 * > AT+UBTALS?
 *
 * @param[in]  puCxHandle:                                      uCX API handle
 * @param[out] pBluetoothGetAdvertismentLegacyConfigurationRsp: Please see \ref uCxBluetoothGetAdvertismentLegacyConfiguration_t
 * @return                                                      0 on success, negative value on error.
 */
int32_t uCxBluetoothGetAdvertismentLegacyConfiguration(uCxHandle_t * puCxHandle, uCxBluetoothGetAdvertismentLegacyConfiguration_t * pBluetoothGetAdvertismentLegacyConfigurationRsp);

/**
 * Write scan interval.
 * 
 * Output AT command:
 * > AT+UBTSS0=<scan_interval>
 *
 * @param[in]  puCxHandle:    uCX API handle
 * @param      scan_interval: Scan interval (must be >= Scan window. 
 *                             Default: 160.
 *                             Calculation: scan_interval * 0.625 ms)
 * @return                    0 on success, negative value on error.
 */
int32_t uCxBluetoothSetScanInterval(uCxHandle_t * puCxHandle, int32_t scan_interval);

/**
 * Read scan Interval.
 * 
 * Output AT command:
 * > AT+UBTSS0?
 *
 * @param[in]  puCxHandle:    uCX API handle
 * @param[out] pScanInterval: Scan interval (must be >= Scan window. 
 *                             Default: 160.
 *                             Calculation: scan_interval * 0.625 ms)
 * @return                    0 on success, negative value on error.
 */
int32_t uCxBluetoothGetScanInterval(uCxHandle_t * puCxHandle, int32_t * pScanInterval);

/**
 * Write scan window.
 * 
 * Output AT command:
 * > AT+UBTSS1=<scan_window>
 *
 * @param[in]  puCxHandle:  uCX API handle
 * @param      scan_window: Scan window (must be <= Scan interval. 
 *                           Default: 128.
 *                           Calculation: scan_interval * 0.625 ms)
 * @return                  0 on success, negative value on error.
 */
int32_t uCxBluetoothSetScanWindow(uCxHandle_t * puCxHandle, int32_t scan_window);

/**
 * Read scan Interval.
 * 
 * Output AT command:
 * > AT+UBTSS1?
 *
 * @param[in]  puCxHandle:  uCX API handle
 * @param[out] pScanWindow: Scan window (must be <= Scan interval. 
 *                           Default: 128.
 *                           Calculation: scan_interval * 0.625 ms)
 * @return                  0 on success, negative value on error.
 */
int32_t uCxBluetoothGetScanWindow(uCxHandle_t * puCxHandle, int32_t * pScanWindow);

/**
 * Enable or disable connecting to directed advertisements during scanning.
 * 
 * Notes:
 * Can be stored using AT&W.
 * 
 * Output AT command:
 * > AT+UBTSS2=<connect_to_directed_adv>
 *
 * @param[in]  puCxHandle:              uCX API handle
 * @param      connect_to_directed_adv: Enable (1) or disable (0) connecting to directed advertisements during scanning. Default:
 *                                      0.
 * @return                              0 on success, negative value on error.
 */
int32_t uCxBluetoothSetConnectToDirectedAdv(uCxHandle_t * puCxHandle, int32_t connect_to_directed_adv);

/**
 * Read connect to directed advertisements setting.
 * 
 * Output AT command:
 * > AT+UBTSS2?
 *
 * @param[in]  puCxHandle:            uCX API handle
 * @param[out] pConnectToDirectedAdv: Enable (1) or disable (0) connecting to directed advertisements during scanning. Default:
 *                                    0.
 * @return                            0 on success, negative value on error.
 */
int32_t uCxBluetoothGetConnectToDirectedAdv(uCxHandle_t * puCxHandle, int32_t * pConnectToDirectedAdv);

/**
 * Set I/O Capabilities
 * 
 * Notes:
 * Can be stored using AT&W.
 * 
 * Output AT command:
 * > AT+UBTIOC=<io_capabilities>
 *
 * @param[in]  puCxHandle:      uCX API handle
 * @param      io_capabilities: 
 * @return                      0 on success, negative value on error.
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
 * @return                      0 on success, negative value on error.
 */
int32_t uCxBluetoothGetIoCapabilities(uCxHandle_t * puCxHandle, uIoCapabilities_t * pIoCapabilities);

/**
 * Writes the security mode
 * 
 * Notes:
 * Can be stored using AT&W.
 * 
 * Output AT command:
 * > AT+UBTBSM=<bt_security_mode>
 *
 * @param[in]  puCxHandle:       uCX API handle
 * @param      bt_security_mode: 
 * @return                       0 on success, negative value on error.
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
 * @return                      0 on success, negative value on error.
 */
int32_t uCxBluetoothGetSecurityMode(uCxHandle_t * puCxHandle, uBtSecurityMode_t * pBtSecurityMode);

/**
 * Writes the pairing mode.
 * 
 * Notes:
 * Can be stored using AT&W.
 * 
 * Output AT command:
 * > AT+UBTPM=<pairing_mode>
 *
 * @param[in]  puCxHandle:   uCX API handle
 * @param      pairing_mode: 
 * @return                   0 on success, negative value on error.
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
 * @return                   0 on success, negative value on error.
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
 * @return                 0 on success, negative value on error.
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
 * @return                 0 on success, negative value on error.
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
 * @return                 0 on success, negative value on error.
 */
int32_t uCxBluetoothUserPasskeyEntry3(uCxHandle_t * puCxHandle, uBtLeAddress_t * bd_addr, uYesNo_t yes_no, int32_t passkey);

/**
 * Initiate bonding. To perform the bonding, the remote device must be in a pairable and connectable mode. Bond Event
 * +UEBTB is generated once the bond is complete.
 * 
 * Output AT command:
 * > AT+UBTB=<bd_addr>
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      bd_addr:    Bluetooth device address of the remote device.
 * @return                 0 on success, negative value on error.
 */
int32_t uCxBluetoothBond(uCxHandle_t * puCxHandle, uBtLeAddress_t * bd_addr);

/**
 * Removes a single previously bonded device.
 * 
 * Output AT command:
 * > AT+UBTUB=<bd_addr>
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      bd_addr:    Bluetooth device address of the remote device.
 * @return                 0 on success, negative value on error.
 */
int32_t uCxBluetoothUnbond(uCxHandle_t * puCxHandle, uBtLeAddress_t * bd_addr);

/**
 * Removes all previously bonded devices.
 * 
 * Output AT command:
 * > AT+UBTUB
 *
 * @param[in]  puCxHandle: uCX API handle
 * @return                 0 on success, negative value on error.
 */
int32_t uCxBluetoothUnbondAll(uCxHandle_t * puCxHandle);

/**
 * Read list of bonded devices.
 * 
 * Output AT command:
 * > AT+UBTBDL
 *
 * @param[in]  puCxHandle: uCX API handle
 * @return                 true on success, false on error (error code will be returned by uCxEnd()).
 *
 * NOTES:
 * Must be terminated by calling uCxEnd()
 */
void uCxBluetoothListBondedDevicesBegin(uCxHandle_t * puCxHandle);

/**
 * 
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param[out] pBdAddr:    Bluetooth device address of the remote device.
 * @return                 true on success, false when there are no more entries or on error (uCxEnd() will return
 *                         error code in this case).
 */
bool uCxBluetoothListBondedDevicesGetNext(uCxHandle_t * puCxHandle, uBtLeAddress_t * pBdAddr);

/**
 * Set a characteristic value.
 * 
 * Notes:
 * Can be stored using AT&W.
 * 
 * Output AT command:
 * > AT+UBTDIS=<characteristic_id>,<characteristic_value>
 *
 * @param[in]  puCxHandle:           uCX API handle
 * @param      characteristic_id:    
 * @param      characteristic_value: Value of Device Information Service characteristic.
 * @return                           0 on success, negative value on error.
 */
int32_t uCxBluetoothSetDeviceInfoServiceChar(uCxHandle_t * puCxHandle, uCharacteristicId_t characteristic_id, const char * characteristic_value);

/**
 * Read a characteristic value.
 * 
 * Output AT command:
 * > AT+UBTDIS=<characteristic_id>
 *
 * @param[in]  puCxHandle:            uCX API handle
 * @param      characteristic_id:     
 * @param[out] ppCharacteristicValue: Value of Device Information Service characteristic.
 * @return                            true on success, false on error (error code will be returned by uCxEnd()).
 *
 * NOTES:
 * Must be terminated by calling uCxEnd()
 */
bool uCxBluetoothGetDeviceInfoServiceCharBegin(uCxHandle_t * puCxHandle, uCharacteristicId_t characteristic_id, const char ** ppCharacteristicValue);

/**
 * Read all individual characteristic of the Device Information Service characteristics.
 * 
 * Output AT command:
 * > AT+UBTDIS?
 *
 * @param[in]  puCxHandle: uCX API handle
 * @return                 true on success, false on error (error code will be returned by uCxEnd()).
 *
 * NOTES:
 * Must be terminated by calling uCxEnd()
 */
void uCxBluetoothListDeviceInfoServiceCharsBegin(uCxHandle_t * puCxHandle);

/**
 * 
 *
 * @param[in]  puCxHandle:                              uCX API handle
 * @param[out] pBluetoothListDeviceInfoServiceCharsRsp: Please see \ref uCxBluetoothListDeviceInfoServiceChars_t
 * @return                                              true on success, false when there are no more entries or on error (uCxEnd() will return
 *                                                      error code in this case).
 */
bool uCxBluetoothListDeviceInfoServiceCharsGetNext(uCxHandle_t * puCxHandle, uCxBluetoothListDeviceInfoServiceChars_t * pBluetoothListDeviceInfoServiceCharsRsp);

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
 *                          OR a bit field with bits:
 *                          Bit 0: 1 Mbps preferred
 *                          Bit 1: 2 Mbps preferred
 * @param      rx_phy:      Requested PHY for Receiver
 *                          0: Let other side decide
 *                          OR a bit field with bits:
 *                          Bit 0: 1 Mbps preferred
 *                          Bit 1: 2 Mbps preferred
 * @return                  0 on success, negative value on error.
 */
int32_t uCxBluetoothRequestPhy(uCxHandle_t * puCxHandle, int32_t conn_handle, int32_t tx_phy, int32_t rx_phy);

/**
 * Reads current PHYs for a connection.
 * 
 * Output AT command:
 * > AT+UBTPHYR=<conn_handle>
 *
 * @param[in]  puCxHandle:          uCX API handle
 * @param      conn_handle:         Connection handle of the Bluetooth low energy connection.
 * @param[out] pBluetoothGetPhyRsp: Please see \ref uCxBluetoothGetPhy_t
 * @return                          0 on success, negative value on error.
 */
int32_t uCxBluetoothGetPhy(uCxHandle_t * puCxHandle, int32_t conn_handle, uCxBluetoothGetPhy_t * pBluetoothGetPhyRsp);

/**
 * Register Connect event callback
 * 
 * Event indicating successful Bluetooth connection.
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      callback:   callback to register. Set to NULL to unregister.
 */
void uCxBluetoothRegisterConnect(uCxHandle_t * puCxHandle, uUEBTC_t callback);

/**
 * Register Disconnect event callback
 * 
 * Event indicating a disconnected Bluetooth connection.
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      callback:   callback to register. Set to NULL to unregister.
 */
void uCxBluetoothRegisterDisconnect(uCxHandle_t * puCxHandle, uUEBTDC_t callback);

/**
 * Register BondStatus event callback
 * 
 * Event indicates that a bonding procedure is completed.
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      callback:   callback to register. Set to NULL to unregister.
 */
void uCxBluetoothRegisterBondStatus(uCxHandle_t * puCxHandle, uUEBTB_t callback);

/**
 * Register UserConfirmation event callback
 * 
 * This event is used while bonding with IO capability DisplayYesNo. This event indicates that the user confirmation of a
 * numeric value is required.
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      callback:   callback to register. Set to NULL to unregister.
 */
void uCxBluetoothRegisterUserConfirmation(uCxHandle_t * puCxHandle, uUEBTUC_t callback);

/**
 * Register PasskeyEntry event callback
 * 
 * This event is used to indicate to the user that a passkey has to be entered on the remote device during a bonding
 * procedure with the IO capability DisplayOnly.
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      callback:   callback to register. Set to NULL to unregister.
 */
void uCxBluetoothRegisterPasskeyEntry(uCxHandle_t * puCxHandle, uUEBTUPD_t callback);

/**
 * Register PasskeyRequest event callback
 * 
 * This event is used during bonding with IO capability KeyboardOnly to indicate that a passkey is required from the user.
 * User should respond to this event with the AT+UBTUPE command.
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      callback:   callback to register. Set to NULL to unregister.
 */
void uCxBluetoothRegisterPasskeyRequest(uCxHandle_t * puCxHandle, uUEBTUPE_t callback);

/**
 * Register PhyUpdate event callback
 * 
 * This event informs the result of a PHY update procedure. It may be generated as a result of the command AT+UBTPHYR or as
 * a successful event, if the operation has been initiated by the remote peer.
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      callback:   callback to register. Set to NULL to unregister.
 */
void uCxBluetoothRegisterPhyUpdate(uCxHandle_t * puCxHandle, uUEBTPHYU_t callback);

/**
 * Register BackgroundDiscovery event callback
 * 
 * This event is generated during background discovery when a device is found. The format matches AT+UBTD responses.
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      callback:   callback to register. Set to NULL to unregister.
 */
void uCxBluetoothRegisterBackgroundDiscovery(uCxHandle_t * puCxHandle, uUEBTBGD_t callback);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _U_CX_BLUETOOTH_H_ */