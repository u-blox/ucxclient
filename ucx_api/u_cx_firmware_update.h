/*
 * Copyright 2024 u-blox
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _U_CX_FIRMWARE_UPDATE_H_
#define _U_CX_FIRMWARE_UPDATE_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "u_cx.h"
#include "u_cx_at_xmodem.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* ------------------------------------------------------------
 * TYPES
 * ---------------------------------------------------------- */

/**
 * Firmware update progress callback
 * 
 * @param totalBytes        Total bytes to transfer
 * @param bytesTransferred  Bytes transferred so far
 * @param percentComplete   Percentage complete (0-100)
 * @param pUserData         User data pointer
 */
typedef void (*uCxFirmwareUpdateProgress_t)(size_t totalBytes, 
                                            size_t bytesTransferred,
                                            int32_t percentComplete,
                                            void *pUserData);

/* ------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * ---------------------------------------------------------- */

/**
 * Update module firmware via XMODEM protocol
 * 
 * This is a convenience function that:
 * 1. Enters firmware update mode (if needed)
 * 2. Changes baudrate (if requested)
 * 3. Transfers firmware file via XMODEM
 * 4. Waits for update completion
 * 
 * Note: The module will reboot after successful firmware update.
 * You should close and reopen the connection after this function completes.
 * 
 * @param[in] puCxHandle      uCX API handle
 * @param[in] pFirmwareFile   Path to firmware file (.bin)
 * @param     baudRate        Baudrate to use for transfer (0 = keep current)
 *                            Common values: 115200, 230400, 460800, 921600
 * @param     progressCallback  Optional progress callback (NULL to disable)
 * @param     pUserData       User data pointer passed to callback
 * @return                    0 on success, negative value on error
 * 
 * Example:
 * @code
 * void progressCallback(size_t total, size_t transferred, int32_t percent, void *pUserData) {
 *     printf("Firmware update: %d%% (%zu/%zu bytes)\n", percent, transferred, total);
 * }
 * 
 * int32_t result = uCxFirmwareUpdate(puCxHandle, "firmware_v3.2.0.bin", 
 *                                    921600, progressCallback, NULL);
 * if (result == 0) {
 *     printf("Firmware updated successfully. Module will reboot.\n");
 * }
 * @endcode
 */
int32_t uCxFirmwareUpdate(uCxHandle_t *puCxHandle,
                          const char *pFirmwareFile,
                          int32_t baudRate,
                          uCxFirmwareUpdateProgress_t progressCallback,
                          void *pUserData);

/**
 * Update firmware using pre-loaded data buffer
 * 
 * Same as uCxFirmwareUpdate but uses a data buffer instead of reading from file.
 * 
 * @param[in] puCxHandle      uCX API handle
 * @param[in] pFirmwareData   Pointer to firmware data
 * @param     dataLen         Length of firmware data in bytes
 * @param     baudRate        Baudrate to use for transfer (0 = keep current)
 * @param     progressCallback  Optional progress callback (NULL to disable)
 * @param     pUserData       User data pointer passed to callback
 * @return                    0 on success, negative value on error
 */
int32_t uCxFirmwareUpdateFromData(uCxHandle_t *puCxHandle,
                                  const uint8_t *pFirmwareData,
                                  size_t dataLen,
                                  int32_t baudRate,
                                  uCxFirmwareUpdateProgress_t progressCallback,
                                  void *pUserData);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _U_CX_FIRMWARE_UPDATE_H_ */
