/** @file
 * @brief u-connectXpress API
 */

#ifndef U_CX_H
#define U_CX_H

#include <stdarg.h>
#include <stdint.h>

#include "u_cx_at_client.h"
#include "u_cx_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ----------------------------------------------------------------
 * COMPILE-TIME MACROS
 * -------------------------------------------------------------- */

/* ----------------------------------------------------------------
 * TYPES
 * -------------------------------------------------------------- */

typedef struct uCxHandle {
    uCxAtClient_t *pAtClient;
    uUrcCallbacks callbacks;
    U_CX_MUTEX_HANDLE sessionMutex;  // Session-level mutex for multi-command sequences
} uCxHandle_t;

/* ----------------------------------------------------------------
 * VARIABLES
 * -------------------------------------------------------------- */

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -------------------------------------------------------------- */

/**
  * @brief Initialize the u-connectXpress API
  *
  * @param[in]  pAtClient:   AT client
  * @param[out] puCxHandle:  the output u-connectXpress API handle
  */
void uCxInit(uCxAtClient_t *pAtClient, uCxHandle_t *puCxHandle);

/**
  * @brief  End AT command started with any of the uCxXXXBegin() functions
  *
  * All functions named uCxXXXBegin() must be terminated by calling this function.
  * The function will wait for the AT status ("OK" or "ERROR").
  *
  * @param[in]  pClient:   the handle from uCxInit().
  * @retval                0 on status OK, -1 on status ERROR, negative value on error.
  */
int32_t uCxEnd(uCxHandle_t *puCxHandle);

/**
  * @brief  Lock the ucxclient session for thread-safe multi-command sequences
  *
  * Use this to protect sequences of AT commands that must execute atomically.
  * For example: socket create + bind, or any operation that must not be interleaved
  * with URC processing or other thread's commands.
  *
  * IMPORTANT: Always pair with uCxSessionUnlock(). The lock is re-entrant (same thread
  * can lock multiple times but must unlock the same number of times).
  *
  * @param[in]  puCxHandle:  the handle from uCxInit().
  */
void uCxSessionLock(uCxHandle_t *puCxHandle);

/**
  * @brief  Unlock the ucxclient session
  *
  * Must be called after uCxSessionLock() to release the lock.
  *
  * @param[in]  puCxHandle:  the handle from uCxInit().
  */
void uCxSessionUnlock(uCxHandle_t *puCxHandle);
#ifdef __cplusplus
}
#endif

#endif // U_CX_H
