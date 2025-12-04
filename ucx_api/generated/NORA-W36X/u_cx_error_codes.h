/*
* This file was automatically generated using csnake v0.3.5.
*
* This file should not be edited directly, any changes will be
* overwritten next time the script is run.
*
* Source code for csnake is available at:
* https://gitlab.com/andrejr/csnake
*
* csnake is also available on PyPI, at :
* https://pypi.org/project/csnake
*/
#ifndef _U_CX_ERROR_CODES_H_
#define _U_CX_ERROR_CODES_H_

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* ------------------------------------------------------------
 * Common Error Codes
 * ---------------------------------------------------------- */

#define U_ERROR_COMMON_BSD_ERROR (-1)
#define U_ERROR_COMMON_NOT_INITIALISED (-2)
#define U_ERROR_COMMON_NOT_IMPLEMENTED (-3)
#define U_ERROR_COMMON_NOT_SUPPORTED (-4)
#define U_ERROR_COMMON_INVALID_PARAMETER (-5)
#define U_ERROR_COMMON_NO_MEMORY (-6)
#define U_ERROR_COMMON_NOT_RESPONDING (-7)
#define U_ERROR_COMMON_PLATFORM (-8)
#define U_ERROR_COMMON_TIMEOUT (-9)
#define U_ERROR_COMMON_DEVICE_ERROR (-10)
#define U_ERROR_COMMON_NOT_FOUND (-11)
#define U_ERROR_COMMON_INVALID_ADDRESS (-12)
#define U_ERROR_COMMON_TEMPORARY_FAILURE (-13)
#define U_ERROR_COMMON_AUTHENTICATION_FAILURE (-14)
#define U_ERROR_COMMON_OPERATION_IN_PROGRESS (-15)
#define U_ERROR_COMMON_NOT_CONNECTED (-16)
#define U_ERROR_COMMON_LIMIT_REACHED (-17)
#define U_ERROR_COMMON_ALREADY_CREATED (-18)
#define U_ERROR_COMMON_END_OF_TRANSMISSION (-19)
#define U_ERROR_COMMON_REMOTE_CANCELLED_TRANSMISSION (-19)
#define U_ERROR_COMMON_NOT_CONFIGURED (-20)
#define U_ERROR_COMMON_INVALID_RESPONSE (-21)
#define U_ERROR_COMMON_UNKNOWN (-22)

/* ------------------------------------------------------------
 * AT Error Codes
 * ---------------------------------------------------------- */

#define U_AT_STATUS_NOT_IMPLEMENTED (-31)
#define U_AT_STATUS_INVALID_COMMAND (-32)
#define U_AT_STATUS_INVALID_ARGUMENTS (-33)
#define U_AT_STATUS_INVALID_ARGUMENT_COUNT (-34)
#define U_AT_STATUS_INVALID_INT_ARG (-35)
#define U_AT_STATUS_INVALID_INT_RANGE (-36)
#define U_AT_STATUS_INVALID_STR_ARG (-37)
#define U_AT_STATUS_INVALID_STR_LENGTH (-38)
#define U_AT_STATUS_INVALID_ENUM_ARG (-39)
#define U_AT_STATUS_INVALID_IP_ADDR_ARG (-40)
#define U_AT_STATUS_INVALID_MAC_ADDR_ARG (-41)
#define U_AT_STATUS_INVALID_BD_ADDR_ARG (-42)
#define U_AT_STATUS_INVALID_BYTE_ARRAY_ARG (-43)
#define U_AT_STATUS_INVALID_BYTE_ARRAY_LENGTH (-44)
#define U_AT_STATUS_UNMATCHED_QUOTE (-45)
#define U_AT_STATUS_TIMEOUT (-46)
#define U_AT_STATUS_BIN_CMD_EXEC_AS_STD_CMD (-47)
#define U_AT_STATUS_INVALID_ESCAPE_CODE (-48)
#define U_AT_STATUS_INVALID_CHARACTER (-49)
#define U_AT_STATUS_INVALID_INT_LIST_ARG (-50)
#define U_AT_STATUS_INVALID_INT_LIST_LENGTH (-51)

/* ------------------------------------------------------------
 * Wi-Fi Error Codes
 * ---------------------------------------------------------- */

