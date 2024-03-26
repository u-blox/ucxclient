/** @file
 * @brief u-connectXpress API
 */

#include <stddef.h>  // NULL, size_t etc.
#include <stdbool.h>
#include <string.h>  // memcpy(), strcmp(), strcspn(), strspm()
#include <stdio.h>

#include "u_cx_log.h"
#include "u_cx_urc.h"

#include "u_cx.h"

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

/* ----------------------------------------------------------------
 * STATIC FUNCTIONS
 * -------------------------------------------------------------- */

static void urcCallback(struct uCxAtClient *pClient, void *pTag, char *pLine, size_t lineLength,
                        uint8_t *pBinaryData, size_t binaryDataLen)
{
    (void)pClient;
    (void)pTag;
    (void)pBinaryData;
    (void)binaryDataLen;
    uCxHandle_t *puCxHandle = (uCxHandle_t *)pTag;
    char *pParams = pLine;
    size_t paramLen = lineLength;
    // Start by checking where params starts. All URCs that have params will have a ':' before params
    while (pParams < pLine + lineLength) {
        if (*pParams == ':') {
            *pParams = 0;
            pParams++;
            paramLen--;
            break;
        }
        pParams++;
        paramLen--;
    }

    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "Received URC '%s', params: '%s'", pLine, pParams);
    uCxUrcParse(puCxHandle, pLine, pParams, paramLen);
}

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -------------------------------------------------------------- */

void uCxInit(uCxAtClient_t *pClient, uCxHandle_t *puCxHandle)
{
    memset(puCxHandle, 0, sizeof(uCxHandle_t));
    puCxHandle->pAtClient = pClient;
    uCxAtClientSetUrcCallback(pClient, urcCallback, puCxHandle);
}

int32_t uCxEnd(uCxHandle_t *puCxHandle)
{
    return uCxAtClientCmdEnd(puCxHandle->pAtClient);
}
