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
#ifndef _U_CX_WIFI_H_
#define _U_CX_WIFI_H_

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
typedef enum
{
    U_CX_WIFI_STATION_GET_SECURITY_RSP_TYPE_SECURITY_MODE_WPA_THRESHOLD,
    U_CX_WIFI_STATION_GET_SECURITY_RSP_TYPE_SECURITY_MODE,
    U_CX_WIFI_STATION_GET_SECURITY_RSP_TYPE_SECURITY_MODE_STR_STR_STR,
    U_CX_WIFI_STATION_GET_SECURITY_RSP_TYPE_SECURITY_MODE_STR_STR
} uCxWifiStationGetSecurityRspType_t;

typedef struct {
    uCxWifiStationGetSecurityRspType_t type;
    union {
        struct
        {
            int32_t security_mode; /**< The current security mode. */
            int32_t wpa_threshold; /**< Lowest WPA version to connect to */
        } rspSecurityModeWpaThreshold;
        struct
        {
            int32_t security_mode; /**< The current security mode. */
        } rspSecurityMode;
        struct
        {
            int32_t security_mode;         /**< The current security mode. */
            const char * ca_name;          /**< Name of the certificate authority (CA) certificate to use */
            const char * client_cert_name; /**< Name of the client certificate to use */
            const char * client_key_name;  /**< Name of the private key for client certificate */
        } rspSecurityModeStrStrStr;
        struct
        {
            int32_t security_mode; /**< The current security mode. */
            const char * username; /**< User name for PEAP authentication. */
            const char * ca_name;  /**< Name of the certificate authority (CA) certificate to use */
        } rspSecurityModeStrStr;
    };
} uCxWifiStationGetSecurity_t;

typedef enum
{
    U_CX_WIFI_STATION_GET_IP_CONFIG_RSP_TYPE_IP_MODE,
    U_CX_WIFI_STATION_GET_IP_CONFIG_RSP_TYPE_IP_MODE_IP_IP_IP_IP_IP
} uCxWifiStationGetIpConfigRspType_t;

typedef struct {
    uCxWifiStationGetIpConfigRspType_t type;
    union {
        struct
        {
            int32_t ip_mode; /**< IP assignment */
        } rspIpMode;
        struct
        {
            int32_t ip_mode;              /**< IP assignment */
            uSockIpAddress_t ip_addr;     /**< Static IPv4 address */
            uSockIpAddress_t subnet_mask; /**< Subnet mask */
            uSockIpAddress_t gateway;     /**< IPv4 gateway address */
            uSockIpAddress_t prim_dns;    /**< IPv4 primary dns address */
            uSockIpAddress_t sec_dns;     /**< IPv4 secondary dns address */
        } rspIpModeIpIpIpIpIp;
    };
} uCxWifiStationGetIpConfig_t;

typedef enum
{
    U_CX_WIFI_STATION_STATUS_RSP_TYPE_WIFI_STATUS_ID_STR,
    U_CX_WIFI_STATION_STATUS_RSP_TYPE_WIFI_STATUS_ID_MAC,
    U_CX_WIFI_STATION_STATUS_RSP_TYPE_WIFI_STATUS_ID_INT
} uCxWifiStationStatusRspType_t;

typedef struct {
    uCxWifiStationStatusRspType_t type;
    union {
        struct
        {
            int32_t wifi_status_id;
            const char * ssid;      /**< SSID */
        } rspWifiStatusIdStr;
        struct
        {
            int32_t wifi_status_id;
            uMacAddress_t bssid;    /**< BSSID of the connected access point */
        } rspWifiStatusIdMac;
        struct
        {
            int32_t wifi_status_id;
            int32_t int_val;        /**< RSSI, Connection status  or Channel */
        } rspWifiStatusIdInt;
    };
} uCxWifiStationStatus_t;

typedef enum
{
    U_CX_WIFI_AP_GET_SECURITY_RSP_TYPE_SECURITY_MODE_WPA_VERSION,
    U_CX_WIFI_AP_GET_SECURITY_RSP_TYPE_SECURITY_MODE
} uCxWifiApGetSecurityRspType_t;

