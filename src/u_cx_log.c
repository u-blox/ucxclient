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
static bool gUCxLogTimestampEnabled = false;  // Timestamps disabled by default for cleaner output

/* ----------------------------------------------------------------
 * STATIC FUNCTIONS
 * -------------------------------------------------------------- */

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -------------------------------------------------------------- */

void uCxLogPrintTime(void)
{
#if U_CX_PORT_PRINTF
    int32_t timestamp_ms = U_CX_PORT_GET_TIME_MS();
    int32_t ms      = (int32_t) (timestamp_ms % 1000);
    int32_t seconds = (int32_t) (timestamp_ms / 1000) % 60 ;
    int32_t minutes = (int32_t) ((timestamp_ms / (1000 * 60)) % 60);
    int32_t hours   = (int32_t) ((timestamp_ms / (1000 * 60 * 60)));
    U_CX_PORT_PRINTF("[%02" PRId32 ":%02" PRId32 ":%02" PRId32 ".%03" PRId32 "]",
                     hours, minutes, seconds, ms);
#endif
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

void uCxLogTimestampDisable(void)
{
    gUCxLogTimestampEnabled = false;
}

void uCxLogTimestampEnable(void)
{
    gUCxLogTimestampEnabled = true;
}

bool uCxLogTimestampIsEnabled(void)
{
    return gUCxLogTimestampEnabled;
}
