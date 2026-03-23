/*
 * NORA-W36 mDNS ucxclient API — firmware v3.4.0+
 *
 * Official mDNS AT commands (chapter 13):
 *   AT+UMDNSE=<0|1>                              - Enable/disable responder
 *   AT+UMDNSHN=<hostname>                         - Set hostname (.local auto-appended)
 *   AT+UMDNSIP=<0|1|2>                            - IP version (v4/v6/dual)
 *   AT+UMDNSSA=<type>,<proto>,<port>[,<instance>] - Add service → returns <service_id>
 *   AT+UMDNSSR=<service_id>                       - Remove service
 *   AT+UMDNSSL?                                   - List services
 *   AT+UMDNSTRA=<service_id>,<key>,<value>        - Add TXT record
 *   AT+UMDNSTRR=<service_id>,<key>                - Remove TXT record
 *   AT+UMDNSTRL=<service_id>                      - List TXT records
 *   AT+UMDNSTRC=<service_id>                      - Clear all TXT records
 *
 * URCs:
 *   +UEMDNSU                                      - mDNS service up
 *   +UEMDNSD                                      - mDNS service down
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
 * ENUMERATORS
 * ---------------------------------------------------------- */

/** Transport protocol for AT+UMDNSSA */
typedef enum
{
    U_MDNS_PROTOCOL_TCP = 0,
    U_MDNS_PROTOCOL_UDP = 1,
} uCxMdnsProtocol_t;

/** IP version for AT+UMDNSIP */
typedef enum
{
    U_MDNS_IP_V4_ONLY   = 0,
    U_MDNS_IP_V6_ONLY   = 1,
    U_MDNS_IP_DUAL_STACK = 2,
} uCxMdnsIpVersion_t;

/* ------------------------------------------------------------
 * PUBLIC FUNCTIONS — responder control
 * ---------------------------------------------------------- */

/**
 * Enable or disable the mDNS responder.
 *
 * AT command: AT+UMDNSE=<enabled>
 *
 * @param[in]  puCxHandle  uCX API handle
 * @param[in]  enable      true = enable, false = disable
 * @return                 0 on success, negative on error.
 */
int32_t uCxMdnsEnable(uCxHandle_t * puCxHandle, bool enable);

/**
 * Set the mDNS hostname (".local" appended automatically by firmware).
 *
 * AT command: AT+UMDNSHN=<hostname>
 *
 * @param[in]  puCxHandle  uCX API handle
 * @param[in]  hostname    Hostname string (1..63 chars, no ".local" suffix)
 * @return                 0 on success, negative on error.
 */
int32_t uCxMdnsSetHostname(uCxHandle_t * puCxHandle, const char * hostname);

/**
 * Set IP version for the mDNS responder.
 *
 * AT command: AT+UMDNSIP=<ip_version>
 *
 * @param[in]  puCxHandle  uCX API handle
 * @param[in]  ipVersion   0=IPv4, 1=IPv6, 2=dual-stack (default)
 * @return                 0 on success, negative on error.
 */
int32_t uCxMdnsSetIpVersion(uCxHandle_t * puCxHandle, uCxMdnsIpVersion_t ipVersion);

/* ------------------------------------------------------------
 * PUBLIC FUNCTIONS — service management
 * ---------------------------------------------------------- */

/**
 * Add an mDNS/DNS-SD service. Returns the firmware-assigned service handle.
 *
 * AT command: AT+UMDNSSA=<service_type>,<protocol>,<port>[,<instance_name>]
 * Response:   +UMDNSSA:<service_id>
 *
 * @param[in]  puCxHandle    uCX API handle
 * @param[in]  serviceType   Service type (e.g. "_matterc"), leading underscore required
 * @param[in]  protocol      U_MDNS_PROTOCOL_TCP or U_MDNS_PROTOCOL_UDP
 * @param[in]  port          Port number (1-65535)
 * @param[in]  instanceName  Instance name (NULL to use hostname)
 * @param[out] pServiceId    Receives the firmware-assigned service handle (0..3)
 * @return                   0 on success, negative on error.
 */
int32_t uCxMdnsServiceAdd(uCxHandle_t * puCxHandle, const char * serviceType,
                           uCxMdnsProtocol_t protocol, int32_t port,
                           const char * instanceName, int32_t * pServiceId);

/**
 * Remove a registered mDNS service.
 *
 * AT command: AT+UMDNSSR=<service_id>
 *
 * @param[in]  puCxHandle  uCX API handle
 * @param[in]  serviceId   Service handle from uCxMdnsServiceAdd()
 * @return                 0 on success, negative on error.
 */
int32_t uCxMdnsServiceRemove(uCxHandle_t * puCxHandle, int32_t serviceId);

/* ------------------------------------------------------------
 * PUBLIC FUNCTIONS — TXT record management
 * ---------------------------------------------------------- */

/**
 * Add or update a TXT key=value record on a service.
 *
 * AT command: AT+UMDNSTRA=<service_id>,<key>,<value>
 *
 * @param[in]  puCxHandle  uCX API handle
 * @param[in]  serviceId   Service handle from uCxMdnsServiceAdd()
 * @param[in]  key         TXT key (e.g. "VP", "D", "SII") — max 64 chars
 * @param[in]  value       TXT value — max 64 chars (empty string for flags)
 * @return                 0 on success, negative on error.
 */
int32_t uCxMdnsTxtRecordAdd(uCxHandle_t * puCxHandle, int32_t serviceId,
                             const char * key, const char * value);

/**
 * Remove a specific TXT record from a service.
 *
 * AT command: AT+UMDNSTRR=<service_id>,<key>
 */
int32_t uCxMdnsTxtRecordRemove(uCxHandle_t * puCxHandle, int32_t serviceId,
                                const char * key);

/**
 * Clear all TXT records from a service.
 *
 * AT command: AT+UMDNSTRC=<service_id>
 */
int32_t uCxMdnsTxtRecordClear(uCxHandle_t * puCxHandle, int32_t serviceId);

/* ------------------------------------------------------------
 * BACKWARD COMPATIBILITY — old API wrappers (deprecated)
 * ---------------------------------------------------------- */

/** @deprecated Use uCxMdnsEnable(handle, true) */
static inline int32_t uCxMdnsStart(uCxHandle_t * h) { return uCxMdnsEnable(h, true); }
/** @deprecated Use uCxMdnsEnable(handle, false) */
static inline int32_t uCxMdnsStop(uCxHandle_t * h) { return uCxMdnsEnable(h, false); }

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _U_CX_MDNS_H_ */