typedef struct {
    uCxWifiApGetSecurityRspType_t type;
    union {
        struct
        {
            int32_t security_mode; /**< The current security mode. */
            int32_t wpa_version;
        } rspSecurityModeWpaVersion;
        struct
        {
            int32_t security_mode; /**< The current security mode. */
        } rspSecurityMode;
    };
} uCxWifiApGetSecurity_t;


typedef struct
{
    int32_t status_id;
    uSockIpAddress_t status_val; /**< IP address */
} uCxWifiStationListNetworkStatus_t;

typedef struct
{
    uMacAddress_t bssid;           /**< BSSID */
    const char * ssid;             /**< SSID */
    int32_t channel;               /**< Channel */
    int32_t rssi;                  /**< RSSI */
    int32_t authentication_suites; /**< Authentication suites. Bit 0 = shared secret, 1 = PSK, 2 = EAP, 3 = WPA, 4 = WPA2, 5 =
                                        WPA3 */
    int32_t unicast_ciphers;       /**< unicast ciphers. Bit 0 = WEP64, 1 = WEP128, 2 = TKIP, 3 = AES/CCMP */
    int32_t group_ciphers;         /**< group ciphers. Bit 0 = WEP64, 1 = WEP128, 2 = TKIP, 3 = AES/CCMP */
} uCxWifiStationScanDefault_t;

typedef struct
{
    uMacAddress_t bssid;           /**< BSSID */
    const char * ssid;             /**< SSID */
    int32_t channel;               /**< Channel */
    int32_t rssi;                  /**< RSSI */
    int32_t authentication_suites; /**< Authentication suites. Bit 0 = shared secret, 1 = PSK, 2 = EAP, 3 = WPA, 4 = WPA2, 5 =
                                        WPA3 */
    int32_t unicast_ciphers;       /**< unicast ciphers. Bit 0 = WEP64, 1 = WEP128, 2 = TKIP, 3 = AES/CCMP */
    int32_t group_ciphers;         /**< group ciphers. Bit 0 = WEP64, 1 = WEP128, 2 = TKIP, 3 = AES/CCMP */
} uCxWifiStationScan_t;

typedef struct
{
    const char * ssid; /**< SSID */
    int32_t channel;   /**< channel */
} uCxWifiApGetConnectionParams_t;

typedef struct
{
    int32_t status_id;
    uSockIpAddress_t status_val; /**< IP address */
} uCxWifiApListNetworkStatus_t;


/* ------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * ---------------------------------------------------------- */

/**
 * Set the Host Name
 * 
 * Notes:
 * Can be stored using AT&W.
 * 
 * Output AT command:
 * > AT+UWHN=<host_name>
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      host_name:  
 * @return                 0 on success, negative value on error.
 */
int32_t uCxWifiSetHostname(uCxHandle_t * puCxHandle, const char * host_name);

/**
 * Reads the current hostname
 * 
 * Output AT command:
 * > AT+UWHN?
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param[out] ppHostName: 
 * @return                 true on success, false on error (error code will be returned by uCxEnd()).
 *
 * NOTES:
 * Must be terminated by calling uCxEnd()
 */
bool uCxWifiGetHostnameBegin(uCxHandle_t * puCxHandle, const char ** ppHostName);

/**
 * Set the EAP-TLS connection parameters to use.
 * 
 * Notes:
 * Can be stored using AT&W.
 * 
 * Output AT command:
 * > AT+UWSSE=<wlan_handle>,<ca_name>,<client_cert_name>,<client_key_name>
 *
 * @param[in]  puCxHandle:       uCX API handle
 * @param      wlan_handle:      Handle to use for Wi-Fi config and connection
 * @param      ca_name:          Name of the certificate authority (CA) certificate to use
 * @param      client_cert_name: Name of the client certificate to use
 * @param      client_key_name:  Name of the private key for client certificate
 * @return                       0 on success, negative value on error.
 */
