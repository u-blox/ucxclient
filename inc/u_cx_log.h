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

#ifndef U_CX_LOG_H
#define U_CX_LOG_H

#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>

#include "u_cx_at_config.h"

/* ----------------------------------------------------------------
 * COMPILE-TIME MACROS
 * -------------------------------------------------------------- */

/* Log channels (used as input for U_CX_LOG_BEGIN() and U_CX_LOG_LINE()) */
#define U_CX_LOG_CH_TX     U_CX_LOG_AT,        ANSI_CYN "[AT TX]"
#define U_CX_LOG_CH_RX     U_CX_LOG_AT,        ANSI_MAG "[AT RX]"
#define U_CX_LOG_CH_DBG    U_CX_LOG_DEBUG,     ANSI_RST "[DBG  ]"
#define U_CX_LOG_CH_WARN   U_CX_LOG_WARNING,   ANSI_YEL "[WARN ]"
#define U_CX_LOG_CH_ERROR  U_CX_LOG_ERROR,     ANSI_RED "[ERROR]"

/* Simple line logging printf style (\n will be added automatically) */
#ifdef _WIN32
/* Windows: Enable logging only for specific modules that are known to work */
#define U_CX_LOG_LINE(logCh, format, ...) \
    _U_CX_LOG_BEGIN_FMT(logCh, format ANSI_RST "\n", ##__VA_ARGS__)
#define U_CX_LOG_LINE_I(logCh, instance, format, ...) \
    _U_CX_LOG_BEGIN_I_FMT(logCh, instance, format ANSI_RST "\n", ##__VA_ARGS__)
#else
#define U_CX_LOG_LINE(logCh, format, ...) \
    _U_CX_LOG_BEGIN_FMT(logCh, format ANSI_RST "\n", ##__VA_ARGS__)
#define U_CX_LOG_LINE_I(logCh, instance, format, ...) \
    _U_CX_LOG_BEGIN_I_FMT(logCh, instance, format ANSI_RST "\n", ##__VA_ARGS__)
#endif

/* Log API for splitting up line in several U_CX_LOG() calls */
#ifdef _WIN32
/* Windows: Enable logging only for specific modules */
#define U_CX_LOG_BEGIN(logCh)              _U_CX_LOG_BEGIN_FMT(logCh, "")
#define U_CX_LOG_BEGIN_I(logCh, instance)  _U_CX_LOG_BEGIN_I_FMT(logCh, instance, "")
#define U_CX_LOG(logCh, format, ...)       _U_CX_LOG(logCh, format, ##__VA_ARGS__)
#else
#define U_CX_LOG_BEGIN(logCh)              _U_CX_LOG_BEGIN_FMT(logCh, "")
#define U_CX_LOG_BEGIN_I(logCh, instance)  _U_CX_LOG_BEGIN_I_FMT(logCh, instance, "")
#define U_CX_LOG(logCh, format, ...)       _U_CX_LOG(logCh, format, ##__VA_ARGS__)
#endif
#define U_CX_LOG_END(logCh)                _U_CX_LOG(logCh, ANSI_RST "\n")

/* ANSI color escape codes */
#if U_CX_LOG_USE_ANSI_COLOR
# define ANSI_RED "\33[0;31m"
# define ANSI_GRN "\33[0;32m"
# define ANSI_YEL "\33[0;33m"
# define ANSI_BLU "\33[0;34m"
# define ANSI_MAG "\33[0;35m"
# define ANSI_CYN "\33[0;36m"
# define ANSI_RST "\33[0m"
#else
# define ANSI_RED
# define ANSI_GRN
# define ANSI_YEL
# define ANSI_BLU
# define ANSI_MAG
# define ANSI_CYN
# define ANSI_RST
#endif

/* Internal defines - do not use! */
#define __U_CX_LOG_BEGIN_FMT(enabled, chText, format, ...)  \
    if (enabled && uCxLogIsEnabled()) {                     \
        uCxLogPrintTime();                                  \
        U_CX_PORT_PRINTF(chText " " format, ##__VA_ARGS__); \
    }
#define __U_CX_LOG_BEGIN_I_FMT(enabled, chText, instance, format, ...)  \
    if (enabled && uCxLogIsEnabled()) {                     \
        uCxLogPrintTime();                                  \
        U_CX_PORT_PRINTF(chText "[%d] " format, instance, ##__VA_ARGS__); \
    }
#define __U_CX_LOG(enabled, chText, format, ...) \
    if (enabled && uCxLogIsEnabled()) {          \
        U_CX_PORT_PRINTF(format, ##__VA_ARGS__); \
    }
/* MSVC workaround */
#define EXPAND(x) x
#define _U_CX_LOG_BEGIN_FMT(...) EXPAND(__U_CX_LOG_BEGIN_FMT(__VA_ARGS__))
#define _U_CX_LOG_BEGIN_I_FMT(...) EXPAND(__U_CX_LOG_BEGIN_I_FMT(__VA_ARGS__))
#define _U_CX_LOG(...) EXPAND(__U_CX_LOG(__VA_ARGS__))

/* ----------------------------------------------------------------
 * TYPES
 * -------------------------------------------------------------- */

/* ----------------------------------------------------------------
 * VARIABLES
 * -------------------------------------------------------------- */

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -------------------------------------------------------------- */

void uCxLogPrintTime(void);

/**
  * @brief Turn off all logging
  *
  * Please see note for uCxLogEnable()
  */
void uCxLogDisable(void);

/**
  * @brief Turn on logging (default)
  *
  * NOTE: Logging output is also controlled by the following config macros:
  *       - U_CX_LOG_AT
  *       - U_CX_LOG_WARNING
  *       - U_CX_LOG_DEBUG
  *       If all of these are set to 0 then uCxLogEnable(), uCxLogDisable() and
  *       uCxLogIsEnabled() will have no effect.
  */
void uCxLogEnable(void);

/**
  * @brief Check if logging is enabled at runtime
  *
  * Please see note for uCxLogEnable()
  *
  * @retval true if logging is enabled
  */
bool uCxLogIsEnabled(void);

#endif // U_CX_LOG_H
