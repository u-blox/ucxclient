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
#ifndef _U_CX_SPS_H_
#define _U_CX_SPS_H_

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
 * SPS connect on connected Bluetooth device
 * 
 * Output AT command:
 * > AT+USPSC=<conn_handle>
 *
 * @param[in]  puCxHandle:  uCX API handle
 * @param      conn_handle: Connection handle of remote peer
 * @return                  0 on success, negative value on error.
 */
int32_t uCxSpsConnect1(uCxHandle_t * puCxHandle, int32_t conn_handle);

/**
 * SPS connect on connected Bluetooth device
 * 
 * Output AT command:
 * > AT+USPSC=<conn_handle>,<flow_control>
 *
 * @param[in]  puCxHandle:   uCX API handle
 * @param      conn_handle:  Connection handle of remote peer
 * @param      flow_control: Flow control: 0 - no flow control, 1 - flow control
 * @return                   0 on success, negative value on error.
 */
int32_t uCxSpsConnect2(uCxHandle_t * puCxHandle, int32_t conn_handle, int32_t flow_control);

/**
 * Enables or disable the SPS Service.
 * 
 * Notes:
 * Can be stored using AT&W.
 * 
 * Output AT command:
 * > AT+USPS=<service_option>
 *
 * @param[in]  puCxHandle:     uCX API handle
 * @param      service_option: 
 * @return                     0 on success, negative value on error.
 */
int32_t uCxSpsSetServiceEnable(uCxHandle_t * puCxHandle, uSpsServiceOption_t service_option);

/**
 * Read if the SPS service is enabled or disabled.
 * 
 * Output AT command:
 * > AT+USPS?
 *
 * @param[in]  puCxHandle:     uCX API handle
 * @param[out] pServiceOption: 
 * @return                     0 on success, negative value on error.
 */
int32_t uCxSpsGetServiceEnable(uCxHandle_t * puCxHandle, uSpsServiceOption_t * pServiceOption);

/**
 * Writes the specified amount of data to the specified SPS connection in binary mode. Max 1000 bytes.
 * 
 * Output AT command:
 * > AT+USPSWB=<conn_handle>,<binary_data>,<binary_data_len>
 *
 * @param[in]  puCxHandle:      uCX API handle
 * @param      conn_handle:     Connection handle of remote peer which has SPS enabled
 * @param      binary_data:     The data to write.
 * @param      binary_data_len: length of binary_data
 * @return                      Negative value on error. On success:
 *                              Data length that was written.
 */
int32_t uCxSpsWrite(uCxHandle_t * puCxHandle, int32_t conn_handle, const uint8_t * binary_data, int32_t binary_data_len);

/**
 * Set the mode in which to receive SPS data in AT mode.
 * 
 * Notes:
 * Can be stored using AT&W.
 * 
 * Output AT command:
 * > AT+USPSRM=<read_mode>
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      read_mode:  Modes to read data in AT
 * @return                 0 on success, negative value on error.
 */
int32_t uCxSpsSetDataMode(uCxHandle_t * puCxHandle, uReadMode_t read_mode);

/**
 * Read mode set.
 * 
 * Output AT command:
 * > AT+USPSRM?
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param[out] pReadMode:  Modes to read data in AT
 * @return                 0 on success, negative value on error.
 */
int32_t uCxSpsGetDataMode(uCxHandle_t * puCxHandle, uReadMode_t * pReadMode);

/**
 * Reads the specified amount of data from the specified connection handle in binary mode.
 * 
 * Output AT command:
 * > AT+USPSRB=<conn_handle>,<length>
 *
 * @param[in]  puCxHandle:  uCX API handle
 * @param      conn_handle: Connection handle of remote peer
 * @param      length:      Data bytes to read.
 * @param[out] pDataBuf:    Output data buffer
 * @return                  Number of bytes read or negative value on error.
 */
int32_t uCxSpsRead(uCxHandle_t * puCxHandle, int32_t conn_handle, int32_t length, uint8_t * pDataBuf);

/**
 * Register Connect event callback
 * 
 * Event response for SPS Connect. Upon a successful SPS connection, conn_handle will contain the connection handle of the
 * remote peer.
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      callback:   callback to register. Set to NULL to unregister.
 */
void uCxSpsRegisterConnect(uCxHandle_t * puCxHandle, uUESPSC_t callback);

/**
 * Register Disconnect event callback
 * 
 * Event response for SPS Connect. Upon a SPS disconnection, conn_handle will contain the connection handle of the remote
 * peer.
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      callback:   callback to register. Set to NULL to unregister.
 */
void uCxSpsRegisterDisconnect(uCxHandle_t * puCxHandle, uUESPSDC_t callback);

/**
 * Register DataAvailable event callback
 * 
 * Unsolicited event containing the number of received bytes to read.
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      callback:   callback to register. Set to NULL to unregister.
 */
void uCxSpsRegisterDataAvailable(uCxHandle_t * puCxHandle, uUESPSDA_t callback);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _U_CX_SPS_H_ */