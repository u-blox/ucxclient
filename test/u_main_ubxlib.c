/** @file
 * @brief Short description of the purpose of the file
 */
#include <stdio.h>

#include "ubxlib.h"
#include "u_cx_at_client.h"
#include "u_cx_system.h"
#include "u_cx_general.h"

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

static void urcHandler(char *pUrcLine)
{
    printf("Got URC: %s\n", pUrcLine);
}

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -------------------------------------------------------------- */

int main(void)
{
    char rxBuf[1024];
    uCxAtClient_t client;

    // Initiate ubxlib
    uPortInit();
    // Get the uart
    uPortUartInit();
    int32_t handleOrErrorCode = uPortUartOpen(
                                    0,
                                    115200,
                                    NULL,
                                    1024,
                                    -1,
                                    -1,
                                    -1,
                                    -1);
    if (handleOrErrorCode < 0) {
        printf("* Failed to open uart\n");
        return 1;
    }

    uCxAtClientInit(U_INT32_TO_PTR(handleOrErrorCode), rxBuf, sizeof(rxBuf), &client);
    uCxSystemSetEchoOff(&client);
    client.urcCallback = urcHandler;
    for (int i = 0; i < 3; i++) {
        const char *pVersion;
        int32_t ret = uCxBeginGeneralGetSoftwareVersion(&client, &pVersion);
        if (ret >= 0) {
            printf("%d Got response: %s\n", i, pVersion);
        } else {
            printf("%d command failed, error code: %d\n", i, ret);
        }
        uCxAtClientCmdEnd(&client);
    }
}
