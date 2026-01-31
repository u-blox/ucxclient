/*
 * Copyright 2025 u-blox
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
 * @brief Raspberry Pi Pico OS port implementation
 *
 * Supports RP2040 (Pico), RP2350 (Pico 2)
 */

#include "u_port_pico.h"
#include "pico/time.h"

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -------------------------------------------------------------- */

int32_t uPortMutexTryLock(critical_section_t *pMutex, uint32_t timeoutMs)
{
    // Pico SDK critical sections always succeed (they disable interrupts)
    // Just enter the critical section - it's a blocking operation
    // on single-core this is always immediate
    (void)timeoutMs;
    critical_section_enter_blocking(pMutex);
    return 0;
}
