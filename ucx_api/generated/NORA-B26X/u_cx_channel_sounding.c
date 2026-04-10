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
#include <string.h>
#include "u_cx_at_client.h"
#include "u_cx_channel_sounding.h"

void uCxChannelSoundingRegisterCsStatus(uCxHandle_t * puCxHandle, uUEBTCSS_t callback)
{
    puCxHandle->callbacks.UEBTCSS = callback;
}
