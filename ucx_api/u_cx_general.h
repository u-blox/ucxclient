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

typedef struct
{
    const char * application_version; /**< Application version. */
    const char * unique_identifier;   /**< Unique identifier. */
} uCxGeneralGetIdentInfo_t;

typedef struct
{
    int32_t greeting_mode;
    const char * text;     /**< The greeting text.
                                Note: Can not be an empty string. */
} uCxGeneralGetGreetingText_t;


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

/**
 * Read identification information.
 * 
 * Output AT command:
 * > ATI9
 *
 * @param[in]  puCxHandle:              uCX API handle
 * @param[out] pGeneralGetIdentInfoRsp: Please see \ref uCxGeneralGetIdentInfo_t
 * @return                              true on success, false on error (error code will be returned by uCxEnd()).
 *
 * NOTES:
 * Must be terminated by calling uCxEnd()
 */
bool uCxGeneralGetIdentInfoBegin(uCxHandle_t * puCxHandle, uCxGeneralGetIdentInfo_t * pGeneralGetIdentInfoRsp);

/**
 * Read type code.
 * 
 * Output AT command:
 * > ATI0
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param[out] ppTypeCode: Type code for the module.
 * @return                 true on success, false on error (error code will be returned by uCxEnd()).
 *
 * NOTES:
 * Must be terminated by calling uCxEnd()
 */
bool uCxGeneralGetTypeCodeBegin(uCxHandle_t * puCxHandle, const char ** ppTypeCode);

/**
 * Set the greeting text and mode.
 * 
 * Notes:
 * Requires AT&W and a reboot before taking effect.
 * 
 * Output AT command:
 * > AT+CSGT=<greeting_mode>
 *
 * @param[in]  puCxHandle:    uCX API handle
 * @param      greeting_mode: 
 * @return                    0 on success, negative value on error.
 */
int32_t uCxGeneralSetGreetingText1(uCxHandle_t * puCxHandle, uGreetingMode_t greeting_mode);

/**
 * Set the greeting text and mode.
 * 
 * Notes:
 * Requires AT&W and a reboot before taking effect.
 * 
 * Output AT command:
 * > AT+CSGT=<greeting_mode>,<text>
 *
 * @param[in]  puCxHandle:    uCX API handle
 * @param      greeting_mode: 
 * @param      text:          The greeting text.
 *                            Note: Can not be an empty string.
 * @return                    0 on success, negative value on error.
 */
int32_t uCxGeneralSetGreetingText2(uCxHandle_t * puCxHandle, uGreetingMode_t greeting_mode, const char * text);

/**
 * Read the greeting text.
 * 
 * Output AT command:
 * > AT+CSGT?
 *
 * @param[in]  puCxHandle:                 uCX API handle
 * @param[out] pGeneralGetGreetingTextRsp: Please see \ref uCxGeneralGetGreetingText_t
 * @return                                 true on success, false on error (error code will be returned by uCxEnd()).
 *
 * NOTES:
 * Must be terminated by calling uCxEnd()
 */
bool uCxGeneralGetGreetingTextBegin(uCxHandle_t * puCxHandle, uCxGeneralGetGreetingText_t * pGeneralGetGreetingTextRsp);

/**
 * Register Startup event callback
 * 
 * Indicates the startup of the device and that it is ready to receive commands.
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      callback:   callback to register. Set to NULL to unregister.
 */
void uCxGeneralRegisterStartup(uCxHandle_t * puCxHandle, uSTARTUP_t callback);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _U_CX_GENERAL_H_ */