int32_t uCxWifiStationSetSecurityEnterprise(uCxHandle_t * puCxHandle, int32_t wlan_handle, const char * ca_name, const char * client_cert_name, const char * client_key_name);

/**
 * Get the current Wi-Fi station security config
 * 
 * Output AT command:
 * > AT+UWSS=<wlan_handle>
 *
 * @param[in]  puCxHandle:                 uCX API handle
 * @param      wlan_handle:                Handle to use for Wi-Fi config and connection
 * @param[out] pWifiStationGetSecurityRsp: Please see \ref uCxWifiStationGetSecurity_t
 * @return                                 true on success, false on error (error code will be returned by uCxEnd()).
 *
 * NOTES:
 * Must be terminated by calling uCxEnd()
 */
bool uCxWifiStationGetSecurityBegin(uCxHandle_t * puCxHandle, int32_t wlan_handle, uCxWifiStationGetSecurity_t * pWifiStationGetSecurityRsp);

/**
 * Set the PEAP connection parameters to use.
 * 
 * Notes:
 * Can be stored using AT&W.
 * 
 * Output AT command:
 * > AT+UWSSP=<wlan_handle>,<peap_user>,<peap_password>
 *
 * @param[in]  puCxHandle:    uCX API handle
 * @param      wlan_handle:   Handle to use for Wi-Fi config and connection
 * @param      peap_user:     User name for PEAP authentication. Could be either only username or username@domain. Use @
 *                            as separator
 * @param      peap_password: Password for PEAP authentication.
 * @return                    0 on success, negative value on error.
 */
int32_t uCxWifiStationSetSecurityPeap3(uCxHandle_t * puCxHandle, int32_t wlan_handle, const char * peap_user, const char * peap_password);

/**
 * Set the PEAP connection parameters to use.
 * 
 * Notes:
 * Can be stored using AT&W.
 * 
 * Output AT command:
 * > AT+UWSSP=<wlan_handle>,<peap_user>,<peap_password>,<ca_name>
 *
 * @param[in]  puCxHandle:    uCX API handle
 * @param      wlan_handle:   Handle to use for Wi-Fi config and connection
 * @param      peap_user:     User name for PEAP authentication. Could be either only username or username@domain. Use @
 *                            as separator
 * @param      peap_password: Password for PEAP authentication.
 * @param      ca_name:       Name of the certificate authority (CA) certificate to use
 * @return                    0 on success, negative value on error.
 */
int32_t uCxWifiStationSetSecurityPeap4(uCxHandle_t * puCxHandle, int32_t wlan_handle, const char * peap_user, const char * peap_password, const char * ca_name);

/**
 * Set WPA connection parameters to use
 * 
 * Notes:
 * Can be stored using AT&W.
 * 
 * Output AT command:
 * > AT+UWSSW=<wlan_handle>,<passphrase>,<wpa_threshold>
 *
 * @param[in]  puCxHandle:    uCX API handle
 * @param      wlan_handle:   Handle to use for Wi-Fi config and connection
 * @param      passphrase:    Passphrase to use for WPA connection
 * @param      wpa_threshold: Lowest WPA version to connect to
 * @return                    0 on success, negative value on error.
 */
int32_t uCxWifiStationSetSecurityWpa(uCxHandle_t * puCxHandle, int32_t wlan_handle, const char * passphrase, uWpaThreshold_t wpa_threshold);

/**
 * Sets security to open security
 * 
 * Notes:
 * Can be stored using AT&W.
 * 
 * Output AT command:
 * > AT+UWSSO=<wlan_handle>
 *
 * @param[in]  puCxHandle:  uCX API handle
 * @param      wlan_handle: Handle to use for Wi-Fi config and connection
 * @return                  0 on success, negative value on error.
 */
int32_t uCxWifiStationSetSecurityOpen(uCxHandle_t * puCxHandle, int32_t wlan_handle);

/**
 * Sets the connection parameters for the connection.
 * 
 * Notes:
 * Can be stored using AT&W.
 * 
 * Output AT command:
 * > AT+UWSCP=<wlan_handle>,<ssid>
 *
 * @param[in]  puCxHandle:  uCX API handle
 * @param      wlan_handle: Handle to use for Wi-Fi config and connection
 * @param      ssid:        SSID
 * @return                  0 on success, negative value on error.
 */
