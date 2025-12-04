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
    U_CX_WIFI_STATION_GET_SECURITY_RSP_TYPE_SECURITY_MODE_STR_STR_STR_STR,
    U_CX_WIFI_STATION_GET_SECURITY_RSP_TYPE_SECURITY_MODE_TLS_VERSION_STR_STR_STR_STR,
    U_CX_WIFI_STATION_GET_SECURITY_RSP_TYPE_SECURITY_MODE_STR_STR,
    U_CX_WIFI_STATION_GET_SECURITY_RSP_TYPE_SECURITY_MODE_TLS_VERSION_STR_STR
} uCxWifiStationGetSecurityRspType_t;

typedef struct {
    uCxWifiStationGetSecurityRspType_t type;
    union {
        struct
        {
            int32_t security_mode; /**< The current security mode. */
            int32_t wpa_threshold; /**< Lowest WPA version to connect to */
        } SecurityModeWpaThreshold;
        struct
        {
            int32_t security_mode; /**< The current security mode. */
        } SecurityMode;
        struct
        {
            int32_t security_mode;         /**< The current security mode. */
            const char * ca_name;          /**< Name of the certificate authority (CA) certificate to use */
            const char * client_cert_name; /**< Name of the client certificate to use */
            const char * client_key_name;  /**< Name of the private key for client certificate */
            const char * identity;         /**< Identity for EAP-TLS */
        } SecurityModeStrStrStrStr;
        struct
        {
            int32_t security_mode;         /**< The current security mode. */
            int32_t tls_version;           /**< TLS version to use */
            const char * ca_name;          /**< Name of the certificate authority (CA) certificate to use */
            const char * client_cert_name; /**< Name of the client certificate to use */
            const char * client_key_name;  /**< Name of the private key for client certificate */
            const char * identity;         /**< Identity for EAP-TLS */
        } SecurityModeTlsVersionStrStrStrStr;
        struct
        {
            int32_t security_mode; /**< The current security mode. */
            const char * username; /**< User name for PEAP authentication. */
            const char * ca_name;  /**< Name of the certificate authority (CA) certificate to use */
        } SecurityModeStrStr;
        struct
        {
            int32_t security_mode; /**< The current security mode. */
            int32_t tls_version;   /**< TLS version to use */
            const char * username; /**< User name for PEAP authentication. */
            const char * ca_name;  /**< Name of the certificate authority (CA) certificate to use */
        } SecurityModeTlsVersionStrStr;
    } rsp;
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
        } IpMode;
        struct
        {
            int32_t ip_mode;              /**< IP assignment */
            uSockIpAddress_t ip_addr;     /**< Static IPv4 address */
            uSockIpAddress_t subnet_mask; /**< Subnet mask */
            uSockIpAddress_t gateway;     /**< IPv4 gateway address */
            uSockIpAddress_t prim_dns;    /**< IPv4 primary dns address */
            uSockIpAddress_t sec_dns;     /**< IPv4 secondary dns address */
        } IpModeIpIpIpIpIp;
    } rsp;
} uCxWifiStationGetIpConfig_t;

typedef enum
{
    U_CX_WIFI_STATION_STATUS_RSP_TYPE_STATUS_ID_STR,
    U_CX_WIFI_STATION_STATUS_RSP_TYPE_STATUS_ID_MAC,
    U_CX_WIFI_STATION_STATUS_RSP_TYPE_STATUS_ID_INT
} uCxWifiStationStatusRspType_t;

typedef struct {
    uCxWifiStationStatusRspType_t type;
    union {
        struct
        {
            int32_t status_id;
            const char * ssid; /**< SSID */
        } StatusIdStr;
        struct
        {
            int32_t status_id;
            uMacAddress_t bssid; /**< BSSID of the connected access point */
        } StatusIdMac;
        struct
        {
            int32_t status_id;
            int32_t int_val;   /**< RSSI, Connection status  or Channel */
        } StatusIdInt;
    } rsp;
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
        } SecurityModeWpaVersion;
        struct
        {
            int32_t security_mode; /**< The current security mode. */
        } SecurityMode;
    } rsp;
} uCxWifiApGetSecurity_t;


