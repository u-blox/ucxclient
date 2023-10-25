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
#ifndef _U_CX_GATT_CLIENT_H_
#define _U_CX_GATT_CLIENT_H_

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
    int32_t conn_handle;  /**< Bluetooth Low Energy connection handle. */
    int32_t start_handle; /**< Service start handle. */
    int32_t end_handle;   /**< Service end handle. */
    uByteArray_t uuid;    /**< UUID of attribute. Either 16-bit or 128-bit. */
} uCxGattClientDiscoverPrimaryServices_t;

typedef struct
{
    int32_t conn_handle;  /**< Bluetooth Low Energy connection handle. */
    int32_t start_handle; /**< Service start handle. */
    int32_t end_handle;   /**< Service end handle. */
} uCxGattClientDiscoverPrimaryServicesByUuid_t;

typedef struct
{
    int32_t conn_handle;     /**< Bluetooth Low Energy connection handle. */
    int32_t attr_handle;     /**< Attribute handle of the characteristic */
    uByteArray_t properties; /**< Bit mask describing the properties of the characteristic */
    int32_t value_handle;    /**< Attribute handle of the characteristic value. */
    uByteArray_t uuid;       /**< UUID of attribute. Either 16-bit or 128-bit. */
} uCxGattClientDiscoverServiceChars_t;

typedef struct
{
    int32_t conn_handle; /**< Bluetooth Low Energy connection handle. */
    int32_t char_handle; /**< Characteristic handle. */
    int32_t desc_handle; /**< Descriptor handle. */
    uByteArray_t uuid;   /**< UUID of attribute. Either 16-bit or 128-bit. */
} uCxGattClientDiscoverCharDescriptors_t;

typedef struct
{
    int32_t conn_handle;   /**< Bluetooth Low Energy connection handle. */
    int32_t value_handle;  /**< Attribute handle of the characteristic value. */
    uByteArray_t hex_data; /**< Characteristic data in hexadecimal form. For example, 070809AABBCC */
} uCxGattClientRead_t;

typedef struct
{
    int32_t conn_handle;   /**< Bluetooth Low Energy connection handle. */
    int32_t value_handle;  /**< Attribute handle of the characteristic value. */
    uByteArray_t hex_data; /**< Characteristic data in hexadecimal form. For example, 070809AABBCC */
} uCxGattClientReadByUuid_t;


/* ------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * ---------------------------------------------------------- */

/**
 * Discover all primary services on the remote device.
 * 
 * Output AT command:
 * > AT+UBTGPSD=<conn_handle>
 *
 * @param[in]  puCxHandle:  uCX API handle
 * @param      conn_handle: Bluetooth Low Energy connection handle.
 */
void uCxBeginGattClientDiscoverPrimaryServices(uCxHandle_t * puCxHandle, int32_t conn_handle);

/**
 * 
 *
 * @param[in]  puCxHandle:                            uCX API handle
 * @param[out] pGattClientDiscoverPrimaryServicesRsp: Please see \ref uCxGattClientDiscoverPrimaryServices_t
 */
bool uCxGattClientDiscoverPrimaryServicesGetResponse(uCxHandle_t * puCxHandle, uCxGattClientDiscoverPrimaryServices_t * pGattClientDiscoverPrimaryServicesRsp);

/**
 * Start discovery.
 * 
 * Output AT command:
 * > AT+UBTGPSDU=<conn_handle>,<uuid>,<uuid_len>
 *
 * @param[in]  puCxHandle:  uCX API handle
 * @param      conn_handle: Bluetooth Low Energy connection handle.
 * @param      uuid:        UUID of attribute. Either 16-bit or 128-bit.
 * @param      uuid_len:    length of uuid
 */
void uCxBeginGattClientDiscoverPrimaryServicesByUuid(uCxHandle_t * puCxHandle, int32_t conn_handle, const uint8_t * uuid, int32_t uuid_len);

/**
 * 
 *
 * @param[in]  puCxHandle:                                  uCX API handle
 * @param[out] pGattClientDiscoverPrimaryServicesByUuidRsp: Please see \ref uCxGattClientDiscoverPrimaryServicesByUuid_t
 */
bool uCxGattClientDiscoverPrimaryServicesByUuidGetResponse(uCxHandle_t * puCxHandle, uCxGattClientDiscoverPrimaryServicesByUuid_t * pGattClientDiscoverPrimaryServicesByUuidRsp);

/**
 * Discover all characteristics of a service.
 * 
 * Output AT command:
 * > AT+UBTGSCD=<conn_handle>,<start>,<end>
 *
 * @param[in]  puCxHandle:  uCX API handle
 * @param      conn_handle: Bluetooth Low Energy connection handle.
 * @param      start:       Service start handle.
 * @param      end:         Service end handle.
 */
void uCxBeginGattClientDiscoverServiceChars(uCxHandle_t * puCxHandle, int32_t conn_handle, int32_t start, int32_t end);

/**
 * 
 *
 * @param[in]  puCxHandle:                         uCX API handle
 * @param[out] pGattClientDiscoverServiceCharsRsp: Please see \ref uCxGattClientDiscoverServiceChars_t
 */
bool uCxGattClientDiscoverServiceCharsGetResponse(uCxHandle_t * puCxHandle, uCxGattClientDiscoverServiceChars_t * pGattClientDiscoverServiceCharsRsp);