int32_t uCxWifiStationSetConnectionParams(uCxHandle_t * puCxHandle, int32_t wlan_handle, const char * ssid);

/**
 * Reads the connection parameters for the connection.
 * 
 * Output AT command:
 * > AT+UWSCP=<wlan_handle>
 *
 * @param[in]  puCxHandle:  uCX API handle
 * @param      wlan_handle: Handle to use for Wi-Fi config and connection
 * @param[out] ppSsid:      SSID
 * @return                  true on success, false on error (error code will be returned by uCxEnd()).
 *
 * NOTES:
 * Must be terminated by calling uCxEnd()
 */
bool uCxWifiStationGetConnectionParamsBegin(uCxHandle_t * puCxHandle, int32_t wlan_handle, const char ** ppSsid);

/**
 * Sets ip configuration to use static ip
 * 
 * Notes:
 * Can be stored using AT&W.
 * 
 * Output AT command:
 * > AT+UWSIPS=<wlan_handle>,<ip_addr>,<subnet_mask>,<gateway>
 *
 * @param[in]  puCxHandle:  uCX API handle
 * @param      wlan_handle: Handle to use for Wi-Fi config and connection
 * @param      ip_addr:     Static IPv4 address
 * @param      subnet_mask: Subnet mask
 * @param      gateway:     IPv4 gateway address
 * @return                  0 on success, negative value on error.
 */
int32_t uCxWifiStationSetIpConfigStatic4(uCxHandle_t * puCxHandle, int32_t wlan_handle, uSockIpAddress_t * ip_addr, uSockIpAddress_t * subnet_mask, uSockIpAddress_t * gateway);

/**
 * Sets ip configuration to use static ip
 * 
 * Notes:
 * Can be stored using AT&W.
 * 
 * Output AT command:
 * > AT+UWSIPS=<wlan_handle>,<ip_addr>,<subnet_mask>,<gateway>,<prim_dns>
 *
 * @param[in]  puCxHandle:  uCX API handle
 * @param      wlan_handle: Handle to use for Wi-Fi config and connection
 * @param      ip_addr:     Static IPv4 address
 * @param      subnet_mask: Subnet mask
 * @param      gateway:     IPv4 gateway address
 * @param      prim_dns:    IPv4 primary dns address
 * @return                  0 on success, negative value on error.
 */
int32_t uCxWifiStationSetIpConfigStatic5(uCxHandle_t * puCxHandle, int32_t wlan_handle, uSockIpAddress_t * ip_addr, uSockIpAddress_t * subnet_mask, uSockIpAddress_t * gateway, uSockIpAddress_t * prim_dns);

/**
 * Sets ip configuration to use static ip
 * 
 * Notes:
 * Can be stored using AT&W.
 * 
 * Output AT command:
 * > AT+UWSIPS=<wlan_handle>,<ip_addr>,<subnet_mask>,<gateway>,<prim_dns>,<sec_dns>
 *
 * @param[in]  puCxHandle:  uCX API handle
 * @param      wlan_handle: Handle to use for Wi-Fi config and connection
 * @param      ip_addr:     Static IPv4 address
 * @param      subnet_mask: Subnet mask
 * @param      gateway:     IPv4 gateway address
 * @param      prim_dns:    IPv4 primary dns address
 * @param      sec_dns:     IPv4 secondary dns address
 * @return                  0 on success, negative value on error.
 */
int32_t uCxWifiStationSetIpConfigStatic6(uCxHandle_t * puCxHandle, int32_t wlan_handle, uSockIpAddress_t * ip_addr, uSockIpAddress_t * subnet_mask, uSockIpAddress_t * gateway, uSockIpAddress_t * prim_dns, uSockIpAddress_t * sec_dns);

