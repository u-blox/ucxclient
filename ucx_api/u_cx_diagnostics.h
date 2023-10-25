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
#ifndef _U_CX_DIAGNOSTICS_H_
#define _U_CX_DIAGNOSTICS_H_

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


/* ------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * ---------------------------------------------------------- */

/**
 * Sends a ping command to a destination address every second, repeating it (count) times.
 * 
 * Output AT command:
 * > AT+UDGP=<destination>
 *
 * @param[in]  puCxHandle:  uCX API handle
 * @param      destination: Destination host to send a ping call to in the form of an IPv4 address (i.e. 192.168.1.10)
 *                          or hostname (i.e. www.u-blox.com).
 */
int32_t uCxDiagnosticsPing1(uCxHandle_t * puCxHandle, const char * destination);

/**
 * Sends a ping command to a destination address every second, repeating it (count) times.
 * 
 * Output AT command:
 * > AT+UDGP=<destination>,<count>
 *
 * @param[in]  puCxHandle:  uCX API handle
 * @param      destination: Destination host to send a ping call to in the form of an IPv4 address (i.e. 192.168.1.10)
 *                          or hostname (i.e. www.u-blox.com).
 * @param      count:       The number of pings (or packets) that will be transmitted. 0 means ping continuously.
 */
int32_t uCxDiagnosticsPing2(uCxHandle_t * puCxHandle, const char * destination, int32_t count);

/**
 * This command will stop any ping in progress.
 * Please note the command is asynchronous, and the ping will not be interrupted immediately,
 * but right after the next ping packet has a response, which might take a few seconds if the packet response times out.
 * This command always returns OK and does nothing if there is no ping in progress.
 * 
 * Output AT command:
 * > AT+UDGSP
 *
 * @param[in]  puCxHandle: uCX API handle
 */
int32_t uCxDiagnosticsPingStop(uCxHandle_t * puCxHandle);

/**
 * Start/stop IPERF 2 server/client
 * 
 * Output AT command:
 * > AT+UDGI=<iperf_action>,<protocol_type>
 *
 * @param[in]  puCxHandle:    uCX API handle
 * @param      iperf_action:  Action
 * @param      protocol_type: IP protocol
 */
int32_t uCxDiagnosticsIperf2(uCxHandle_t * puCxHandle, uIperfAction_t iperf_action, uProtocolType_t protocol_type);

/**
 * Start/stop IPERF 2 server/client
 * 
 * Output AT command:
 * > AT+UDGI=<iperf_action>,<protocol_type>,<role>,<port>,<report_interval>
 *
 * @param[in]  puCxHandle:      uCX API handle
 * @param      iperf_action:    Action
 * @param      protocol_type:   IP protocol
 * @param      role:            Role
 * @param      port:            Port
 * @param      report_interval: Report interval
 */
int32_t uCxDiagnosticsIperf5(uCxHandle_t * puCxHandle, uIperfAction_t iperf_action, uProtocolType_t protocol_type, uRole_t role, int32_t port, int32_t report_interval);

/**
 * Start/stop IPERF 2 server/client
 * 
 * Output AT command:
 * > AT+UDGI=<iperf_action>,<protocol_type>,<role>,<port>,<report_interval>,<time_boundary>,<ip_addr>
 *
 * @param[in]  puCxHandle:      uCX API handle
 * @param      iperf_action:    Action
 * @param      protocol_type:   IP protocol
 * @param      role:            Role
 * @param      port:            Port
 * @param      report_interval: Report interval
 * @param      time_boundary:   Time boundary. Client only. Ignored if role is server
 * @param      ip_addr:         IP address to connect to. Client only. Ignored if role is server
 */
int32_t uCxDiagnosticsIperf7(uCxHandle_t * puCxHandle, uIperfAction_t iperf_action, uProtocolType_t protocol_type, uRole_t role, int32_t port, int32_t report_interval, int32_t time_boundary, uSockIpAddress_t * ip_addr);

/**
 * Start/stop IPERF 2 server/client
 * 
 * Output AT command:
 * > AT+UDGI=<iperf_action>,<protocol_type>,<role>,<port>,<report_interval>,<time_boundary>,<ip_addr>,<length>
 *
 * @param[in]  puCxHandle:      uCX API handle
 * @param      iperf_action:    Action
 * @param      protocol_type:   IP protocol
 * @param      role:            Role
 * @param      port:            Port
 * @param      report_interval: Report interval
 * @param      time_boundary:   Time boundary. Client only. Ignored if role is server
 * @param      ip_addr:         IP address to connect to. Client only. Ignored if role is server
 * @param      length:          Size of packets.
 */
int32_t uCxDiagnosticsIperf8(uCxHandle_t * puCxHandle, uIperfAction_t iperf_action, uProtocolType_t protocol_type, uRole_t role, int32_t port, int32_t report_interval, int32_t time_boundary, uSockIpAddress_t * ip_addr, int32_t length);

/**
 * Start/stop IPERF 2 server/client
 * 
 * Output AT command:
 * > AT+UDGI=<iperf_action>,<protocol_type>,<role>,<port>,<report_interval>,<time_boundary>,<ip_addr>,<length>,<bandwidth>
 *
 * @param[in]  puCxHandle:      uCX API handle
 * @param      iperf_action:    Action
 * @param      protocol_type:   IP protocol
 * @param      role:            Role
 * @param      port:            Port
 * @param      report_interval: Report interval
 * @param      time_boundary:   Time boundary. Client only. Ignored if role is server
 * @param      ip_addr:         IP address to connect to. Client only. Ignored if role is server
 * @param      length:          Size of packets.
 * @param      bandwidth:       Bandwidth to be used for UDP
 */
int32_t uCxDiagnosticsIperf9(uCxHandle_t * puCxHandle, uIperfAction_t iperf_action, uProtocolType_t protocol_type, uRole_t role, int32_t port, int32_t report_interval, int32_t time_boundary, uSockIpAddress_t * ip_addr, int32_t length, int32_t bandwidth);

/**
 * Start/stop IPERF 2 server/client
 * 
 * Output AT command:
 * > AT+UDGI=<iperf_action>,<protocol_type>,<role>,<port>,<report_interval>,<time_boundary>,<ip_addr>,<length>,<bandwidth>,
 * <bidirectional>
 *
 * @param[in]  puCxHandle:      uCX API handle
 * @param      iperf_action:    Action
 * @param      protocol_type:   IP protocol
 * @param      role:            Role
 * @param      port:            Port
 * @param      report_interval: Report interval
 * @param      time_boundary:   Time boundary. Client only. Ignored if role is server
 * @param      ip_addr:         IP address to connect to. Client only. Ignored if role is server
 * @param      length:          Size of packets.
 * @param      bandwidth:       Bandwidth to be used for UDP
 * @param      bidirectional:   Bidirectional flag. Client only. Ignored if role is server
 */
int32_t uCxDiagnosticsIperf10(uCxHandle_t * puCxHandle, uIperfAction_t iperf_action, uProtocolType_t protocol_type, uRole_t role, int32_t port, int32_t report_interval, int32_t time_boundary, uSockIpAddress_t * ip_addr, int32_t length, int32_t bandwidth, uBidirectional_t bidirectional);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _U_CX_DIAGNOSTICS_H_ */