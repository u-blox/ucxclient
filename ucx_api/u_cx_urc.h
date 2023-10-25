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
#ifndef _U_CX_URC_H_
#define _U_CX_URC_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "u_cx.h"

/* ------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * ---------------------------------------------------------- */

int32_t uCxUrcParse(uCxHandle_t * puCxHandle, const char * pUrcName, char * pParams, size_t paramsLength);

void uCxUrcRegisterBluetoothConnect(struct uCxHandle * puCxHandle, uUEBTC_t callback);

void uCxUrcRegisterBluetoothDisconnect(struct uCxHandle * puCxHandle, uUEBTDC_t callback);

void uCxUrcRegisterBluetoothBondStatus(struct uCxHandle * puCxHandle, uUEBTB_t callback);

void uCxUrcRegisterBluetoothUserConfirmation(struct uCxHandle * puCxHandle, uUEBTUC_t callback);

void uCxUrcRegisterBluetoothPasskeyRequest(struct uCxHandle * puCxHandle, uUEBTUPE_t callback);

void uCxUrcRegisterBluetoothPhyUpdate(struct uCxHandle * puCxHandle, uUEBTPHYU_t callback);

void uCxUrcRegisterGattClientNotification(struct uCxHandle * puCxHandle, uUEBTGCN_t callback);

void uCxUrcRegisterGattClientIndication(struct uCxHandle * puCxHandle, uUEBTGCI_t callback);

void uCxUrcRegisterGattServerNotification(struct uCxHandle * puCxHandle, uUEBTGCW_t callback);

void uCxUrcRegisterGattServerReadAttribute(struct uCxHandle * puCxHandle, uUEBTGRR_t callback);

void uCxUrcRegisterGattServerIndicationAck(struct uCxHandle * puCxHandle, uUEBTGIC_t callback);

void uCxUrcRegisterSpsConnect(struct uCxHandle * puCxHandle, uUESPSC_t callback);

void uCxUrcRegisterSpsDisconnect(struct uCxHandle * puCxHandle, uUESPSDC_t callback);

void uCxUrcRegisterSpsDataAvailable(struct uCxHandle * puCxHandle, uUESPSDA_t callback);

void uCxUrcRegisterWifiLinkUp(struct uCxHandle * puCxHandle, uUEWLU_t callback);

void uCxUrcRegisterWifiLinkDown(struct uCxHandle * puCxHandle, uUEWLD_t callback);

void uCxUrcRegisterWifiStationNetworkUp(struct uCxHandle * puCxHandle, uUEWSNU_t callback);

void uCxUrcRegisterWifiStationNetworkDown(struct uCxHandle * puCxHandle, uUEWSND_t callback);

void uCxUrcRegisterWifiApNetworkUp(struct uCxHandle * puCxHandle, uUEWAPNU_t callback);

void uCxUrcRegisterWifiApNetworkDown(struct uCxHandle * puCxHandle, uUEWAPND_t callback);

void uCxUrcRegisterWifiApUp(struct uCxHandle * puCxHandle, uUEWAPU_t callback);

void uCxUrcRegisterWifiApDown(struct uCxHandle * puCxHandle, uUEWAPD_t callback);

void uCxUrcRegisterWifiApStationAssociated(struct uCxHandle * puCxHandle, uUEWAPSA_t callback);

void uCxUrcRegisterWifiApStationDisassociated(struct uCxHandle * puCxHandle, uUEWAPSDA_t callback);

void uCxUrcRegisterSocketConnect(struct uCxHandle * puCxHandle, uUESOC_t callback);

void uCxUrcRegisterSocketDataAvailable(struct uCxHandle * puCxHandle, uUESODA_t callback);

void uCxUrcRegisterSocketClosed(struct uCxHandle * puCxHandle, uUESOCL_t callback);

void uCxUrcRegisterSocketIncommingConnection(struct uCxHandle * puCxHandle, uUESOIC_t callback);

void uCxUrcRegisterMqttConnect(struct uCxHandle * puCxHandle, uUEMQC_t callback);

void uCxUrcRegisterMqttDisconnect(struct uCxHandle * puCxHandle, uUEMQDC_t callback);

void uCxUrcRegisterMqttDataAvailable(struct uCxHandle * puCxHandle, uUEMQDA_t callback);

void uCxUrcRegisterDiagnosticsPingComplete(struct uCxHandle * puCxHandle, uUEDGPC_t callback);

void uCxUrcRegisterDiagnosticsPingResponse(struct uCxHandle * puCxHandle, uUEDGP_t callback);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _U_CX_URC_H_ */