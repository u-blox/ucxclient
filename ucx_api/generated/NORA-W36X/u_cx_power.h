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
 * Set Power Save Level.
 * 
 * Output AT command:
 * > AT+UPMPSL=<level>
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      level:      Power save level
 * @return                 0 on success, negative value on error.
 */
int32_t uCxPowerSetPowerSaveLevel(uCxHandle_t * puCxHandle, int32_t level);

/**
 * Read Power Save Level.
 * 
 * Output AT command:
 * > AT+UPMPSL?
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param[out] pLevel:     Power save level
 * @return                 0 on success, negative value on error.
 */
int32_t uCxPowerGetPowerSaveLevel(uCxHandle_t * puCxHandle, int32_t * pLevel);

/**
 * Set Power Save active state timeout in milli second .
 * 
 * Output AT command:
 * > AT+UPMPSTO=<timeoutMs>
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      timeoutMs:  Active state timeout ms
 * @return                 0 on success, negative value on error.
 */
int32_t uCxPowerSetPowerSaveTimeout(uCxHandle_t * puCxHandle, int32_t timeoutMs);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _U_CX_POWER_H_ */