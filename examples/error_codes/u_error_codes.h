/*
 * Copyright 2025 u-blox
 *
 * Auto-generated from error_codes.yaml
 * u-connectXpress v3.1.0 Error Codes Documentation
 * Error codes defined in the codebase
 * 
 * Generated on: 2025-11-11 16:44:59
 * Do not edit manually - regenerate using convert_yaml_to_c.py
 */

#ifndef U_ERROR_CODES_H
#define U_ERROR_CODES_H

#include <stdint.h>
#include <stdio.h>

typedef struct {
    int32_t value;
    const char *name;
    const char *module;
} UcxErrorCode_t;

static const UcxErrorCode_t UCX_ERROR_CODES[] = {
    {1, "U_ERROR_COMMON_BSD_ERROR", "Common"},
    {2, "U_ERROR_COMMON_NOT_INITIALISED", "Common"},
    {3, "U_ERROR_COMMON_NOT_IMPLEMENTED", "Common"},
    {4, "U_ERROR_COMMON_NOT_SUPPORTED", "Common"},
    {5, "U_ERROR_COMMON_INVALID_PARAMETER", "Common"},
    {6, "U_ERROR_COMMON_NO_MEMORY", "Common"},
    {7, "U_ERROR_COMMON_NOT_RESPONDING", "Common"},
    {8, "U_ERROR_COMMON_PLATFORM", "Common"},
    {9, "U_ERROR_COMMON_TIMEOUT", "Common"},
    {10, "U_ERROR_COMMON_DEVICE_ERROR", "Common"},
    {11, "U_ERROR_COMMON_NOT_FOUND", "Common"},
    {12, "U_ERROR_COMMON_INVALID_ADDRESS", "Common"},
    {13, "U_ERROR_COMMON_TEMPORARY_FAILURE", "Common"},
    {14, "U_ERROR_COMMON_AUTHENTICATION_FAILURE", "Common"},
    {15, "U_ERROR_COMMON_OPERATION_IN_PROGRESS", "Common"},
    {16, "U_ERROR_COMMON_NOT_CONNECTED", "Common"},
    {17, "U_ERROR_COMMON_LIMIT_REACHED", "Common"},
    {18, "U_ERROR_COMMON_ALREADY_CREATED", "Common"},
    {19, "U_ERROR_COMMON_END_OF_TRANSMISSION", "Common"},
    {19, "U_ERROR_COMMON_REMOTE_CANCELLED_TRANSMISSION", "Common"},
    {20, "U_ERROR_COMMON_NOT_CONFIGURED", "Common"},
    {21, "U_ERROR_COMMON_INVALID_RESPONSE", "Common"},
    {22, "U_ERROR_COMMON_UNKNOWN", "Common"},
    {31, "U_AT_STATUS_NOT_IMPLEMENTED", "AT"},
    {32, "U_AT_STATUS_INVALID_COMMAND", "AT"},
    {33, "U_AT_STATUS_INVALID_ARGUMENTS", "AT"},
    {34, "U_AT_STATUS_INVALID_ARGUMENT_COUNT", "AT"},
    {35, "U_AT_STATUS_INVALID_INT_ARG", "AT"},
    {36, "U_AT_STATUS_INVALID_INT_RANGE", "AT"},
    {37, "U_AT_STATUS_INVALID_STR_ARG", "AT"},
    {38, "U_AT_STATUS_INVALID_STR_LENGTH", "AT"},
    {39, "U_AT_STATUS_INVALID_ENUM_ARG", "AT"},
    {40, "U_AT_STATUS_INVALID_IP_ADDR_ARG", "AT"},
    {41, "U_AT_STATUS_INVALID_MAC_ADDR_ARG", "AT"},
    {42, "U_AT_STATUS_INVALID_BD_ADDR_ARG", "AT"},
    {43, "U_AT_STATUS_INVALID_BYTE_ARRAY_ARG", "AT"},
    {44, "U_AT_STATUS_INVALID_BYTE_ARRAY_LENGTH", "AT"},
    {45, "U_AT_STATUS_UNMATCHED_QUOTE", "AT"},
    {46, "U_AT_STATUS_TIMEOUT", "AT"},
    {47, "U_AT_STATUS_BIN_CMD_EXEC_AS_STD_CMD", "AT"},
    {48, "U_AT_STATUS_INVALID_ESCAPE_CODE", "AT"},
    {49, "U_AT_STATUS_INVALID_CHARACTER", "AT"},
    {50, "U_AT_STATUS_INVALID_INT_LIST_ARG", "AT"},
    {51, "U_AT_STATUS_INVALID_INT_LIST_LENGTH", "AT"},
    {60, "U_ERROR_WIFI_AT", "Wi-Fi"},
    {61, "U_ERROR_WIFI_NOT_CONFIGURED", "Wi-Fi"},
    {62, "U_ERROR_WIFI_NOT_FOUND", "Wi-Fi"},
    {63, "U_ERROR_WIFI_INVALID_MODE", "Wi-Fi"},
    {64, "U_ERROR_WIFI_TEMPORARY_FAILURE", "Wi-Fi"},
    {65, "U_ERROR_WIFI_ALREADY_CONNECTED", "Wi-Fi"},
    {66, "U_ERROR_WIFI_ALREADY_CONNECTED_TO_SSID", "Wi-Fi"},
    {67, "U_ERROR_WIFI_DISCONNECTED", "Wi-Fi"},
    {68, "U_ERROR_WIFI_ALREADY_UP", "Wi-Fi"},
    {69, "U_ERROR_WIFI_AP_NOT_STARTED", "Wi-Fi"},
    {70, "U_ERROR_WIFI_IS_DOWN", "Wi-Fi"},
    {71, "U_ERROR_WIFI_ALREADY_STARTED", "Wi-Fi"},
    {91, "U_PORT_GATT_STATUS_INVALID_HANDLE", "GATT"},
    {92, "U_PORT_GATT_STATUS_READ_NOT_PERMITTED", "GATT"},
    {93, "U_PORT_GATT_STATUS_WRITE_NOT_PERMITTED", "GATT"},
    {94, "U_PORT_GATT_STATUS_INVALID_PDU", "GATT"},
    {95, "U_PORT_GATT_STATUS_AUTHENTICATION", "GATT"},
    {96, "U_PORT_GATT_STATUS_NOT_SUPPORTED", "GATT"},
    {97, "U_PORT_GATT_STATUS_INVALID_OFFSET", "GATT"},
    {98, "U_PORT_GATT_STATUS_AUTHORIZATION", "GATT"},
    {99, "U_PORT_GATT_STATUS_PREPARE_QUEUE_FULL", "GATT"},
    {100, "U_PORT_GATT_STATUS_ATTRIBUTE_NOT_FOUND", "GATT"},
    {101, "U_PORT_GATT_STATUS_ATTRIBUTE_NOT_LONG", "GATT"},
    {102, "U_PORT_GATT_STATUS_ENCRYPTION_KEY_SIZE", "GATT"},
    {103, "U_PORT_GATT_STATUS_INVALID_ATTRIBUTE_LEN", "GATT"},
    {104, "U_PORT_GATT_STATUS_UNLIKELY", "GATT"},
    {105, "U_PORT_GATT_STATUS_INSUFFICIENT_ENCRYPTION", "GATT"},
    {106, "U_PORT_GATT_STATUS_UNSUPPORTED_GROUP_TYPE", "GATT"},
    {107, "U_PORT_GATT_STATUS_INSUFFICIENT_RESOURCES", "GATT"},
    {108, "U_PORT_GATT_STATUS_DB_OUT_OF_SYNC", "GATT"},
    {109, "U_PORT_GATT_STATUS_VALUE_NOT_ALLOWED", "GATT"},
    {110, "U_PORT_GATT_STATUS_WRITE_REQ_REJECTED", "GATT"},
    {111, "U_PORT_GATT_STATUS_CCC_IMPROPER_CONF", "GATT"},
    {112, "U_PORT_GATT_STATUS_PROCEDURE_IN_PROGRESS", "GATT"},
    {113, "U_PORT_GATT_STATUS_OUT_OF_RANGE", "GATT"},
    {114, "U_PORT_GATT_STATUS_UNKNOWN", "GATT"},
    {160, "U_ERROR_HTTP_HEADER_NOT_READ", "HTTP"},
    {180, "U_ERROR_SOCKET_ALREADY_BOUND", "Socket"},
};