#define U_ERROR_WIFI_AT (-60)
#define U_ERROR_WIFI_NOT_CONFIGURED (-61)
#define U_ERROR_WIFI_NOT_FOUND (-62)
#define U_ERROR_WIFI_INVALID_MODE (-63)
#define U_ERROR_WIFI_TEMPORARY_FAILURE (-64)
#define U_ERROR_WIFI_ALREADY_CONNECTED (-65)
#define U_ERROR_WIFI_ALREADY_CONNECTED_TO_SSID (-66)
#define U_ERROR_WIFI_DISCONNECTED (-67)
#define U_ERROR_WIFI_ALREADY_UP (-68)
#define U_ERROR_WIFI_AP_NOT_STARTED (-69)
#define U_ERROR_WIFI_IS_DOWN (-70)
#define U_ERROR_WIFI_ALREADY_STARTED (-71)

/* ------------------------------------------------------------
 * GATT Error Codes
 * ---------------------------------------------------------- */

#define U_PORT_GATT_STATUS_INVALID_HANDLE (-91)
#define U_PORT_GATT_STATUS_READ_NOT_PERMITTED (-92)
#define U_PORT_GATT_STATUS_WRITE_NOT_PERMITTED (-93)
#define U_PORT_GATT_STATUS_INVALID_PDU (-94)
#define U_PORT_GATT_STATUS_AUTHENTICATION (-95)
#define U_PORT_GATT_STATUS_NOT_SUPPORTED (-96)
#define U_PORT_GATT_STATUS_INVALID_OFFSET (-97)
#define U_PORT_GATT_STATUS_AUTHORIZATION (-98)
#define U_PORT_GATT_STATUS_PREPARE_QUEUE_FULL (-99)
#define U_PORT_GATT_STATUS_ATTRIBUTE_NOT_FOUND (-100)
#define U_PORT_GATT_STATUS_ATTRIBUTE_NOT_LONG (-101)
#define U_PORT_GATT_STATUS_ENCRYPTION_KEY_SIZE (-102)
#define U_PORT_GATT_STATUS_INVALID_ATTRIBUTE_LEN (-103)
#define U_PORT_GATT_STATUS_UNLIKELY (-104)
#define U_PORT_GATT_STATUS_INSUFFICIENT_ENCRYPTION (-105)
#define U_PORT_GATT_STATUS_UNSUPPORTED_GROUP_TYPE (-106)
#define U_PORT_GATT_STATUS_INSUFFICIENT_RESOURCES (-107)
#define U_PORT_GATT_STATUS_DB_OUT_OF_SYNC (-108)
#define U_PORT_GATT_STATUS_VALUE_NOT_ALLOWED (-109)
#define U_PORT_GATT_STATUS_WRITE_REQ_REJECTED (-110)
#define U_PORT_GATT_STATUS_CCC_IMPROPER_CONF (-111)
#define U_PORT_GATT_STATUS_PROCEDURE_IN_PROGRESS (-112)
#define U_PORT_GATT_STATUS_OUT_OF_RANGE (-113)
#define U_PORT_GATT_STATUS_UNKNOWN (-114)

/* ------------------------------------------------------------
 * HTTP Error Codes
 * ---------------------------------------------------------- */

#define U_ERROR_HTTP_HEADER_NOT_READ (-160)

/* ------------------------------------------------------------
 * Socket Error Codes
 * ---------------------------------------------------------- */

#define U_ERROR_SOCKET_ALREADY_BOUND (-180)

/* ------------------------------------------------------------
 * TYPES
 * ---------------------------------------------------------- */

typedef struct {
    int32_t value;
    const char *pName;
    const char *pModule;
} uCxErrorCode_t;

/* ------------------------------------------------------------
 * FUNCTIONS
 * ---------------------------------------------------------- */

/**
 * @brief Get the error name for a given error code.
 *
 * @param errorCode The error code value.
 * @return The error name string, or NULL if not found.
 */
const char *uCxGetErrorName(int32_t errorCode);

/**
 * @brief Get the module name for a given error code.
 *
 * @param errorCode The error code value.
 * @return The module name string, or NULL if not found.
 */
const char *uCxGetErrorModule(int32_t errorCode);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _U_CX_ERROR_CODES_H_ */