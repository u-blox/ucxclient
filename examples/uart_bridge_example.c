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
 * @brief Transparent UART bridge between the module and the ST-Link VCP (STM32 only)
 *
 * Turns the STM32 board into a passthrough between a PC and the u-blox
 * module, using only the single ST-Link USB cable already connected for
 * flashing/debugging - no separate USB-serial adapter wired to the module
 * is required.
 *
 *   PC (TeraTerm / any AT terminal / XMODEM sender)
 *     <--USB--> ST-Link VCP <--USART (console)--> STM32 <--USART (U_EXAMPLE_UART)--> module
 *
 * This is useful for:
 *   - Interactive AT command debugging: open the ST-Link VCP in a terminal
 *     and type AT commands directly to the module.
 *   - Local firmware updates: send "AT+USYFWUS=<baud>,1" through the bridge,
 *     then use the terminal's own XMODEM sender (e.g. TeraTerm's
 *     File > Transfer > XMODEM > Send) to flash new module firmware.
 *
 * The bridge is fully transparent: no AT parsing happens on the MCU, bytes
 * are simply copied byte-for-byte in both directions. This means it works
 * with any AT command or binary protocol the module supports, without this
 * example needing to know anything about it.
 *
 * Note: the module UART baud rate is fixed at build time (U_EXAMPLE_BRIDGE_BAUD).
 * If you change the module's baud rate through the bridge (e.g. via AT+USYUS
 * or as a side effect of AT+USYFWUS), reconfigure U_EXAMPLE_BRIDGE_BAUD to
 * match and rebuild/reflash - this example does not follow baud rate changes
 * at runtime.
 */

#include <stdio.h>
#include <stdint.h>
#include "u_port.h"
#include "example_utils.h"

#ifndef U_PORT_FREERTOS
#error "uart_bridge_example is only supported on STM32 (FreeRTOS) targets - see examples/README.md"
#endif

#ifndef U_EXAMPLE_BRIDGE_BAUD
#define U_EXAMPLE_BRIDGE_BAUD 115200
#endif

/* Poll "timeout" per direction, in ms. 0 = non-blocking: check once, move
 * straight on to the other direction if nothing is available.
 *
 * Both directions are backed by interrupt/DMA-fed ring buffers (module UART:
 * u_port_uart_stm32*.c; console UART: main_stm32.c), so a non-blocking check
 * here never loses bytes - the buffers keep collecting data between polls
 * regardless of how this loop is scheduled. A non-zero value used to be
 * needed here to avoid busy-spinning, but it actively hurt throughput: with
 * single-byte-at-a-time relaying, blocking on an idle direction for even a
 * couple of ms adds up fast over a large binary transfer (e.g. XMODEM),
 * which was the root cause of transfers "getting stuck"/timing out. Keep
 * this at 0 unless you have a specific reason to slow the loop down. */
#define U_EXAMPLE_BRIDGE_POLL_MS 0

/*
 * Raw access to the console/debug UART (ST-Link VCP), implemented per-board
 * in main_stm32.c. This bypasses the printf retarget (TX only) so the
 * console UART can also be read.
 */
extern int32_t exampleConsoleUartRead(uint8_t *pByte, int32_t timeoutMs);
extern int32_t exampleConsoleUartWrite(const void *pData, size_t length);

int U_EXAMPLE_MAIN(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    printf("===========================================\n");
    printf("UART Bridge Example\n");
    printf("===========================================\n");
    printf("Bridging module UART (%s) <-> this console (ST-Link VCP)\n", U_EXAMPLE_UART);
    printf("Baud: %d, no flow control\n", U_EXAMPLE_BRIDGE_BAUD);
    printf("\n");
    printf("Open this port in a terminal to talk to the module directly:\n");
    printf("  - Send AT commands for interactive debugging\n");
    printf("  - Send AT+USYFWUS then use the terminal's XMODEM sender to\n");
    printf("    flash new module firmware, over this same USB cable\n");
    printf("\n");
    printf("The bridge is transparent - this is the last line this example\n");
    printf("prints. Reset the board to exit.\n");
    printf("===========================================\n\n");

    uPortInit();

    uPortUartHandle_t moduleUart = uPortUartOpen(U_EXAMPLE_UART, U_EXAMPLE_BRIDGE_BAUD, false);
    if (moduleUart == NULL) {
        printf("ERROR: Failed to open module UART %s\n", U_EXAMPLE_UART);
        return -1;
    }

    for (;;) {
        uint8_t byte;

        /* Module -> console */
        if (uPortUartRead(moduleUart, &byte, 1, U_EXAMPLE_BRIDGE_POLL_MS) == 1) {
            (void)exampleConsoleUartWrite(&byte, 1);
        }

        /* Console -> module */
        if (exampleConsoleUartRead(&byte, U_EXAMPLE_BRIDGE_POLL_MS) == 1) {
            (void)uPortUartWrite(moduleUart, &byte, 1);
        }
    }

    /* Never reached */
    uPortUartClose(moduleUart);
    return 0;
}