/**
 * Discover all descriptors of a characteristic.
 * 
 * Output AT command:
 * > AT+UBTGCDD=<conn_handle>,<value_handle>,<characteristic_end_handle>
 *
 * @param[in]  puCxHandle:                uCX API handle
 * @param      conn_handle:               Bluetooth Low Energy connection handle.
 * @param      value_handle:              Attribute handle of the characteristic value.
 * @param      characteristic_end_handle: End handle of characteristic to which descriptor discovery is being performed.
 */
void uCxBeginGattClientDiscoverCharDescriptors(uCxHandle_t * puCxHandle, int32_t conn_handle, int32_t value_handle, int32_t characteristic_end_handle);

/**
 * 
 *
 * @param[in]  puCxHandle:                            uCX API handle
 * @param[out] pGattClientDiscoverCharDescriptorsRsp: Please see \ref uCxGattClientDiscoverCharDescriptors_t
 */
bool uCxGattClientDiscoverCharDescriptorsGetResponse(uCxHandle_t * puCxHandle, uCxGattClientDiscoverCharDescriptors_t * pGattClientDiscoverCharDescriptorsRsp);

/**
 * Reads the characteristic; all bytes included.
 * 
 * Output AT command:
 * > AT+UBTGR=<conn_handle>,<value_handle>
 *
 * @param[in]  puCxHandle:         uCX API handle
 * @param      conn_handle:        Bluetooth Low Energy connection handle.
 * @param      value_handle:       Attribute handle of the characteristic value.
 * @param[out] pGattClientReadRsp: Please see \ref uCxGattClientRead_t
 */
bool uCxBeginGattClientRead(uCxHandle_t * puCxHandle, int32_t conn_handle, int32_t value_handle, uCxGattClientRead_t * pGattClientReadRsp);

/**
 * Read all the characteristics by UUID. It will read all the bytes in each characteristic.
 * 
 * Output AT command:
 * > AT+UBTGRU=<conn_handle>,<start>,<end>,<uuid>,<uuid_len>
 *
 * @param[in]  puCxHandle:               uCX API handle
 * @param      conn_handle:              Bluetooth Low Energy connection handle.
 * @param      start:                    Start handle.
 * @param      end:                      End handle.
 * @param      uuid:                     UUID of attribute. Either 16-bit or 128-bit.
 * @param      uuid_len:                 length of uuid
 * @param[out] pGattClientReadByUuidRsp: Please see \ref uCxGattClientReadByUuid_t
 */
bool uCxBeginGattClientReadByUuid(uCxHandle_t * puCxHandle, int32_t conn_handle, int32_t start, int32_t end, const uint8_t * uuid, int32_t uuid_len, uCxGattClientReadByUuid_t * pGattClientReadByUuidRsp);

/**
 * Write the characteristic value.
 * 
 * Output AT command:
 * > AT+UBTGW=<conn_handle>,<value_handle>,<hex_data>,<hex_data_len>
 *
 * @param[in]  puCxHandle:   uCX API handle
 * @param      conn_handle:  Bluetooth Low Energy connection handle.
 * @param      value_handle: Attribute handle of the characteristic value.
 * @param      hex_data:     Characteristic data in hexadecimal form. For example, 070809AABBCC
 * @param      hex_data_len: length of hex_data
 */
int32_t uCxGattClientWrite(uCxHandle_t * puCxHandle, int32_t conn_handle, int32_t value_handle, const uint8_t * hex_data, int32_t hex_data_len);

/**
 * Writes the client characteristic configuration.
 * 
 * Output AT command:
 * > AT+UBTGCCW=<conn_handle>,<desc_handle>,<config>
 *
 * @param[in]  puCxHandle:  uCX API handle
 * @param      conn_handle: Bluetooth Low Energy connection handle.
 * @param      desc_handle: Descriptor handle.
 * @param      config:      
 */
int32_t uCxGattClientConfigWrite(uCxHandle_t * puCxHandle, int32_t conn_handle, int32_t desc_handle, uConfig_t config);

/**
 * Write characteristic.
 * 
 * Output AT command:
 * > AT+UBTGWNR=<conn_handle>,<value_handle>,<hex_data>,<hex_data_len>
 *
 * @param[in]  puCxHandle:   uCX API handle
 * @param      conn_handle:  Bluetooth Low Energy connection handle.
 * @param      value_handle: Attribute handle of the characteristic value.
 * @param      hex_data:     Characteristic data in hexadecimal form. For example, 070809AABBCC
 * @param      hex_data_len: length of hex_data
 */
int32_t uCxGattClientWriteNoRsp(uCxHandle_t * puCxHandle, int32_t conn_handle, int32_t value_handle, const uint8_t * hex_data, int32_t hex_data_len);

/**
 * Write long characteristic.
 * 
 * Output AT command:
 * > AT+UBTGWL=<conn_handle>,<value_handle>,<hex_data>,<reliable>,<flag>,<offset>
 *
 * @param[in]  puCxHandle:   uCX API handle
 * @param      conn_handle:  Bluetooth Low Energy connection handle.
 * @param      value_handle: Attribute handle of the characteristic value.
 * @param      hex_data:     
 * @param      reliable:     
 * @param      flag:         
 * @param      offset:       
 */
int32_t uCxGattClientWriteLong(uCxHandle_t * puCxHandle, int32_t conn_handle, int32_t value_handle, const char * hex_data, uReliable_t reliable, uFlag_t flag, int32_t offset);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _U_CX_GATT_CLIENT_H_ */