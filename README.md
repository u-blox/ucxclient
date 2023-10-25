# ucxclient
This repo contains an AT command client for talking to 2nd gen uConnectXpress modules such as NORA-W36. The client is written in C with a small RAM footprint.

There are two levels of APIs included in this repo; the lower [uAtClient API](#uatclient-api) and the upper [uConnectXpress API](#uconnectxpress-api)

## uAtClient API
This API contains an AT client implementation that handles transmission of AT commands, reception and parsing of AT responses and URCs. You will find the uAtClient API in the [inc/](inc) directory.

### Example
```c
#include "u_cx_at_client.h"

static int32_t myReadFunction(uCxAtClient_t *pClient, void *pStreamHandle, void *pData, size_t length, int32_t timeoutMs)
{
    // TODO: Implement
}

static int32_t myWriteFunction(uCxAtClient_t *pClient, void *pStreamHandle, const void *pData, size_t length)
{
    // TODO: Implement
}

void main(void)
{
  int32_t status;
  static char rxBuf[1024];
  static char urcBuf[1024];
  static uCxAtClientConfig_t config = {
      .pRxBuffer = &rxBuf[0],
      .rxBufferLen = sizeof(rxBuf),
      .pUrcBuffer = &urcBuf[0],
      .urcBufferLen = sizeof(urcBuf),
      .pStreamHandle = NULL,
      .write = uCxAtWrite,
      .read = uCxAtRead
  };

  uCxAtClient_t client;
  uCxAtClientInit(&config, &client);

  // Example of executing AT command without any AT response

  // To execute command without any response use the uCxAtClientExecSimpleCmd(F)
  // uCxAtClientExecSimpleCmdF() uses a format string for the AT command params where
  // each character represent a parameter type (a bit like printf)
  status = uCxAtClientExecSimpleCmdF(&client, "AT+USYUS=", "dd",
                                     115200, 0, U_CX_AT_UTIL_PARAM_LAST);
  printf("'AT+USYUS=' status: %d\n", status);


  // Example of executing AT command with an AT response

  // Execute cmd 'AT+USYUS?
  uCxAtClientCmdBeginF(&client, "AT+USYUS?", "", U_CX_AT_UTIL_PARAM_LAST);

  // Read out the AT response
  int32_t baudrate;
  int32_t flowControl;
  status = uCxAtClientCmdGetRspParamsF(&client, "+USYUS:", NULL, NULL, "dd",
                                       &baudrate, &flowControl, U_CX_AT_UTIL_PARAM_LAST);
  printf("Response params: %d\n", status);

  // All AT client APIs that ends with 'Begin' (such as uCxAtClientCmdBeginF())
  // must be terminated by calling uCxAtClientCmdEnd().
  // This where you'll get the AT command status
  status = uCxAtClientCmdEnd(&client);
  printf("'AT+USYUS?' status: %d, baudrate: %d, flowControl: %d\n", status, baudrate, flowControl);
}
```

## uConnectXpress API
This API is a higher level API that that simplifies communication with a 2nd gen uConnectXpress u-blox module.
Using this API eliminates the need of manually sending AT commands to the module.
You will find the uConnectXpress API in the [ucx_api/](ucx_api) directory.

### Example
```c
#include "u_cx_at_client.h"
#include "u_cx.h"
#include "u_cx_system.h"

void main(void)
{
  int32_t status;
  uCxAtClient_t client;
  uCxHandle_t ucxHandle;

  // You need to initialize the AT client in same way as in the uAtClient API example (part of this
  // has been left out here for simplicity)
  uCxAtClientInit(&config, &client);

  uCxInit(&client, &ucxHandle);

  // This will send the "AT+USYUS=" AT command
  status = uCxSystemSetUartSettings2(&ucxHandle, 115200, 0);
  printf("uCxSystemSetUartSettings2(): %d\n", status);

  // This will send the "AT+USYUS?" AT command and parse the AT reponse params to &settings
  uCxSystemGetUartSettings_t settings;
  status = uCxSystemGetUartSettings(&ucxHandle, &settings);
  printf("uCxSystemGetUartSettings(): %d, baudrate: %d, flow control: %d\n",
         status, settings.baud_rate, settings.flow_control);
```

## Porting and Configuration
All configuration and porting config is located in [inc/u_cx_at_config.h](inc/u_cx_at_config.h).
Make a copy of this file and place it in your code base where you can modify each config to your likings.
When compiling you can specify the name of this local file with `U_CX_AT_CONFIG_FILE` (for GCC you could pass `-DU_CX_AT_CONFIG_FILE=\"my_u_cx_at_config.h\"`).

### Minimum Porting
Some things are not required for successfully running the AT client (such as U_CX_PORT_PRINTF for logging, U_CX_AT_PORT_ASSERT), but the following is required:

| Function | Description |
| -------- | ----------- |
| U_CX_PORT_GET_TIME_MS | Must return a 32 bit timestamp in milliseconds.|
| read()   | Passed as argument to uCxAtClientInit(). Should read data from UART with a timeout in millisec. Must return the number of bytes received, 0 if there is no data available withing the timeout or negative value on error. |
| write()  | Passed as argument to uCxAtClientInit(). Should write data to the UART. Must return the number of actual bytes written or negative number on error. |

# Disclaimer
Copyright &#x00a9; u-blox

u-blox reserves all rights in this deliverable (documentation, software, etc.,
hereafter “Deliverable”).

u-blox grants you the right to use, copy, modify and distribute the
Deliverable provided hereunder for any purpose without fee.

THIS DELIVERABLE IS BEING PROVIDED "AS IS", WITHOUT ANY EXPRESS OR IMPLIED
WARRANTY. IN PARTICULAR, NEITHER THE AUTHOR NOR U-BLOX MAKES ANY
REPRESENTATION OR WARRANTY OF ANY KIND CONCERNING THE MERCHANTABILITY OF THIS
DELIVERABLE OR ITS FITNESS FOR ANY PARTICULAR PURPOSE.

In case you provide us a feedback or make a contribution in the form of a
further development of the Deliverable (“Contribution”), u-blox will have the
same rights as granted to you, namely to use, copy, modify and distribute the
Contribution provided to us for any purpose without fee.
