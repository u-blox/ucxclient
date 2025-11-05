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

#include "ui_menus.h"

// TODO: Extract from original ucxclient_win64.c lines ~2133-2473, ~2474-3115

void printHeader(void)
{
    printf("\n");
    printf("===========================================\n");
    printf("  u-blox UCXClient Win64 - Version %s\n", APP_VERSION);
    printf("===========================================\n");
    printf("\n");
}

void printWelcomeGuide(void)
{
    printf("TODO: printWelcomeGuide() - needs implementation\n");
}

void printHelp(void)
{
    printf("TODO: printHelp() - needs implementation\n");
}

void printMenu(void)
{
    printf("\n=== Main Menu ===\n");
    printf("1. Connect/Disconnect\n");
    printf("2. Bluetooth\n");
    printf("3. WiFi\n");
    printf("4. Sockets\n");
    printf("5. GATT\n");
    printf("6. MQTT\n");
    printf("7. HTTP\n");
    printf("8. Firmware Update\n");
    printf("9. API Commands\n");
    printf("h. Help\n");
    printf("q. Quit\n");
    printf("\nChoice: ");
}

void handleUserInput(void)
{
    printf("TODO: handleUserInput() - needs full implementation\n");
    printf("(This is the main input dispatcher - ~600 lines)\n");
    
    // Minimal implementation to prevent infinite loop
    char input[256];
    if (fgets(input, sizeof(input), stdin)) {
        if (input[0] == 'q' || input[0] == 'Q') {
            gMenuState = MENU_EXIT;
        } else {
            printf("Feature not yet implemented. Press 'q' to quit.\n");
        }
    }
}

void bluetoothMenu(void)
{
    printf("TODO: bluetoothMenu() - needs implementation\n");
    gMenuState = MENU_MAIN;
}

void wifiMenu(void)
{
    printf("TODO: wifiMenu() - needs implementation\n");
    gMenuState = MENU_MAIN;
}

void socketMenu(void)
{
    printf("TODO: socketMenu() - needs implementation\n");
    gMenuState = MENU_MAIN;
}

void gattClientMenu(void)
{
    printf("TODO: gattClientMenu() - needs implementation\n");
    gMenuState = MENU_MAIN;
}

void gattServerMenu(void)
{
    printf("TODO: gattServerMenu() - needs implementation\n");
    gMenuState = MENU_MAIN;
}

void securityTlsMenu(void)
{
    printf("TODO: securityTlsMenu() - needs implementation\n");
    gMenuState = MENU_MAIN;
}
