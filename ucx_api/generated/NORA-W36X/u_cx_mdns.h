/*
* This file was manually created for NORA-W36 mDNS support.
* Will be auto-generated in future ucxclient versions.
*
* mDNS AT Commands:
* AT+UWMDNSS  - Start mDNS responder
* AT+UWMDNSP  - Stop mDNS responder
* AT+UWMDNSC  - Configure mDNS service
* AT+UWMDNSCL - Clear all services
*/
#ifndef _U_CX_MDNS_H_
#define _U_CX_MDNS_H_

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
    int32_t index;                /**< Service index (0-based) */
    const char * service_name;    /**< Service instance name (max 63 chars) */
    const char * service_type;    /**< Service type (e.g., "_matter._tcp") */
    int32_t port;                 /**< Port number (1-65535) */
} uCxMdnsServiceInfo_t;

/* ------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * ---------------------------------------------------------- */

/**
 * Start mDNS responder on NORA-W36.
 * 
 * Output AT command:
 * > AT+UWMDNSS
 *
 * @param[in]  puCxHandle: uCX API handle
 * @return                 0 on success, negative value on error.
 */
int32_t uCxMdnsStart(uCxHandle_t * puCxHandle);

/**
 * Stop mDNS responder on NORA-W36.
 * 
 * Output AT command:
 * > AT+UWMDNSP
 *
 * @param[in]  puCxHandle: uCX API handle
 * @return                 0 on success, negative value on error.
 */
int32_t uCxMdnsStop(uCxHandle_t * puCxHandle);

/**
 * Configure an mDNS service on NORA-W36.
 * 
 * Output AT command:
 * > AT+UWMDNSC="<service_name>","<service_type>",<port>
 *
 * @param[in]  puCxHandle:    uCX API handle
 * @param[in]  service_name:  Service instance name (e.g., "matter-default")
 * @param[in]  service_type:  Service type (e.g., "_matter._tcp" or "_matterc._udp")
 * @param[in]  port:          Port number (1-65535)
 * @return                    0 on success, negative value on error.
 *
 * Example:
 *   uCxMdnsConfigureService(handle, "matter-default", "_matter._tcp", 5540);
 */
int32_t uCxMdnsConfigureService(uCxHandle_t * puCxHandle, const char * service_name, 
                                 const char * service_type, int32_t port);

/**
 * Clear all configured mDNS services on NORA-W36.
 * 
 * Output AT command:
 * > AT+UWMDNSCL
 *
 * @param[in]  puCxHandle: uCX API handle
 * @return                 0 on success, negative value on error.
 *
 * Note: This stops mDNS if it's running.
 */
int32_t uCxMdnsClearServices(uCxHandle_t * puCxHandle);

/**
 * BEGIN section for listing configured services (read command).
 * Call uCxMdnsListServicesGetNext() to iterate through services.
 * 
 * Output AT command:
 * > AT+UWMDNSC?
 *
 * @param[in]  puCxHandle: uCX API handle
 * @return                 0 on success, negative value on error.
 */
int32_t uCxMdnsListServicesBegin(uCxHandle_t * puCxHandle);

/**
 * Get next service from the list started with uCxMdnsListServicesBegin().
 * 
 * Response format:
 * +UWMDNSC: <index>,"<service_name>","<service_type>",<port>
 *
 * @param[in]  puCxHandle:     uCX API handle
 * @param[out] pServiceInfo:   Pointer to service info structure to fill
 * @return                     0 on success, negative value on error or no more services.
 */
int32_t uCxMdnsListServicesGetNext(uCxHandle_t * puCxHandle, uCxMdnsServiceInfo_t * pServiceInfo);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _U_CX_MDNS_H_ */
