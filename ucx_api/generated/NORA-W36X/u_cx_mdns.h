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
    int32_t service_id;         /**< Service handle returned by AT+UMDSA. Used to identify a registered service. */
    const char * service_type;  /**< Service type string, for example matterc, matterd or http.
                                     The leading underscore is part of the name as per RFC 6763 and must be included. */
    int32_t protocol;           /**< Transport protocol for the service. */
    int32_t port;               /**< Port number the service listens on. */
    const char * instance_name; /**< Instance name for the service. If omitted, the mDNS hostname is used.
                                     Maximum 63 characters per RFC 6763. */
} uCxMdnsServiceList_t;

typedef struct
{
    int32_t service_id;     /**< Service handle returned by AT+UMDSA. Used to identify a registered service. */
    const char * txt_key;   /**< TXT record key (e.g. "VP", "D", "SII"). */
    const char * txt_value; /**< TXT record value. */
} uCxMdnsTxtRecordList_t;


/* ------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * ---------------------------------------------------------- */

/**
 * Enable or disable the mDNS responder.
 * 
 * Notes:
 * Can be stored using AT&W.
 * 
 * Output AT command:
 * > AT+UMDE=<enabled>
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      enabled:    
 * @return                 0 on success, negative value on error.
 */
int32_t uCxMdnsSetEnabled(uCxHandle_t * puCxHandle, uEnabled_t enabled);

/**
 * Read the current mDNS responder state.
 * 
 * Output AT command:
 * > AT+UMDE?
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param[out] pEnabled:   
 * @return                 0 on success, negative value on error.
 */
int32_t uCxMdnsGetEnabled(uCxHandle_t * puCxHandle, uEnabled_t * pEnabled);

/**
 * Set the mDNS hostname.
 * 
 * Notes:
 * Can be stored using AT&W.
 * 
 * Output AT command:
 * > AT+UMDHN=<mdns_hostname>
 *
 * @param[in]  puCxHandle:    uCX API handle
 * @param      mdns_hostname: Hostname advertised via mDNS. The ".local" suffix is appended automatically.
 *                            If not set, defaults to the Wi-Fi hostname.
 * @return                    0 on success, negative value on error.
 */
int32_t uCxMdnsSetHostname(uCxHandle_t * puCxHandle, const char * mdns_hostname);

/**
 * Read the current mDNS hostname.
 * 
 * Output AT command:
 * > AT+UMDHN?
 *
 * @param[in]  puCxHandle:     uCX API handle
 * @param[out] ppMdnsHostname: Hostname advertised via mDNS. The ".local" suffix is appended automatically.
 *                             If not set, defaults to the Wi-Fi hostname.
 * @return                     true on success, false on error (error code will be returned by uCxEnd()).
 *
 * NOTES:
 * Must be terminated by calling uCxEnd()
 */
bool uCxMdnsGetHostnameBegin(uCxHandle_t * puCxHandle, const char ** ppMdnsHostname);

/**
 * Add a service to the mDNS responder.
 * 
 * Output AT command:
 * > AT+UMDSA=<service_type>,<protocol>,<port>
 *
 * @param[in]  puCxHandle:   uCX API handle
 * @param      service_type: Service type string, for example matterc, matterd or http.
 *                           The leading underscore is part of the name as per RFC 6763 and must be included.
 * @param      protocol:     Transport protocol for the service.
 * @param      port:         Port number the service listens on.
 * @param[out] pServiceId:   Service handle returned by AT+UMDSA. Used to identify a registered service.
 * @return                   0 on success, negative value on error.
 */
int32_t uCxMdnsServiceAdd3(uCxHandle_t * puCxHandle, const char * service_type, uMdnsProtocol_t protocol, int32_t port, int32_t * pServiceId);

/**
 * Add a service to the mDNS responder.
 * 
 * Output AT command:
 * > AT+UMDSA=<service_type>,<protocol>,<port>,<instance_name>
 *
 * @param[in]  puCxHandle:    uCX API handle
 * @param      service_type:  Service type string, for example matterc, matterd or http.
 *                            The leading underscore is part of the name as per RFC 6763 and must be included.
 * @param      protocol:      Transport protocol for the service.
 * @param      port:          Port number the service listens on.
 * @param      instance_name: Instance name for the service. If omitted, the mDNS hostname is used.
 *                            Maximum 63 characters per RFC 6763.
 * @param[out] pServiceId:    Service handle returned by AT+UMDSA. Used to identify a registered service.
 * @return                    0 on success, negative value on error.
 */
