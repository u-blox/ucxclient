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
#include <string.h>
#include "u_cx_at_client.h"
#include "u_cx_diagnostics.h"

int32_t uCxDiagnosticsPing1(uCxHandle_t * puCxHandle, const char * destination)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UDGP=", "s", destination, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxDiagnosticsPing2(uCxHandle_t * puCxHandle, const char * destination, int32_t count)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UDGP=", "sd", destination, count, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxDiagnosticsPingStop(uCxHandle_t * puCxHandle)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UDGSP", "", U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxDiagnosticsIperf2(uCxHandle_t * puCxHandle, uIperfAction_t iperf_action, uProtocolType_t protocol_type)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UDGI=", "dd", iperf_action, protocol_type, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxDiagnosticsIperf5(uCxHandle_t * puCxHandle, uIperfAction_t iperf_action, uProtocolType_t protocol_type, uRole_t role, int32_t port, int32_t report_interval)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UDGI=", "ddddd", iperf_action, protocol_type, role, port, report_interval, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxDiagnosticsIperf7(uCxHandle_t * puCxHandle, uIperfAction_t iperf_action, uProtocolType_t protocol_type, uRole_t role, int32_t port, int32_t report_interval, int32_t time_boundary, uSockIpAddress_t * ip_addr)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UDGI=", "ddddddi", iperf_action, protocol_type, role, port, report_interval, time_boundary, ip_addr, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxDiagnosticsIperf8(uCxHandle_t * puCxHandle, uIperfAction_t iperf_action, uProtocolType_t protocol_type, uRole_t role, int32_t port, int32_t report_interval, int32_t time_boundary, uSockIpAddress_t * ip_addr, int32_t length)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UDGI=", "ddddddid", iperf_action, protocol_type, role, port, report_interval, time_boundary, ip_addr, length, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxDiagnosticsIperf9(uCxHandle_t * puCxHandle, uIperfAction_t iperf_action, uProtocolType_t protocol_type, uRole_t role, int32_t port, int32_t report_interval, int32_t time_boundary, uSockIpAddress_t * ip_addr, int32_t length, int32_t bandwidth)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UDGI=", "ddddddidd", iperf_action, protocol_type, role, port, report_interval, time_boundary, ip_addr, length, bandwidth, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxDiagnosticsIperf10(uCxHandle_t * puCxHandle, uIperfAction_t iperf_action, uProtocolType_t protocol_type, uRole_t role, int32_t port, int32_t report_interval, int32_t time_boundary, uSockIpAddress_t * ip_addr, int32_t length, int32_t bandwidth, uBidirectional_t bidirectional)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+UDGI=", "ddddddiddd", iperf_action, protocol_type, role, port, report_interval, time_boundary, ip_addr, length, bandwidth, bidirectional, U_CX_AT_UTIL_PARAM_LAST);
}

void uCxDiagnosticsRegisterPingComplete(uCxHandle_t * puCxHandle, uUEDGPC_t callback)
{
    puCxHandle->callbacks.UEDGPC = callback;
}

void uCxDiagnosticsRegisterPingResponse(uCxHandle_t * puCxHandle, uUEDGP_t callback)
{
    puCxHandle->callbacks.UEDGP = callback;
}
