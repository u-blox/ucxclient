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
#ifndef _U_CX_NETWORK_TIME_H_
#define _U_CX_NETWORK_TIME_H_

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
    int32_t ntp_server_id;           /**< NTP server */
    const char * ntp_server_address; /**< NTP server address (Fully Qualified Domain name or IP address) */
    uSockIpAddress_t ntp_server_ip;  /**< NTP server IP address */
    int32_t reachable;               /**< NTP server reachability */
} uCxNtpGetNtpServer_t;


/* ------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * ---------------------------------------------------------- */

/**
 * Enable/disable NTP client
 * 
 * Notes:
 * Can be stored using AT&W.
 * 
 * Output AT command:
 * > AT+UNTE=<client_status>
 *
 * @param[in]  puCxHandle:    uCX API handle
 * @param      client_status: Enable/disable NTP client
 * @return                    0 on success, negative value on error.
 */
int32_t uCxNetworkTimeSetClientEnabled(uCxHandle_t * puCxHandle, uNtpClientStatus_t client_status);

/**
 * Read NTP client status
 * 
 * Output AT command:
 * > AT+UNTE?
 *
 * @param[in]  puCxHandle:    uCX API handle
 * @param[out] pClientStatus: Enable/disable NTP client
 * @return                    0 on success, negative value on error.
 */
int32_t uCxNetworkTimeGetClientEnabled(uCxHandle_t * puCxHandle, uNtpClientStatus_t * pClientStatus);

/**
 * Set NTP servers
 * 
 * Notes:
 * To remove this NTP server address, set it to empty string ("")
 * Can be stored using AT&W.
 * 
 * Output AT command:
 * > AT+UNTSC=<ntp_server_id>,<ntp_server_address>
 *
 * @param[in]  puCxHandle:         uCX API handle
 * @param      ntp_server_id:      NTP server
 * @param      ntp_server_address: NTP server address (Fully Qualified Domain name or IP address)
 * @return                         0 on success, negative value on error.
 */
int32_t uCxNetworkTimeSetNtpServer(uCxHandle_t * puCxHandle, int32_t ntp_server_id, const char * ntp_server_address);

/**
 * Read the information of NTP servers currently being used
 * 
 * Notes:
 * the NTP servers could come from DHCP offer or be configured via AT+UNTSC depending on the NTP client configuration and
 * network environment.
 * 
 * Output AT command:
 * > AT+UNTSC?
 *
 * @param[in]  puCxHandle:          uCX API handle
 * @param[out] pNtpGetNtpServerRsp: Please see \ref uCxNtpGetNtpServer_t
 * @return                          true on success, false on error (error code will be returned by uCxEnd()).
 *
 * NOTES:
 * Must be terminated by calling uCxEnd()
 */
bool uCxNetworkTimeGetNtpServerBegin(uCxHandle_t * puCxHandle, uCxNtpGetNtpServer_t * pNtpGetNtpServerRsp);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _U_CX_NETWORK_TIME_H_ */