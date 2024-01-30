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
#include "u_cx_security.h"

int32_t uCxSecurityCertificateRemove(uCxHandle_t * puCxHandle, uCertType_t cert_type, const char * name)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+USECR=", "ds", cert_type, name, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxSecurityCertificateRemoveAll(uCxHandle_t * puCxHandle)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+USECR", "", U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxSecurityCertificateUpload2(uCxHandle_t * puCxHandle, uCertType_t cert_type, const char * name, uint8_t * pWData, size_t wDataLen)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+USECUB=", "dsB", cert_type, name, pWData, wDataLen, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxSecurityCertificateUpload3(uCxHandle_t * puCxHandle, uCertType_t cert_type, const char * name, const char * password, uint8_t * pWData, size_t wDataLen)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+USECUB=", "dssB", cert_type, name, password, pWData, wDataLen, U_CX_AT_UTIL_PARAM_LAST);
}

void uCxSecurityListCertificatesBegin(uCxHandle_t * puCxHandle)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    uCxAtClientCmdBeginF(pAtClient, "AT+USECL?", "", U_CX_AT_UTIL_PARAM_LAST);
}

bool uCxSecurityListCertificatesGetNext(uCxHandle_t * puCxHandle, uCxSecurityListCertificates_t * pSecurityListCertificatesRsp)
{
    int32_t ret;
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+USECL:", NULL, NULL, "ds", &pSecurityListCertificatesRsp->cert_type, &pSecurityListCertificatesRsp->name, U_CX_AT_UTIL_PARAM_LAST);
    return ret >= 0;
}

void uCxSecurityListTlsExtensionsBegin(uCxHandle_t * puCxHandle)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    uCxAtClientCmdBeginF(pAtClient, "AT+USETE?", "", U_CX_AT_UTIL_PARAM_LAST);
}

bool uCxSecurityListTlsExtensionsGetNext(uCxHandle_t * puCxHandle, uCxSecurityListTlsExtensions_t * pSecurityListTlsExtensionsRsp)
{
    int32_t ret;
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+USETE:", NULL, NULL, "dd", &pSecurityListTlsExtensionsRsp->extension, &pSecurityListTlsExtensionsRsp->enabled, U_CX_AT_UTIL_PARAM_LAST);
    return ret >= 0;
}

int32_t uCxSecuritySetTlsServerNameIndication(uCxHandle_t * puCxHandle, uEnabled_t enabled)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+USETE0=", "d", enabled, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxSecurityGetTlsServerNameIndication(uCxHandle_t * puCxHandle, uEnabled_t * pEnabled)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+USETE0?", "", U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+USETE0:", NULL, NULL, "d", pEnabled, U_CX_AT_UTIL_PARAM_LAST);
    {
        // Always call uCxAtClientCmdEnd() even if any previous function failed
        int32_t endRet = uCxAtClientCmdEnd(pAtClient);
        if (ret >= 0) {
            ret = endRet;
        }
    }
    return ret;
}

int32_t uCxSecuritySetTlsHandshakeFrag(uCxHandle_t * puCxHandle, uEnabled_t enabled)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    return uCxAtClientExecSimpleCmdF(pAtClient, "AT+USETE1=", "d", enabled, U_CX_AT_UTIL_PARAM_LAST);
}

int32_t uCxSecurityGetTlsHandshakeFrag(uCxHandle_t * puCxHandle, uEnabled_t * pEnabled)
{
    uCxAtClient_t *pAtClient = puCxHandle->pAtClient;
    int32_t ret;
    uCxAtClientCmdBeginF(pAtClient, "AT+USETE1?", "", U_CX_AT_UTIL_PARAM_LAST);
    ret = uCxAtClientCmdGetRspParamsF(pAtClient, "+USETE1:", NULL, NULL, "d", pEnabled, U_CX_AT_UTIL_PARAM_LAST);
    {
        // Always call uCxAtClientCmdEnd() even if any previous function failed
        int32_t endRet = uCxAtClientCmdEnd(pAtClient);
        if (ret >= 0) {
            ret = endRet;
        }
    }
    return ret;
}
