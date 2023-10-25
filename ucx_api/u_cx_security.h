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
#ifndef _U_CX_SECURITY_H_
#define _U_CX_SECURITY_H_

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
    int32_t cert_type;
    const char * name;
} uCxSecurityListCertificates_t;

typedef struct
{
    int32_t extension;
    int32_t enabled;
} uCxSecurityListTlsExtensions_t;


/* ------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * ---------------------------------------------------------- */

/**
 * Remove X.509 certificates and private keys.
 * 
 * Output AT command:
 * > AT+USECR=<cert_type>,<name>
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      cert_type:  
 * @param      name:       
 */
int32_t uCxSecurityCertificateRemove(uCxHandle_t * puCxHandle, uCertType_t cert_type, const char * name);

/**
 * Remove X.509 certificates and private keys.
 * 
 * Output AT command:
 * > AT+USECR=<remove_all>
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      remove_all: 
 */
int32_t uCxSecurityCertificateRemoveAll(uCxHandle_t * puCxHandle, uRemoveAll_t remove_all);

/**
 * Write an X.509 certificate or private key using binary transfer.
 * 
 * Output AT command:
 * > AT+USECUB=<cert_type>,<name>
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      cert_type:  
 * @param      name:       
 * @param[in]  pWData:     binary data to write
 * @param      wDataLen:   number of bytes to write
 */
int32_t uCxSecurityUploadCertificate2(uCxHandle_t * puCxHandle, uCertType_t cert_type, const char * name, uint8_t * pWData, size_t wDataLen);

/**
 * Write an X.509 certificate or private key using binary transfer.
 * 
 * Output AT command:
 * > AT+USECUB=<cert_type>,<name>,<password>
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      cert_type:  
 * @param      name:       
 * @param      password:   Decryption password; applicable only for PKCS8 encrypted client private keys. The maximum
 *                         length is 64 characters.
 *                         NOTE: Supported Encryption method for private keys is AES only
 * @param[in]  pWData:     binary data to write
 * @param      wDataLen:   number of bytes to write
 */
int32_t uCxSecurityUploadCertificate3(uCxHandle_t * puCxHandle, uCertType_t cert_type, const char * name, const char * password, uint8_t * pWData, size_t wDataLen);

/**
 * Read all uploaded certificate names
 * 
 * Output AT command:
 * > AT+USECL?
 *
 * @param[in]  puCxHandle: uCX API handle
 */
void uCxBeginSecurityListCertificates(uCxHandle_t * puCxHandle);

/**
 * 
 *
 * @param[in]  puCxHandle:                   uCX API handle
 * @param[out] pSecurityListCertificatesRsp: Please see \ref uCxSecurityListCertificates_t
 */
bool uCxSecurityListCertificatesGetResponse(uCxHandle_t * puCxHandle, uCxSecurityListCertificates_t * pSecurityListCertificatesRsp);

/**
 * Read all TLS extension settings
 * 
 * Output AT command:
 * > AT+USETE?
 *
 * @param[in]  puCxHandle: uCX API handle
 */
void uCxBeginSecurityListTlsExtensions(uCxHandle_t * puCxHandle);

/**
 * 
 *
 * @param[in]  puCxHandle:                    uCX API handle
 * @param[out] pSecurityListTlsExtensionsRsp: Please see \ref uCxSecurityListTlsExtensions_t
 */
bool uCxSecurityListTlsExtensionsGetResponse(uCxHandle_t * puCxHandle, uCxSecurityListTlsExtensions_t * pSecurityListTlsExtensionsRsp);

/**
 * Turn Server Name Indication TLS extension on and off on a system level
 * 
 * Output AT command:
 * > AT+USETE0=<enabled>
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      enabled:    
 */
int32_t uCxSecuritySetTlsServerNameIndication(uCxHandle_t * puCxHandle, uEnabled_t enabled);

/**
 * Read Server Name Indication setting
 * 
 * Output AT command:
 * > AT+USETE0?
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param[out] pEnabled:   
 */
int32_t uCxSecurityGetTlsServerNameIndication(uCxHandle_t * puCxHandle, uEnabled_t * pEnabled);

/**
 * Turn Handshake Fragmentation TLS extension on and off on a system level
 * 
 * Output AT command:
 * > AT+USETE1=<enabled>
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      enabled:    
 */
int32_t uCxSecuritySetTlsHandshakeFrag(uCxHandle_t * puCxHandle, uEnabled_t enabled);

/**
 * Read Handshake Fragmentation setting
 * 
 * Output AT command:
 * > AT+USETE1?
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param[out] pEnabled:   
 */
int32_t uCxSecurityGetTlsHandshakeFrag(uCxHandle_t * puCxHandle, uEnabled_t * pEnabled);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _U_CX_SECURITY_H_ */