/**
 * Sets ip configuration to receive ip address via dhcp
 * 
 * Notes:
 * Can be stored using AT&W.
 * 
 * Output AT command:
 * > AT+UWSIPD=<wlan_handle>
 *
 * @param[in]  puCxHandle:  uCX API handle
 * @param      wlan_handle: Handle to use for Wi-Fi config and connection
 * @return                  0 on success, negative value on error.
 */
int32_t uCxWifiStationSetIpConfigDhcp(uCxHandle_t * puCxHandle, int32_t wlan_handle);

/**
 * Read the current configuration for IP address assignment
 * 
 * Output AT command:
 * > AT+UWSIP=<wlan_handle>
 *
 * @param[in]  puCxHandle:                 uCX API handle
 * @param      wlan_handle:                Handle to use for Wi-Fi config and connection
 * @param[out] pWifiStationGetIpConfigRsp: Please see \ref uCxWifiStationGetIpConfig_t
 * @return                                 0 on success, negative value on error.
 */
int32_t uCxWifiStationGetIpConfig(uCxHandle_t * puCxHandle, int32_t wlan_handle, uCxWifiStationGetIpConfig_t * pWifiStationGetIpConfigRsp);

/**
 * Initiate connection to Wi-Fi network
 * 
 * Notes:
 * Can be stored using AT&W.
 * 
 * Output AT command:
 * > AT+UWSC=<wlan_handle>
 *
 * @param[in]  puCxHandle:  uCX API handle
 * @param      wlan_handle: Handle to use for Wi-Fi config and connection
 * @return                  0 on success, negative value on error.
 */
int32_t uCxWifiStationConnect(uCxHandle_t * puCxHandle, int32_t wlan_handle);

/**
 * Disconnect from Wi-Fi network
 * 
 * Notes:
 * Can be stored using AT&W.
 * 
 * Output AT command:
 * > AT+UWSDC
 *
 * @param[in]  puCxHandle: uCX API handle
 * @return                 0 on success, negative value on error.
 */
int32_t uCxWifiStationDisconnect(uCxHandle_t * puCxHandle);

/**
 * Show current status of Wi-Fi station network interface
 * 
 * Output AT command:
 * > AT+UWSNST=<status_id>
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      status_id:  
 * @param[out] pStatusVal: IP address
 * @return                 0 on success, negative value on error.
 */
int32_t uCxWifiStationGetNetworkStatus(uCxHandle_t * puCxHandle, uStatusId_t status_id, uSockIpAddress_t * pStatusVal);

/**
 * Show current status of Wi-Fi station network interface
 * 
 * Output AT command:
 * > AT+UWSNST?
 *
 * @param[in]  puCxHandle: uCX API handle
 * @return                 true on success, false on error (error code will be returned by uCxEnd()).
 *
 * NOTES:
 * Must be terminated by calling uCxEnd()
 */
void uCxWifiStationListNetworkStatusBegin(uCxHandle_t * puCxHandle);

/**
 * 
 *
 * @param[in]  puCxHandle:                       uCX API handle
 * @param[out] pWifiStationListNetworkStatusRsp: Please see \ref uCxWifiStationListNetworkStatus_t
 * @return                                       true on success, false when there are no more entries or on error (uCxEnd() will return
 *                                               error code in this case).
 */
bool uCxWifiStationListNetworkStatusGetNext(uCxHandle_t * puCxHandle, uCxWifiStationListNetworkStatus_t * pWifiStationListNetworkStatusRsp);

/**
 * Initiate synchronous Wi-Fi scan (will lock AT interface until scan has finished)
 * 
 * Output AT command:
 * > AT+UWSSC
 *
 * @param[in]  puCxHandle: uCX API handle
 * @return                 true on success, false on error (error code will be returned by uCxEnd()).
 *
 * NOTES:
 * Must be terminated by calling uCxEnd()
 */
void uCxWifiStationScanDefaultBegin(uCxHandle_t * puCxHandle);

/**
 * 
 *
 * @param[in]  puCxHandle:                 uCX API handle
 * @param[out] pWifiStationScanDefaultRsp: Please see \ref uCxWifiStationScanDefault_t
 * @return                                 true on success, false when there are no more entries or on error (uCxEnd() will return
 *                                         error code in this case).
 */
