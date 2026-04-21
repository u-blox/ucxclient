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
#ifndef _U_CX_CHANNEL_SOUNDING_H_
#define _U_CX_CHANNEL_SOUNDING_H_

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
 * Writes the Channel Sounding mode.
 * 
 * Notes:
 * Can be stored using AT&W.
 * 
 * Output AT command:
 * > AT+UBTCSM=<channel_sounding_mode>
 *
 * @param[in]  puCxHandle:            uCX API handle
 * @param      channel_sounding_mode: 
 * @return                            0 on success, negative value on error.
 */
int32_t uCxChannelSoundingSetChannelSoundingMode(uCxHandle_t * puCxHandle, uBtChannelSoundingMode_t channel_sounding_mode);

/**
 * Read current Channel Sounding mode.
 * 
 * Output AT command:
 * > AT+UBTCSM?
 *
 * @param[in]  puCxHandle:           uCX API handle
 * @param[out] pChannelSoundingMode: 
 * @return                           0 on success, negative value on error.
 */
int32_t uCxChannelSoundingGetChannelSoundingMode(uCxHandle_t * puCxHandle, uBtChannelSoundingMode_t * pChannelSoundingMode);

/**
 * Register CsStatus event callback
 * 
 * Unsolicited event reporting Channel Sounding state changes.
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      callback:   callback to register. Set to NULL to unregister.
 */
void uCxChannelSoundingRegisterCsStatus(uCxHandle_t * puCxHandle, uUEBTCSS_t callback);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _U_CX_CHANNEL_SOUNDING_H_ */