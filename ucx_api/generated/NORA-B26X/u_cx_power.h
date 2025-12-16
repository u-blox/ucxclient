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
#ifndef _U_CX_POWER_H_
#define _U_CX_POWER_H_

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
 * Enter Deep Sleep Mode with GPIO wakeup.
 * 
 * Output AT command:
 * > AT+UPMDS
 *
 * @param[in]  puCxHandle: uCX API handle
 * @return                 0 on success, negative value on error.
 */
int32_t uCxPowerDeepSleepWithGpioWakeup(uCxHandle_t * puCxHandle);

/**
 * Enter Deep Sleep Mode with specified wakeup mode.
 * 
 * Output AT command:
 * > AT+UPMDS=<wakeup_mode>
 *
 * @param[in]  puCxHandle:  uCX API handle
 * @param      wakeup_mode: Selects how to wake up from deep sleep.
 * @return                  0 on success, negative value on error.
 */
int32_t uCxPowerDeepSleep(uCxHandle_t * puCxHandle, uPowerWakeupMode_t wakeup_mode);

/**
 * Enters sleep mode. During this mode connection will be kept and the host continues to receive events from the module,
 * but the module will not respond to any AT commands.
 * On connection event, module will wake up, send URC, and go back to sleep.
 * To exit sleep mode, pull WAKE_UP GPIO low or send a character on the UART and wait 50ms.
 * 
 * Output AT command:
 * > AT+UPMS
 *
 * @param[in]  puCxHandle: uCX API handle
 * @return                 0 on success, negative value on error.
 */
int32_t uCxPowerSleep(uCxHandle_t * puCxHandle);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _U_CX_POWER_H_ */