typedef struct
{
    int32_t net_status_id;
    uSockIpAddress_t net_status_val; /**< IP address */
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
    int32_t net_status_id;
    uSockIpAddress_t net_status_val; /**< IP address */
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
 * > AT+UWSSE=<wlan_handle>,<tls_version>,<ca_name>,<client_cert_name>,<client_key_name>
 *
 * @param[in]  puCxHandle:       uCX API handle
 * @param      wlan_handle:      Handle to use for Wi-Fi config and connection
 * @param      tls_version:      TLS version to use
 * @param      ca_name:          Name of the certificate authority (CA) certificate to use
 * @param      client_cert_name: Name of the client certificate to use
 * @param      client_key_name:  Name of the private key for client certificate
 * @return                       0 on success, negative value on error.
 */
int32_t uCxWifiStationSetSecurityEnterprise5(uCxHandle_t * puCxHandle, int32_t wlan_handle, uWifiTlsVersion_t tls_version, const char * ca_name, const char * client_cert_name, const char * client_key_name);

/**
 * Set the EAP-TLS connection parameters to use.
 * 
 * Notes:
 * Can be stored using AT&W.
 * 
 * Output AT command:
 * > AT+UWSSE=<wlan_handle>,<tls_version>,<ca_name>,<client_cert_name>,<client_key_name>,<identity>
 *
 * @param[in]  puCxHandle:       uCX API handle
 * @param      wlan_handle:      Handle to use for Wi-Fi config and connection
 * @param      tls_version:      TLS version to use
 * @param      ca_name:          Name of the certificate authority (CA) certificate to use
 * @param      client_cert_name: Name of the client certificate to use
 * @param      client_key_name:  Name of the private key for client certificate
 * @param      identity:         Identity for EAP-TLS
 * @return                       0 on success, negative value on error.
 */
int32_t uCxWifiStationSetSecurityEnterprise6(uCxHandle_t * puCxHandle, int32_t wlan_handle, uWifiTlsVersion_t tls_version, const char * ca_name, const char * client_cert_name, const char * client_key_name, const char * identity);

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
 * > AT+UWSSP=<wlan_handle>,<tls_version>,<peap_user>,<peap_password>
 *
 * @param[in]  puCxHandle:    uCX API handle
 * @param      wlan_handle:   Handle to use for Wi-Fi config and connection
 * @param      tls_version:   TLS version to use
 * @param      peap_user:     User name for PEAP authentication. Could be either only username or username@domain. Use @
 *                            as separator
 * @param      peap_password: Password for PEAP authentication.
 * @return                    0 on success, negative value on error.
 */
int32_t uCxWifiStationSetSecurityPeap4(uCxHandle_t * puCxHandle, int32_t wlan_handle, uWifiTlsVersion_t tls_version, const char * peap_user, const char * peap_password);

/**
 * Set the PEAP connection parameters to use.
 * 
 * Notes:
 * Can be stored using AT&W.
 * 
 * Output AT command:
 * > AT+UWSSP=<wlan_handle>,<tls_version>,<peap_user>,<peap_password>,<ca_name>
 *
 * @param[in]  puCxHandle:    uCX API handle
 * @param      wlan_handle:   Handle to use for Wi-Fi config and connection
 * @param      tls_version:   TLS version to use
 * @param      peap_user:     User name for PEAP authentication. Could be either only username or username@domain. Use @
 *                            as separator
 * @param      peap_password: Password for PEAP authentication.
 * @param      ca_name:       Name of the certificate authority (CA) certificate to use
 * @return                    0 on success, negative value on error.
 */
int32_t uCxWifiStationSetSecurityPeap5(uCxHandle_t * puCxHandle, int32_t wlan_handle, uWifiTlsVersion_t tls_version, const char * peap_user, const char * peap_password, const char * ca_name);

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
int32_t uCxWifiStationSetSecurityWpa(uCxHandle_t * puCxHandle, int32_t wlan_handle, const char * passphrase, uWifiWpaThreshold_t wpa_threshold);

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
 * > AT+UWSNST=<net_status_id>
 *
 * @param[in]  puCxHandle:    uCX API handle
 * @param      net_status_id: 
 * @param[out] pNetStatusVal: IP address
 * @return                    0 on success, negative value on error.
 */
int32_t uCxWifiStationGetNetworkStatus(uCxHandle_t * puCxHandle, uWifiNetStatusId_t net_status_id, uSockIpAddress_t * pNetStatusVal);

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
 * Set the regulatory domain for all Wi-Fi interfaces. This will configure the channel list and power levels for Wi-Fi.
 * Make sure to set the correct code for the region the device will be operating in. Both 2.4 and 5 GHz band is always
 * operative. Set the domain before starting AP or Station
 * 
 * Notes:
 * Can be stored using AT&W.
 * 
 * Output AT command:
 * > AT+UWRD=<reg_domain>
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      reg_domain: Regulatory domain
 * @return                 0 on success, negative value on error.
 */
int32_t uCxWifiSetRegulatoryDomain(uCxHandle_t * puCxHandle, uWifiRegDomain_t reg_domain);

/**
 * Read regulatory domain
 * 
 * Output AT command:
 * > AT+UWRD?
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param[out] pRegDomain: Regulatory domain
 * @return                 0 on success, negative value on error.
 */
int32_t uCxWifiGetRegulatoryDomain(uCxHandle_t * puCxHandle, uWifiRegDomain_t * pRegDomain);

/**
 * Together with the regulatory domain, the channel list determines which channels to used during scanning and connection,
 * a mix of 2.4 and 5 GHz channels is possible. Note that the brackets [ ] must be included in the command, example
 * AT+UWCL=[1,6,11,48,64]
 * 
 * Notes:
 * Can be stored using AT&W.
 * 
 * Output AT command:
 * > AT+UWCL=<channel_list>,<channel_list_len>
 *
 * @param[in]  puCxHandle:       uCX API handle
 * @param      channel_list:     
 * @param      channel_list_len: length of channel_list
 * @return                       0 on success, negative value on error.
 */
int32_t uCxWifiSetChannelList(uCxHandle_t * puCxHandle, const int16_t * channel_list, int32_t channel_list_len);

/**
 * Read the current channel list
 * 
 * Output AT command:
 * > AT+UWCL?
 *
 * @param[in]  puCxHandle:   uCX API handle
 * @param[out] pChannelList: 
 * @return                   0 on success, negative value on error.
 */
int32_t uCxWifiGetChannelList(uCxHandle_t * puCxHandle, uIntList_t * pChannelList);

/**
 * Get the active channels that are used for scan and connection
 * 
 * Output AT command:
 * > AT+UWAC?
 *
 * @param[in]  puCxHandle:   uCX API handle
 * @param[out] pChannelList: 
 * @return                   0 on success, negative value on error.
 */
int32_t uCxWifiGetActiveChannels(uCxHandle_t * puCxHandle, uIntList_t * pChannelList);

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
void uCxWifiStationScan1Begin(uCxHandle_t * puCxHandle, uWifiScanMode_t scan_mode);

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
void uCxWifiStationScan2Begin(uCxHandle_t * puCxHandle, uWifiScanMode_t scan_mode, const char * ssid);

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
 * > AT+UWSST=<status_id>
 *
 * @param[in]  puCxHandle:            uCX API handle
 * @param      status_id:             
 * @param[out] pWifiStationStatusRsp: Please see \ref uCxWifiStationStatus_t
 * @return                            true on success, false on error (error code will be returned by uCxEnd()).
 *
 * NOTES:
 * Must be terminated by calling uCxEnd()
 */
bool uCxWifiStationStatusBegin(uCxHandle_t * puCxHandle, uWifiStatusId_t status_id, uCxWifiStationStatus_t * pWifiStationStatusRsp);

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
int32_t uCxWifiApSetConnectionParams2(uCxHandle_t * puCxHandle, const char * ssid, uWifiChannel_t channel);

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
int32_t uCxWifiApSetSecurityWpa2(uCxHandle_t * puCxHandle, const char * passphrase, uWifiWpaVersion_t wpa_version);

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
 * > AT+UWAPNST=<net_status_id>
 *
 * @param[in]  puCxHandle:    uCX API handle
 * @param      net_status_id: 
 * @param[out] pNetStatusVal: IP address
 * @return                    0 on success, negative value on error.
 */
int32_t uCxWifiApGetNetworkStatus(uCxHandle_t * puCxHandle, uWifiNetStatusId_t net_status_id, uSockIpAddress_t * pNetStatusVal);

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
 * Set Wi-Fi station roaming enabled or disabled.
 * 
 * Notes:
 * Can be stored using AT&W.
 * 
 * Output AT command:
 * > AT+UWSROE=<roaming>
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      roaming:    
 * @return                 0 on success, negative value on error.
 */
int32_t uCxWifiSetWifiRoaming(uCxHandle_t * puCxHandle, uWifiRoaming_t roaming);

/**
 * Get Wi-Fi station roaming enabled or disabled
 * 
 * Output AT command:
 * > AT+UWSROE?
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param[out] pRoaming:   
 * @return                 0 on success, negative value on error.
 */
int32_t uCxWifiGetWifiRoaming(uCxHandle_t * puCxHandle, uWifiRoaming_t * pRoaming);

/**
 * Write the threshold for when to start roaming background scanning, in dBm.
 * 
 * Notes:
 * Can be stored using AT&W.
 * 
 * Output AT command:
 * > AT+UWSROS0=<roaming_scanning_threshold>
 *
 * @param[in]  puCxHandle:                 uCX API handle
 * @param      roaming_scanning_threshold: Threshold in dBm where background scanning for roaming is started
 * @return                                 0 on success, negative value on error.
 */
int32_t uCxWifiStationSetRoamingBGScanThreshold(uCxHandle_t * puCxHandle, int32_t roaming_scanning_threshold);

/**
 * Read Connection Interval minium.
 * 
 * Output AT command:
 * > AT+UWSROS0?
 *
 * @param[in]  puCxHandle:                uCX API handle
 * @param[out] pRoamingScanningThreshold: Threshold in dBm where background scanning for roaming is started
 * @return                                0 on success, negative value on error.
 */
int32_t uCxWifiStationGetRoamingBGScanThreshold(uCxHandle_t * puCxHandle, int32_t * pRoamingScanningThreshold);

/**
 * Write the roaming switch limit.
 * 
 * Notes:
 * Can be stored using AT&W.
 * 
 * Output AT command:
 * > AT+UWSROS1=<roaming_switch_limit>
 *
 * @param[in]  puCxHandle:           uCX API handle
 * @param      roaming_switch_limit: Switch limit
 * @return                           0 on success, negative value on error.
 */
int32_t uCxWifiStationSetRoamingSwitchLimit(uCxHandle_t * puCxHandle, int32_t roaming_switch_limit);

/**
 * Read roaming switch limit.
 * 
 * Output AT command:
 * > AT+UWSROS1?
 *
 * @param[in]  puCxHandle:          uCX API handle
 * @param[out] pRoamingSwitchLimit: Switch limit
 * @return                          0 on success, negative value on error.
 */
int32_t uCxWifiStationGetRoamingSwitchLimit(uCxHandle_t * puCxHandle, int32_t * pRoamingSwitchLimit);

/**
 * Write the interval in milliseconds to trigger roaming background scan.
 * 
 * Notes:
 * Can be stored using AT&W.
 * 
 * Output AT command:
 * > AT+UWSROS2=<roaming_scan_interval>
 *
 * @param[in]  puCxHandle:            uCX API handle
 * @param      roaming_scan_interval: Scan interval
 * @return                            0 on success, negative value on error.
 */
int32_t uCxWifiStationSetRoamingScanInterval(uCxHandle_t * puCxHandle, int32_t roaming_scan_interval);

/**
 * Read roaming scan interval
 * 
 * Output AT command:
 * > AT+UWSROS2?
 *
 * @param[in]  puCxHandle:           uCX API handle
 * @param[out] pRoamingScanInterval: Scan interval
 * @return                           0 on success, negative value on error.
 */
int32_t uCxWifiStationGetRoamingScanInterval(uCxHandle_t * puCxHandle, int32_t * pRoamingScanInterval);

/**
 * Set aggressive roaming to enabled or disabled. When this option is disabled (default), roaming will perform a connection
 * handover if an newly found Access Point has an RSSI of <roaming_scanning_threshold> + <roaming_switch_limit> dB or
 * better. When this option is enabled, roaming will perform a connection handover if an newly found Access Point has an
 * RSSI that is at least <roaming_switch_limit> dB above its current value.
 * 
 * Notes:
 * Can be stored using AT&W.
 * 
 * Output AT command:
 * > AT+UWSROS3=<roaming_aggressive>
 *
 * @param[in]  puCxHandle:         uCX API handle
 * @param      roaming_aggressive: 
 * @return                         0 on success, negative value on error.
 */
int32_t uCxWifiStationSetAggressiveRoaming(uCxHandle_t * puCxHandle, uWifiRoamingAggressive_t roaming_aggressive);

/**
 * Get aggressive roaming enabled or disabled.
 * 
 * Output AT command:
 * > AT+UWSROS3?
 *
 * @param[in]  puCxHandle:         uCX API handle
 * @param[out] pRoamingAggressive: 
 * @return                         0 on success, negative value on error.
 */
int32_t uCxWifiStationGetAggressiveRoaming(uCxHandle_t * puCxHandle, uWifiRoamingAggressive_t * pRoamingAggressive);

/**
 * Write the delay time roaming waits before performing handover.
 * 
 * Notes:
 * Can be stored using AT&W.
 * 
 * Output AT command:
 * > AT+UWSROS4=<roaming_delay_time>
 *
 * @param[in]  puCxHandle:         uCX API handle
 * @param      roaming_delay_time: Roaming delay
 * @return                         0 on success, negative value on error.
 */
int32_t uCxWifiStationSetRoamingDelayMs(uCxHandle_t * puCxHandle, int32_t roaming_delay_time);

/**
 * Read roaming delay time.
 * 
 * Output AT command:
 * > AT+UWSROS4?
 *
 * @param[in]  puCxHandle:        uCX API handle
 * @param[out] pRoamingDelayTime: Roaming delay
 * @return                        0 on success, negative value on error.
 */
int32_t uCxWifiStationGetRoamingDelayMs(uCxHandle_t * puCxHandle, int32_t * pRoamingDelayTime);

/**
 * Set whether to perform roaming on all channels. If set to false the wifi station will only scan the channel used in the
 * current connection. If all access points uses the same channel, roaming on this channel only will give a much faster and
 * less intrusive roaming.
 * 
 * Notes:
 * Can be stored using AT&W.
 * 
 * Output AT command:
 * > AT+UWSROS5=<roaming_channels>
 *
 * @param[in]  puCxHandle:       uCX API handle
 * @param      roaming_channels: Roaming on all channels or current channel
 * @return                       0 on success, negative value on error.
 */
int32_t uCxWifiStationSetRoamingAllChannels(uCxHandle_t * puCxHandle, uWifiRoamingChannels_t roaming_channels);

/**
 * Read whether to perform roaming on all channels.
 * 
 * Output AT command:
 * > AT+UWSROS5?
 *
 * @param[in]  puCxHandle:       uCX API handle
 * @param[out] pRoamingChannels: Roaming on all channels or current channel
 * @return                       0 on success, negative value on error.
 */
int32_t uCxWifiStationGetRoamingAllChannels(uCxHandle_t * puCxHandle, uWifiRoamingChannels_t * pRoamingChannels);

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
 * Register StationRoamingSwitchInitiated event callback
 * 
 * This event is sent during Wi-Fi Station Roaming when AP switch is initiated
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      callback:   callback to register. Set to NULL to unregister.
 */
void uCxWifiRegisterStationRoamingSwitchInitiated(uCxHandle_t * puCxHandle, uUEWSRSI_t callback);

/**
 * Register StationRoamingSwitchFailed event callback
 * 
 * This event is sent during Wi-Fi Station Roaming when the switch to the new AP failed. After this event the module will
 * try to reconnect to the configured SSID
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      callback:   callback to register. Set to NULL to unregister.
 */
void uCxWifiRegisterStationRoamingSwitchFailed(uCxHandle_t * puCxHandle, uUEWSRSF_t callback);

/**
 * Register StationRoamingSwitchCompleted event callback
 * 
 * This event is sent during Wi-Fi Station Roaming when AP switch is completed
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      callback:   callback to register. Set to NULL to unregister.
 */
void uCxWifiRegisterStationRoamingSwitchCompleted(uCxHandle_t * puCxHandle, uUEWSRSC_t callback);

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