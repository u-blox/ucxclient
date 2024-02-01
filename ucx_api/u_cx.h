/** @file
 * @brief u-connectXpress API
 */

#ifndef U_CX_H
#define U_CX_H

#include <stdarg.h>
#include <stdint.h>

#include "u_cx_at_client.h"
#include "u_cx_types.h"

/* ----------------------------------------------------------------
 * COMPILE-TIME MACROS
 * -------------------------------------------------------------- */

/* ----------------------------------------------------------------
 * TYPES
 * -------------------------------------------------------------- */

typedef struct uCxHandle {
    uCxAtClient_t *pAtClient;
    uUrcCallbacks callbacks;
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


#endif // U_CX_H