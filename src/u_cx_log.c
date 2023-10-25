/** @file
 * @brief Basic logging facility
 */

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <inttypes.h>

#include "u_cx_log.h"

/* ----------------------------------------------------------------
 * COMPILE-TIME MACROS
 * -------------------------------------------------------------- */

/* ----------------------------------------------------------------
 * TYPES
 * -------------------------------------------------------------- */

/* ----------------------------------------------------------------
 * STATIC PROTOTYPES
 * -------------------------------------------------------------- */

/* ----------------------------------------------------------------
 * STATIC VARIABLES
 * -------------------------------------------------------------- */
static bool gUCxLogEnabled = true;

/* ----------------------------------------------------------------
 * STATIC FUNCTIONS
 * -------------------------------------------------------------- */

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -------------------------------------------------------------- */

void uCxLogPrintTime(void)
{
    int32_t timestamp_ms = U_CX_PORT_GET_TIME_MS();
    int32_t ms      = (int32_t) (timestamp_ms % 1000);
    int32_t seconds = (int32_t) (timestamp_ms / 1000) % 60 ;
    int32_t minutes = (int32_t) ((timestamp_ms / (1000 * 60)) % 60);
    int32_t hours   = (int32_t) ((timestamp_ms / (1000 * 60 * 60)));
    U_CX_PORT_PRINTF("[%02" PRId32 ":%02" PRId32 ":%02" PRId32 ".%03" PRId32"]",
                     hours, minutes, seconds, ms);
}

void uCxLogDisable(void)
{
    gUCxLogEnabled = false;
}

void uCxLogEnable(void)
{
    gUCxLogEnabled = true;
}

bool uCxLogIsEnabled(void)
{
    return gUCxLogEnabled;
}