bool uCxWifiStationScanDefaultGetNext(uCxHandle_t * puCxHandle, uCxWifiStationScanDefault_t * pWifiStationScanDefaultRsp);

/**
 * Initiate synchronous Wi-Fi scan (will lock AT interface until scan has finished)
 * 
 * Output AT command:
 * > AT+UWSSC=<scan_mode>
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      scan_mode:  Choose how to scan
 * @return                 true on success, false on error (error code will be returned by uCxEnd()).
 *
 * NOTES:
 * Must be terminated by calling uCxEnd()
 */
void uCxWifiStationScan1Begin(uCxHandle_t * puCxHandle, uScanMode_t scan_mode);

/**
 * 
 *
 * @param[in]  puCxHandle:          uCX API handle
 * @param[out] pWifiStationScanRsp: Please see \ref uCxWifiStationScan_t
 * @return                          true on success, false when there are no more entries or on error (uCxEnd() will return
 *                                  error code in this case).
 */
bool uCxWifiStationScan1GetNext(uCxHandle_t * puCxHandle, uCxWifiStationScan_t * pWifiStationScanRsp);

/**
 * Initiate synchronous Wi-Fi scan (will lock AT interface until scan has finished)
 * 
 * Output AT command:
 * > AT+UWSSC=<scan_mode>,<ssid>
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      scan_mode:  Choose how to scan
 * @param      ssid:       SSID
 * @return                 true on success, false on error (error code will be returned by uCxEnd()).
 *
 * NOTES:
 * Must be terminated by calling uCxEnd()
 */
void uCxWifiStationScan2Begin(uCxHandle_t * puCxHandle, uScanMode_t scan_mode, const char * ssid);

/**
 * 
 *
 * @param[in]  puCxHandle:          uCX API handle
 * @param[out] pWifiStationScanRsp: Please see \ref uCxWifiStationScan_t
 * @return                          true on success, false when there are no more entries or on error (uCxEnd() will return
 *                                  error code in this case).
 */
bool uCxWifiStationScan2GetNext(uCxHandle_t * puCxHandle, uCxWifiStationScan_t * pWifiStationScanRsp);

/**
 * Read status
 * 
 * Output AT command:
 * > AT+UWSST=<wifi_status_id>
 *
 * @param[in]  puCxHandle:            uCX API handle
 * @param      wifi_status_id:        
 * @param[out] pWifiStationStatusRsp: Please see \ref uCxWifiStationStatus_t
 * @return                            true on success, false on error (error code will be returned by uCxEnd()).
 *
 * NOTES:
 * Must be terminated by calling uCxEnd()
 */
bool uCxWifiStationStatusBegin(uCxHandle_t * puCxHandle, uWifiStatusId_t wifi_status_id, uCxWifiStationStatus_t * pWifiStationStatusRsp);

/**
 * Start an access point with the current access point configuration.
 * 
 * Notes:
 * Can be stored using AT&W.
 * 
 * Output AT command:
 * > AT+UWAPA
 *
 * @param[in]  puCxHandle: uCX API handle
 * @return                 0 on success, negative value on error.
 */
int32_t uCxWifiApActivate(uCxHandle_t * puCxHandle);

/**
 * Brings down Wi-Fi access point and disconnect all connected stations
 * 
 * Notes:
 * Can be stored using AT&W.
 * 
 * Output AT command:
 * > AT+UWAPD
 *
 * @param[in]  puCxHandle: uCX API handle
 * @return                 0 on success, negative value on error.
 */
int32_t uCxWifiApDeactivate(uCxHandle_t * puCxHandle);

/**
 * Sets connection parameters for the AP configuration
 * 
 * Notes:
 * Can be stored using AT&W.
 * 
 * Output AT command:
 * > AT+UWAPCP=<ssid>
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      ssid:       SSID
 * @return                 0 on success, negative value on error.
 */
int32_t uCxWifiApSetConnectionParams1(uCxHandle_t * puCxHandle, const char * ssid);