#define UCX_ERROR_CODES_COUNT 82

/**
 * Get the error name for a given error code
 * 
 * @param errorCode The error code value
 * @return The error name string, or NULL if not found
 */
static inline const char* ucxGetErrorName(int32_t errorCode) {
    for (int i = 0; i < UCX_ERROR_CODES_COUNT; i++) {
        if (UCX_ERROR_CODES[i].value == errorCode) {
            return UCX_ERROR_CODES[i].name;
        }
    }
    return NULL;
}

/**
 * Get the module name for a given error code
 * 
 * @param errorCode The error code value
 * @return The module name string, or NULL if not found
 */
static inline const char* ucxGetErrorModule(int32_t errorCode) {
    for (int i = 0; i < UCX_ERROR_CODES_COUNT; i++) {
        if (UCX_ERROR_CODES[i].value == errorCode) {
            return UCX_ERROR_CODES[i].module;
        }
    }
    return NULL;
}

/**
 * Get a user-friendly error description
 * 
 * @param errorCode The error code value
 * @param buffer Buffer to write the description to
 * @param bufferSize Size of the buffer
 * @return Number of characters written (excluding null terminator), or -1 if error not found
 */
static inline int ucxGetErrorDescription(int32_t errorCode, char *buffer, size_t bufferSize) {
    const char *name = ucxGetErrorName(errorCode);
    const char *module = ucxGetErrorModule(errorCode);
    
    if (name == NULL || module == NULL) {
        return -1;
    }
    
    int written = snprintf(buffer, bufferSize, "[%s] %s (code %d)", module, name, errorCode);
    return (written < (int)bufferSize) ? written : -1;
}

#endif /* U_ERROR_CODES_H */
