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

#ifndef API_COMMANDS_H
#define API_COMMANDS_H

#include "common.h"

// ----------------------------------------------------------------
// API Command Management Functions
// ----------------------------------------------------------------

/**
 * Fetch API commands from GitHub for specific product and version
 * 
 * @param product Product name (e.g., "NORA-W36")
 * @param version Firmware version (e.g., "3.1.0")
 * @return true if successful, false otherwise
 */
bool fetchApiCommandsFromGitHub(const char *product, const char *version);

/**
 * Parse YAML content and populate gApiCommands array
 * 
 * @param yamlContent YAML content string
 */
void parseYamlCommands(const char *yamlContent);

/**
 * Free dynamically allocated API commands
 */
void freeApiCommands(void);

/**
 * List all API commands organized by chapter
 */
void listAllApiCommands(void);

/**
 * Fetch latest firmware version for a product from GitHub
 * 
 * @param product Product name
 * @return Version string (caller must free), or NULL if failed
 */
char* fetchLatestVersion(const char *product);

/**
 * Extract product name from firmware filename
 * 
 * @param filename Firmware filename
 * @return Product name (caller must free), or NULL if failed
 */
char* extractProductFromFilename(const char *filename);

// ----------------------------------------------------------------
// HTTP Helper Functions
// ----------------------------------------------------------------

/**
 * HTTP GET request
 * 
 * @param server Server hostname
 * @param path Request path
 * @return Response body (caller must free), or NULL if failed
 */
char* httpGetRequest(const wchar_t *server, const wchar_t *path);

/**
 * HTTP GET request for binary data
 * 
 * @param server Server hostname
 * @param path Request path
 * @param outSize Output size pointer
 * @return Response body (caller must free), or NULL if failed
 */
char* httpGetBinaryRequest(const wchar_t *server, const wchar_t *path, size_t *outSize);

#endif // API_COMMANDS_H