/**
 * Sets connection parameters for the AP configuration
 * 
 * Notes:
 * Can be stored using AT&W.
 * 
 * Output AT command:
 * > AT+UWAPCP=<ssid>,<channel>
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      ssid:       SSID
 * @param      channel:    channel
 * @return                 0 on success, negative value on error.
 */
int32_t uCxWifiApSetConnectionParams2(uCxHandle_t * puCxHandle, const char * ssid, uChannel_t channel);

/**
 * Read the current connection parameters
 * 
 * Output AT command:
 * > AT+UWAPCP?
 *
 * @param[in]  puCxHandle:                    uCX API handle
 * @param[out] pWifiApGetConnectionParamsRsp: Please see \ref uCxWifiApGetConnectionParams_t
 * @return                                    true on success, false on error (error code will be returned by uCxEnd()).
 *
 * NOTES:
 * Must be terminated by calling uCxEnd()
 */
bool uCxWifiApGetConnectionParamsBegin(uCxHandle_t * puCxHandle, uCxWifiApGetConnectionParams_t * pWifiApGetConnectionParamsRsp);

/**
 * Sets WPA parameters for the AP config
 * 
 * Notes:
 * Can be stored using AT&W.
 * 
 * Output AT command:
 * > AT+UWAPSW=<passphrase>
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      passphrase: Passphrase to use
 * @return                 0 on success, negative value on error.
 */
int32_t uCxWifiApSetSecurityWpa1(uCxHandle_t * puCxHandle, const char * passphrase);

/**
 * Sets WPA parameters for the AP config
 * 
 * Notes:
 * Can be stored using AT&W.
 * 
 * Output AT command:
 * > AT+UWAPSW=<passphrase>,<wpa_version>
 *
 * @param[in]  puCxHandle:  uCX API handle
 * @param      passphrase:  Passphrase to use
 * @param      wpa_version: 
 * @return                  0 on success, negative value on error.
 */
int32_t uCxWifiApSetSecurityWpa2(uCxHandle_t * puCxHandle, const char * passphrase, uWpaVersion_t wpa_version);

/**
 * Sets security level to open for the AP config
 * 
 * Notes:
 * Can be stored using AT&W.
 * 
 * Output AT command:
 * > AT+UWAPSO
 *
 * @param[in]  puCxHandle: uCX API handle
 * @return                 0 on success, negative value on error.
 */
int32_t uCxWifiApSetSecurityOpen(uCxHandle_t * puCxHandle);

/**
 * Get the current security configuration for Wi-Fi AP
 * 
 * Output AT command:
 * > AT+UWAPS?
 *
 * @param[in]  puCxHandle:            uCX API handle
 * @param[out] pWifiApGetSecurityRsp: Please see \ref uCxWifiApGetSecurity_t
 * @return                            0 on success, negative value on error.
 */
int32_t uCxWifiApGetSecurity(uCxHandle_t * puCxHandle, uCxWifiApGetSecurity_t * pWifiApGetSecurityRsp);

/**
 * Get a list of connected stations. One response will be sent for each connected station
 * 
 * Output AT command:
 * > AT+UWAPCS?
 *
 * @param[in]  puCxHandle: uCX API handle
 * @return                 true on success, false on error (error code will be returned by uCxEnd()).
 *
 * NOTES:
 * Must be terminated by calling uCxEnd()
 */
void uCxWifiApListStationsBegin(uCxHandle_t * puCxHandle);

/**
 * 
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param[out] pMac:       MAC address of the connected Wi-Fi Station
 * @return                 true on success, false when there are no more entries or on error (uCxEnd() will return
 *                         error code in this case).
 */
bool uCxWifiApListStationsGetNext(uCxHandle_t * puCxHandle, uMacAddress_t * pMac);

/**
 * Show current status of Wi-Fi station network interface
 * 
 * Output AT command:
 * > AT+UWAPNST=<status_id>
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      status_id:  
 * @param[out] pStatusVal: IP address
 * @return                 0 on success, negative value on error.
 */
int32_t uCxWifiApGetNetworkStatus(uCxHandle_t * puCxHandle, uStatusId_t status_id, uSockIpAddress_t * pStatusVal);

