// Minimal FTDI D2XX function declarations for ucxclient
// These functions are imported from ftd2xx.dll (32-bit) or ftd2xx64.dll (64-bit)
// Based on FTDI D2XX Programmer's Guide

#ifndef FTD2XX_MINIMAL_H
#define FTD2XX_MINIMAL_H

#include <windows.h>

// Type definitions
typedef PVOID FT_HANDLE;
typedef ULONG FT_STATUS;

// Status codes
#define FT_OK 0
#define FT_INVALID_HANDLE 1
#define FT_DEVICE_NOT_FOUND 2
#define FT_DEVICE_NOT_OPENED 3
#define FT_IO_ERROR 4
#define FT_INSUFFICIENT_RESOURCES 5
#define FT_INVALID_PARAMETER 6

// Open flags
#define FT_OPEN_BY_SERIAL_NUMBER 1
#define FT_OPEN_BY_DESCRIPTION 2

// Purge flags
#define FT_PURGE_RX 1
#define FT_PURGE_TX 2

// Flow control
#define FT_FLOW_NONE 0x0000
#define FT_FLOW_RTS_CTS 0x0100
#define FT_FLOW_DTR_DSR 0x0200
#define FT_FLOW_XON_XOFF 0x0400

// Data characteristics
#define FT_BITS_8 8
#define FT_STOP_BITS_1 0
#define FT_STOP_BITS_2 2
#define FT_PARITY_NONE 0
#define FT_PARITY_ODD 1
#define FT_PARITY_EVEN 2
#define FT_PARITY_MARK 3
#define FT_PARITY_SPACE 4

// Event types
#define FT_EVENT_RXCHAR 1
#define FT_EVENT_MODEM_STATUS 2

// Function declarations - these will be resolved at link time from ftd2xx.lib
#ifdef __cplusplus
extern "C" {
#endif

FT_STATUS WINAPI FT_Open(int deviceNumber, FT_HANDLE *pHandle);

FT_STATUS WINAPI FT_OpenEx(PVOID pArg1, DWORD Flags, FT_HANDLE *pHandle);

FT_STATUS WINAPI FT_Close(FT_HANDLE ftHandle);

FT_STATUS WINAPI FT_Read(FT_HANDLE ftHandle, LPVOID lpBuffer, DWORD nBufferSize, LPDWORD lpBytesReturned);

FT_STATUS WINAPI FT_Write(FT_HANDLE ftHandle, LPVOID lpBuffer, DWORD nBufferSize, LPDWORD lpBytesWritten);

FT_STATUS WINAPI FT_SetBaudRate(FT_HANDLE ftHandle, ULONG BaudRate);

FT_STATUS WINAPI FT_SetDataCharacteristics(FT_HANDLE ftHandle, UCHAR WordLength, UCHAR StopBits, UCHAR Parity);

FT_STATUS WINAPI FT_SetFlowControl(FT_HANDLE ftHandle, USHORT FlowControl, UCHAR XonChar, UCHAR XoffChar);

FT_STATUS WINAPI FT_SetTimeouts(FT_HANDLE ftHandle, ULONG ReadTimeout, ULONG WriteTimeout);

FT_STATUS WINAPI FT_Purge(FT_HANDLE ftHandle, ULONG Mask);

FT_STATUS WINAPI FT_GetQueueStatus(FT_HANDLE ftHandle, DWORD *dwRxBytes);

FT_STATUS WINAPI FT_SetEventNotification(FT_HANDLE ftHandle, DWORD Mask, PVOID Param);

FT_STATUS WINAPI FT_SetUSBParameters(FT_HANDLE ftHandle, ULONG ulInTransferSize, ULONG ulOutTransferSize);

FT_STATUS WINAPI FT_SetLatencyTimer(FT_HANDLE ftHandle, UCHAR ucLatency);

// Device enumeration
FT_STATUS WINAPI FT_CreateDeviceInfoList(LPDWORD lpdwNumDevs);

// Device info structure
typedef struct _ft_device_list_info_node {
    ULONG Flags;
    ULONG Type;
    ULONG ID;
    DWORD LocId;
    char SerialNumber[16];
    char Description[64];
    FT_HANDLE ftHandle;
} FT_DEVICE_LIST_INFO_NODE;

FT_STATUS WINAPI FT_GetDeviceInfoList(FT_DEVICE_LIST_INFO_NODE *pDest, LPDWORD lpdwNumDevs);

FT_STATUS WINAPI FT_GetComPortNumber(FT_HANDLE ftHandle, LPLONG lplComPortNumber);

#ifdef __cplusplus
}
#endif

#endif // FTD2XX_MINIMAL_H
