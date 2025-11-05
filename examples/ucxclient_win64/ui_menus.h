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

#ifndef UI_MENUS_H
#define UI_MENUS_H

#include "common.h"

// ----------------------------------------------------------------
// UI and Menu Functions
// ----------------------------------------------------------------

/**
 * Print application header
 */
void printHeader(void);

/**
 * Print welcome guide for first-time users
 */
void printWelcomeGuide(void);

/**
 * Print help information
 */
void printHelp(void);

/**
 * Print main menu
 */
void printMenu(void);

/**
 * Handle user input and menu navigation
 */
void handleUserInput(void);

/**
 * Bluetooth menu
 */
void bluetoothMenu(void);

/**
 * WiFi menu
 */
void wifiMenu(void);

/**
 * Socket menu
 */
void socketMenu(void);

/**
 * GATT Client menu
 */
void gattClientMenu(void);

/**
 * GATT Server menu
 */
void gattServerMenu(void);

/**
 * Security/TLS menu
 */
void securityTlsMenu(void);

#endif // UI_MENUS_H
