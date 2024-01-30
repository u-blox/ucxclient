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
#ifndef _U_CX_GENERAL_H_
#define _U_CX_GENERAL_H_

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
 * Attention command.
 * 
 * Output AT command:
 * > AT
 *
 * @param[in]  puCxHandle: uCX API handle
 * @return                 0 on success, negative value on error.
 */
int32_t uCxGeneralAttention(uCxHandle_t * puCxHandle);

/**
 * Read manufacturer text string.
 * 
 * Output AT command:
 * > AT+GMI
 *
 * @param[in]  puCxHandle:     uCX API handle
 * @param[out] ppManufacturer: Manufacturer ("u-blox").
 * @return                     true on success, false on error (error code will be returned by uCxEnd()).
 *
 * NOTES:
 * Must be terminated by calling uCxEnd()
 */
bool uCxGeneralGetManufacturerIdentificationBegin(uCxHandle_t * puCxHandle, const char ** ppManufacturer);

/**
 * Read device model.
 * 
 * Output AT command:
 * > AT+GMM
 *
 * @param[in]  puCxHandle:    uCX API handle
 * @param[out] ppDeviceModel: Device model
 * @return                    true on success, false on error (error code will be returned by uCxEnd()).
 *
 * NOTES:
 * Must be terminated by calling uCxEnd()
 */
bool uCxGeneralGetDeviceModelIdentificationBegin(uCxHandle_t * puCxHandle, const char ** ppDeviceModel);

/**
 * Read software version.
 * 
 * Output AT command:
 * > AT+GMR
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param[out] ppVersion:  Version.
 * @return                 true on success, false on error (error code will be returned by uCxEnd()).
 *
 * NOTES:
 * Must be terminated by calling uCxEnd()
 */
bool uCxGeneralGetSoftwareVersionBegin(uCxHandle_t * puCxHandle, const char ** ppVersion);

/**
 * Read serial number.
 * 
 * Output AT command:
 * > AT+GSN
 *
 * @param[in]  puCxHandle:     uCX API handle
 * @param[out] ppSerialNumber: Serial number.
 * @return                     true on success, false on error (error code will be returned by uCxEnd()).
 *
 * NOTES:
 * Must be terminated by calling uCxEnd()
 */
bool uCxGeneralGetSerialNumberBegin(uCxHandle_t * puCxHandle, const char ** ppSerialNumber);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _U_CX_GENERAL_H_ */