int32_t uCxMdnsServiceAdd4(uCxHandle_t * puCxHandle, const char * service_type, uMdnsProtocol_t protocol, int32_t port, const char * instance_name, int32_t * pServiceId);

/**
 * Remove the specified service.
 * 
 * Output AT command:
 * > AT+UMDSR=<service_id>
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      service_id: Service handle returned by AT+UMDSA. Used to identify a registered service.
 * @return                 0 on success, negative value on error.
 */
int32_t uCxMdnsServiceRemove(uCxHandle_t * puCxHandle, int32_t service_id);

/**
 * List all registered mDNS services.
 * 
 * Output AT command:
 * > AT+UMDSL?
 *
 * @param[in]  puCxHandle: uCX API handle
 * @return                 true on success, false on error (error code will be returned by uCxEnd()).
 *
 * NOTES:
 * Must be terminated by calling uCxEnd()
 */
void uCxMdnsServiceListBegin(uCxHandle_t * puCxHandle);

/**
 * 
 *
 * @param[in]  puCxHandle:          uCX API handle
 * @param[out] pMdnsServiceListRsp: Please see \ref uCxMdnsServiceList_t
 * @return                          true on success, false when there are no more entries or on error (uCxEnd() will return
 *                                  error code in this case).
 */
bool uCxMdnsServiceListGetNext(uCxHandle_t * puCxHandle, uCxMdnsServiceList_t * pMdnsServiceListRsp);

/**
 * Add or update a TXT record on the specified service.
 * 
 * Output AT command:
 * > AT+UMDTRA=<service_id>,<txt_key>,<txt_value>
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      service_id: Service handle returned by AT+UMDSA. Used to identify a registered service.
 * @param      txt_key:    TXT record key (e.g. "VP", "D", "SII").
 * @param      txt_value:  TXT record value.
 * @return                 0 on success, negative value on error.
 */
int32_t uCxMdnsTxtRecordAdd(uCxHandle_t * puCxHandle, int32_t service_id, const char * txt_key, const char * txt_value);

/**
 * Remove the TXT record with the specified key.
 * 
 * Output AT command:
 * > AT+UMDTRR=<service_id>,<txt_key>
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      service_id: Service handle returned by AT+UMDSA. Used to identify a registered service.
 * @param      txt_key:    TXT record key (e.g. "VP", "D", "SII").
 * @return                 0 on success, negative value on error.
 */
int32_t uCxMdnsTxtRecordRemove(uCxHandle_t * puCxHandle, int32_t service_id, const char * txt_key);

/**
 * List all TXT records for the specified service.
 * 
 * Output AT command:
 * > AT+UMDTRL=<service_id>
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      service_id: Service handle returned by AT+UMDSA. Used to identify a registered service.
 * @return                 true on success, false on error (error code will be returned by uCxEnd()).
 *
 * NOTES:
 * Must be terminated by calling uCxEnd()
 */
void uCxMdnsTxtRecordListBegin(uCxHandle_t * puCxHandle, int32_t service_id);

/**
 * 
 *
 * @param[in]  puCxHandle:            uCX API handle
 * @param[out] pMdnsTxtRecordListRsp: Please see \ref uCxMdnsTxtRecordList_t
 * @return                            true on success, false when there are no more entries or on error (uCxEnd() will return
 *                                    error code in this case).
 */
bool uCxMdnsTxtRecordListGetNext(uCxHandle_t * puCxHandle, uCxMdnsTxtRecordList_t * pMdnsTxtRecordListRsp);

/**
 * Clear all TXT records from the specified service.
 * 
 * Output AT command:
 * > AT+UMDTRC=<service_id>
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      service_id: Service handle returned by AT+UMDSA. Used to identify a registered service.
 * @return                 0 on success, negative value on error.
 */
int32_t uCxMdnsTxtRecordClear(uCxHandle_t * puCxHandle, int32_t service_id);

/**
 * Register MdnsUp event callback
 * 
 * Emitted when the mDNS responder has started.
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      callback:   callback to register. Set to NULL to unregister.
 */
void uCxMdnsRegisterMdnsUp(uCxHandle_t * puCxHandle, uUEMDU_t callback);

/**
 * Register MdnsDown event callback
 * 
 * Emitted when the mDNS responder has stopped.
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      callback:   callback to register. Set to NULL to unregister.
 */
void uCxMdnsRegisterMdnsDown(uCxHandle_t * puCxHandle, uUEMDD_t callback);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _U_CX_MDNS_H_ */