/**
 * Show current status of Wi-Fi station network interface
 * 
 * Output AT command:
 * > AT+UWAPNST?
 *
 * @param[in]  puCxHandle: uCX API handle
 * @return                 true on success, false on error (error code will be returned by uCxEnd()).
 *
 * NOTES:
 * Must be terminated by calling uCxEnd()
 */
void uCxWifiApListNetworkStatusBegin(uCxHandle_t * puCxHandle);

/**
 * 
 *
 * @param[in]  puCxHandle:                  uCX API handle
 * @param[out] pWifiApListNetworkStatusRsp: Please see \ref uCxWifiApListNetworkStatus_t
 * @return                                  true on success, false when there are no more entries or on error (uCxEnd() will return
 *                                          error code in this case).
 */
bool uCxWifiApListNetworkStatusGetNext(uCxHandle_t * puCxHandle, uCxWifiApListNetworkStatus_t * pWifiApListNetworkStatusRsp);

/**
 * Register LinkUp event callback
 * 
 * This event is sent when Wi-Fi Link goes up
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      callback:   callback to register. Set to NULL to unregister.
 */
void uCxWifiRegisterLinkUp(uCxHandle_t * puCxHandle, uUEWLU_t callback);

/**
 * Register LinkDown event callback
 * 
 * This event is sent when Wi-Fi Link goes down
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      callback:   callback to register. Set to NULL to unregister.
 */
void uCxWifiRegisterLinkDown(uCxHandle_t * puCxHandle, uUEWLD_t callback);

/**
 * Register StationNetworkUp event callback
 * 
 * This event is sent when Wi-Fi Station network is up
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      callback:   callback to register. Set to NULL to unregister.
 */
void uCxWifiRegisterStationNetworkUp(uCxHandle_t * puCxHandle, uUEWSNU_t callback);

/**
 * Register StationNetworkDown event callback
 * 
 * This event is sent when Wi-Fi Station network is down
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      callback:   callback to register. Set to NULL to unregister.
 */
void uCxWifiRegisterStationNetworkDown(uCxHandle_t * puCxHandle, uUEWSND_t callback);

/**
 * Register ApNetworkUp event callback
 * 
 * This event is sent when Wi-Fi Access Point network is up
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      callback:   callback to register. Set to NULL to unregister.
 */
void uCxWifiRegisterApNetworkUp(uCxHandle_t * puCxHandle, uUEWAPNU_t callback);

/**
 * Register ApNetworkDown event callback
 * 
 * This event is sent when Wi-Fi Access Point network is down
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      callback:   callback to register. Set to NULL to unregister.
 */
void uCxWifiRegisterApNetworkDown(uCxHandle_t * puCxHandle, uUEWAPND_t callback);

/**
 * Register ApUp event callback
 * 
 * This event is sent when Wi-Fi Access Point is started
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      callback:   callback to register. Set to NULL to unregister.
 */
void uCxWifiRegisterApUp(uCxHandle_t * puCxHandle, uUEWAPU_t callback);

/**
 * Register ApDown event callback
 * 
 * This event is sent when Wi-Fi Access Point is stopped
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      callback:   callback to register. Set to NULL to unregister.
 */
void uCxWifiRegisterApDown(uCxHandle_t * puCxHandle, uUEWAPD_t callback);

/**
 * Register ApStationAssociated event callback
 * 
 * This event is sent when a Wi-Fi station has associated with the Wi-Fi Access point
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      callback:   callback to register. Set to NULL to unregister.
 */
void uCxWifiRegisterApStationAssociated(uCxHandle_t * puCxHandle, uUEWAPSA_t callback);

/**
 * Register ApStationDisassociated event callback
 * 
 * This event is sent when a Wi-Fi station has disassociated with the Wi-Fi Access point
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      callback:   callback to register. Set to NULL to unregister.
 */
void uCxWifiRegisterApStationDisassociated(uCxHandle_t * puCxHandle, uUEWAPSDA_t callback);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _U_CX_WIFI_H_ */