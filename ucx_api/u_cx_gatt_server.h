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
#ifndef _U_CX_GATT_SERVER_H_
#define _U_CX_GATT_SERVER_H_

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
    int32_t value_handle; /**< Added characteristic handle. */
    int32_t cccd_handle;  /**< CCCD characteristic handle. This value is zero if there is no CCCD. */
} uCxGattServerCharDefine_t;

typedef struct
{
    int32_t value_handle; /**< Value handle of the added characteristic. */
    int32_t cccd_handle;  /**< Client Characteristic Configuration Descriptor (CCCD) handle of the added characteristic.
                               This value is zero if there is no CCCD. */
} uCxGattServerHostCharDefine_t;


/* ------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * ---------------------------------------------------------- */

/**
 * Defines a service.
 * 
 * Output AT command:
 * > AT+UBTGS=<uuid>,<uuid_len>
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      uuid:       UUID of service. This can be either 16 bit or 128 bit.
 * @param      uuid_len:   length of uuid
 * @param[out] pSerHandle: Handle of the created service.
 */
int32_t uCxGattServerServiceDefine(uCxHandle_t * puCxHandle, const uint8_t * uuid, int32_t uuid_len, int32_t * pSerHandle);

/**
 * Create a new characteristic in the GATT table for a GATT server. The CCCD for the characteristic, if applicable, is
 * created here. Extended properties such as CPFD, CUDD, and SCCD are not supported.
 * 
 * Output AT command:
 * > AT+UBTGC=<uuid>,<uuid_len>,<properties>,<properties_len>,<security_read>,<security_write>,<value>,<value_len>
 *
 * @param[in]  puCxHandle:               uCX API handle
 * @param      uuid:                     UUID of characteristic. This can be either 16 bit or 128 bit.
 * @param      uuid_len:                 length of uuid
 * @param      properties:               Property value (a bit field):
 *                                       Broadcast: 0x01 - If set, it allows broadcasts of the Characteristic Value using
 *                                       Characteristic Configuration Descriptor.
 *                                       Read: 0x02 - If set, it allows reads of the Characteristic Value.
 *                                       Write Without Response: 0x04 - If set, it allows writing of the Characteristic Value
 *                                       without response.
 *                                       Write: 0x08 - If set, it allows writing of the Characteristic Value with response.
 *                                       Notify: 0x10 - If set, it allows notifications of a characteristic value.
 *                                       Indicate: 0x20 - If set, it allows indication of a characteristic value with
 *                                       acknowledgement.
 *                                       Authenticated Signed Writes: 0x40 - If set, it allows signed writes to the characteristic
 *                                       value.
 *                                       Reserved Bit: 0x80 - Do not use. Reserved for future use.
 * @param      properties_len:           length of properties
 * @param      security_read:            
 * @param      security_write:           
 * @param      value:                    Default characteristic value before any value is pushed to it. A characteristic value can
 *                                       be 244 bytes long.
 * @param      value_len:                length of value
 * @param[out] pGattServerCharDefineRsp: Please see \ref uCxGattServerCharDefine_t
 */
int32_t uCxGattServerCharDefine5(uCxHandle_t * puCxHandle, const uint8_t * uuid, int32_t uuid_len, const uint8_t * properties, int32_t properties_len, uSecurityRead_t security_read, uSecurityWrite_t security_write, const uint8_t * value, int32_t value_len, uCxGattServerCharDefine_t * pGattServerCharDefineRsp);

/**
 * Create a new characteristic in the GATT table for a GATT server. The CCCD for the characteristic, if applicable, is
 * created here. Extended properties such as CPFD, CUDD, and SCCD are not supported.
 * 
 * Output AT command:
 * > AT+UBTGC=<uuid>,<uuid_len>,<properties>,<properties_len>,<security_read>,<security_write>,<value>,<value_len>,<max_len
 * gth>
 *
 * @param[in]  puCxHandle:               uCX API handle
 * @param      uuid:                     UUID of characteristic. This can be either 16 bit or 128 bit.
 * @param      uuid_len:                 length of uuid
 * @param      properties:               Property value (a bit field):
 *                                       Broadcast: 0x01 - If set, it allows broadcasts of the Characteristic Value using
 *                                       Characteristic Configuration Descriptor.
 *                                       Read: 0x02 - If set, it allows reads of the Characteristic Value.
 *                                       Write Without Response: 0x04 - If set, it allows writing of the Characteristic Value
 *                                       without response.
 *                                       Write: 0x08 - If set, it allows writing of the Characteristic Value with response.
 *                                       Notify: 0x10 - If set, it allows notifications of a characteristic value.
 *                                       Indicate: 0x20 - If set, it allows indication of a characteristic value with
 *                                       acknowledgement.
 *                                       Authenticated Signed Writes: 0x40 - If set, it allows signed writes to the characteristic
 *                                       value.
 *                                       Reserved Bit: 0x80 - Do not use. Reserved for future use.
 * @param      properties_len:           length of properties
 * @param      security_read:            
 * @param      security_write:           
 * @param      value:                    Default characteristic value before any value is pushed to it. A characteristic value can
 *                                       be 244 bytes long.
 * @param      value_len:                length of value
 * @param      max_length:               Maximum length of the characteristic in bytes. The maximum value is 244 bytes.
 * @param[out] pGattServerCharDefineRsp: Please see \ref uCxGattServerCharDefine_t
 */
