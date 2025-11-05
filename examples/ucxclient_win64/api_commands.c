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

#include "api_commands.h"

// TODO: Extract from original ucxclient_win64.c lines ~277-1020, ~4253-4515

bool fetchApiCommandsFromGitHub(const char *product, const char *version)
{
    printf("TODO: fetchApiCommandsFromGitHub() - needs implementation\n");
    (void)product;
    (void)version;
    return false;
}

void parseYamlCommands(const char *yamlContent)
{
    printf("TODO: parseYamlCommands() - needs implementation\n");
    (void)yamlContent;
}

void freeApiCommands(void)
{
    if (gApiCommands) {
        free(gApiCommands);
        gApiCommands = NULL;
        gApiCommandCount = 0;
    }
}

void listAllApiCommands(void)
{
    printf("TODO: listAllApiCommands() - needs implementation\n");
}

char* fetchLatestVersion(const char *product)
{
    printf("TODO: fetchLatestVersion() - needs implementation\n");
    (void)product;
    return NULL;
}

char* extractProductFromFilename(const char *filename)
{
    printf("TODO: extractProductFromFilename() - needs implementation\n");
    (void)filename;
    return NULL;
}

char* httpGetRequest(const wchar_t *server, const wchar_t *path)
{
    printf("TODO: httpGetRequest() - needs implementation\n");
    (void)server;
    (void)path;
    return NULL;
}

char* httpGetBinaryRequest(const wchar_t *server, const wchar_t *path, size_t *outSize)
{
    printf("TODO: httpGetBinaryRequest() - needs implementation\n");
    (void)server;
    (void)path;
    if (outSize) *outSize = 0;
    return NULL;
}