int32_t uCxGattServerCharDefine6(uCxHandle_t * puCxHandle, const uint8_t * uuid, int32_t uuid_len, const uint8_t * properties, int32_t properties_len, uSecurityRead_t security_read, uSecurityWrite_t security_write, const uint8_t * value, int32_t value_len, int32_t max_length, uCxGattServerCharDefine_t * pGattServerCharDefineRsp);

/**
 * Define a characteristic.
 * 
 * Output AT command:
 * > AT+UBTGHCC=<uuid>,<uuid_len>,<properties>,<properties_len>,<security_read>,<security_write>
 *
 * @param[in]  puCxHandle:                   uCX API handle
 * @param      uuid:                         UUID of characteristic. This can be either 16 bit or 128 bit.
 * @param      uuid_len:                     length of uuid
 * @param      properties:                   Property value (a bit field):
 *                                           Broadcast: 0x01 - If set, it allows broadcasts of the Characteristic Value using
 *                                           Characteristic Configuration Descriptor.
 *                                           Read: 0x02 - If set, it allows reads of the Characteristic Value.
 *                                           Write Without Response: 0x04 - If set, it allows writing of the Characteristic Value
 *                                           without response.
 *                                           Write: 0x08 - If set, it allows writing of the Characteristic Value with response.
 *                                           Notify: 0x10 - If set, it allows notifications of a characteristic value.
 *                                           Indicate: 0x20 - If set, it allows indication of a characteristic value with
 *                                           acknowledgement.
 *                                           Authenticated Signed Writes: 0x40 - If set, it allows signed writes to the characteristic
 *                                           value.
 *                                           Reserved Bit: 0x80 - Do not use. Reserved for future use.
 * @param      properties_len:               length of properties
 * @param      security_read:                
 * @param      security_write:               
 * @param[out] pGattServerHostCharDefineRsp: Please see \ref uCxGattServerHostCharDefine_t
 */
int32_t uCxGattServerHostCharDefine(uCxHandle_t * puCxHandle, const uint8_t * uuid, int32_t uuid_len, const uint8_t * properties, int32_t properties_len, uSecurityRead_t security_read, uSecurityWrite_t security_write, uCxGattServerHostCharDefine_t * pGattServerHostCharDefineRsp);

/**
 * Define descriptor.
 * 
 * Output AT command:
 * > AT+UBTGD=<uuid>,<uuid_len>,<security_read>,<security_write>,<value>,<value_len>
 *
 * @param[in]  puCxHandle:     uCX API handle
 * @param      uuid:           UUID of characteristic. This can be either 16 bit or 128 bit.
 * @param      uuid_len:       length of uuid
 * @param      security_read:  
 * @param      security_write: 
 * @param      value:          Descriptor value. This can be 23 bytes long.
 * @param      value_len:      length of value
 * @param[out] pDescHandle:    Handle of the created descriptor.
 */
int32_t uCxGattServerDescriptorDefine4(uCxHandle_t * puCxHandle, const uint8_t * uuid, int32_t uuid_len, uSecurityRead_t security_read, uSecurityWrite_t security_write, const uint8_t * value, int32_t value_len, int32_t * pDescHandle);

/**
 * Define descriptor.
 * 
 * Output AT command:
 * > AT+UBTGD=<uuid>,<uuid_len>,<security_read>,<security_write>,<value>,<value_len>,<max_length>
 *
 * @param[in]  puCxHandle:     uCX API handle
 * @param      uuid:           UUID of characteristic. This can be either 16 bit or 128 bit.
 * @param      uuid_len:       length of uuid
 * @param      security_read:  
 * @param      security_write: 
 * @param      value:          Descriptor value. This can be 23 bytes long.
 * @param      value_len:      length of value
 * @param      max_length:     Maximum length of the descriptor in bytes. The maximum value is 23 bytes.
 * @param[out] pDescHandle:    Handle of the created descriptor.
 */
int32_t uCxGattServerDescriptorDefine5(uCxHandle_t * puCxHandle, const uint8_t * uuid, int32_t uuid_len, uSecurityRead_t security_read, uSecurityWrite_t security_write, const uint8_t * value, int32_t value_len, int32_t max_length, int32_t * pDescHandle);

/**
 * Activate current defined service.
 * 
 * Output AT command:
 * > AT+UBTGSA
 *
 * @param[in]  puCxHandle: uCX API handle
 */
int32_t uCxGattServerServiceActivate(uCxHandle_t * puCxHandle);

/**
 * Responds to read request.
 * 
 * Output AT command:
 * > AT+UBTGRRR=<conn_handle>,<value>,<value_len>
 *
 * @param[in]  puCxHandle:  uCX API handle
 * @param      conn_handle: GAP handle of the connected device.
 * @param      value:       Characteristic value. This can be 244 bytes long.
 * @param      value_len:   length of value
 */
int32_t uCxGattServerReadReqRespond(uCxHandle_t * puCxHandle, int32_t conn_handle, const uint8_t * value, int32_t value_len);

/**
 * Send notification
 * 
 * Output AT command:
 * > AT+UBTGNS=<conn_handle>,<char_handle>,<value>,<value_len>
 *
 * @param[in]  puCxHandle:  uCX API handle
 * @param      conn_handle: GAP handle of the connected device.
 * @param      char_handle: Characteristic value handle.
 * @param      value:       Characteristic value. The maximum length is the current MTU size - 3.
 * @param      value_len:   length of value
 */
int32_t uCxGattServerSendNotification(uCxHandle_t * puCxHandle, int32_t conn_handle, int32_t char_handle, const uint8_t * value, int32_t value_len);

/**
 * Send notification
 * 
 * Output AT command:
 * > AT+UBTGIS=<conn_handle>,<char_handle>,<value>,<value_len>
 *
 * @param[in]  puCxHandle:  uCX API handle
 * @param      conn_handle: GAP handle of the connected device.
 * @param      char_handle: Characteristic value handle.
 * @param      value:       Characteristic value. The maximum length is the current MTU size - 3.
 * @param      value_len:   length of value
 */
int32_t uCxGattServerSendIndication(uCxHandle_t * puCxHandle, int32_t conn_handle, int32_t char_handle, const uint8_t * value, int32_t value_len);

/**
 * Set attribute value.
 * 
 * Output AT command:
 * > AT+UBTGAV=<attr_handle>,<value>,<value_len>
 *
 * @param[in]  puCxHandle:  uCX API handle
 * @param      attr_handle: Attribute handle.
 * @param      value:       Characterstic value. This can be 244 bytes long.
 * @param      value_len:   length of value
 */
int32_t uCxGattServerSetAttrValue(uCxHandle_t * puCxHandle, int32_t attr_handle, const uint8_t * value, int32_t value_len);

/**
 * Respond with error code.
 * 
 * Output AT command:
 * > AT+UBTGRRRE=<conn_handle>,<error_code>,<error_code_len>
 *
 * @param[in]  puCxHandle:     uCX API handle
 * @param      conn_handle:    GAP handle of connected device.
 * @param      error_code:     Application error code. Allowed value range: 0x80-0x9F
 * @param      error_code_len: length of error_code
 */
int32_t uCxGattServerReadReqRespondWithErr(uCxHandle_t * puCxHandle, int32_t conn_handle, const uint8_t * error_code, int32_t error_code_len);

/**
 * Respond with error code.
 * 
 * Output AT command:
 * > AT+UBTGWRE=<conn_handle>,<error_code>,<error_code_len>
 *
 * @param[in]  puCxHandle:     uCX API handle
 * @param      conn_handle:    GAP handle of connected device.
 * @param      error_code:     Application error code. Allowed value range: 0x80-0x9F
 * @param      error_code_len: length of error_code
 */
int32_t uCxGattServerWriteRespondWithErr(uCxHandle_t * puCxHandle, int32_t conn_handle, const uint8_t * error_code, int32_t error_code_len);

/**
 * Respond to write request.
 * 
 * Output AT command:
 * > AT+UBTGWRR=<conn_handle>
 *
 * @param[in]  puCxHandle:  uCX API handle
 * @param      conn_handle: GAP handle of connected device.
 */
int32_t uCxGattServerWriteRespond(uCxHandle_t * puCxHandle, int32_t conn_handle);

/**
 * Send Service Changed Indication.
 * 
 * Output AT command:
 * > AT+UBTGSCI=<conn_handle>,<start_handle>,<end_handle>
 *
 * @param[in]  puCxHandle:   uCX API handle
 * @param      conn_handle:  GAP handle of connected device.
 * @param      start_handle: Start of the affected attribute handle range.
 * @param      end_handle:   End of the affected attribute handle range.
 */
int32_t uCxGattServerSendServiceChangedInd(uCxHandle_t * puCxHandle, int32_t conn_handle, int32_t start_handle, int32_t end_handle);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _U_CX_GATT_SERVER_H_ */