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
#ifndef _U_CX_SOCKET_H_
#define _U_CX_SOCKET_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "u_cx_types.h"
#include "u_cx.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* ------------------------------------------------------------
 * RESPONSES
 * ---------------------------------------------------------- */

typedef struct
{
    int32_t socket_handle;         /**< Socket identifier be used for any operation on that socket. */
    int32_t tls_version;           /**< TLS version to use */
    const char * ca_name;          /**< Name of the certificate authority (CA) certificate to use */
    const char * client_cert_name; /**< Name of the client certificate to use */
    const char * client_key_name;  /**< Name of the private key for client certificate */
} uCxSocketGetTLS_t;

typedef struct
{
    uSockIpAddress_t remote_ip; /**< The ip address of the remote peer. */
    int32_t remote_port;        /**< The port of the remote peer. */
} uCxSocketReceiveFrom_t;

typedef struct
{
    uSockIpAddress_t remote_ip; /**< The ip address of the remote peer. */
    int32_t remote_port;        /**< The port of the remote peer. */
} uCxSocketGetPeerAddress_t;

typedef struct
{
    int32_t socket_handle; /**< Socket identifier be used for any operation on that socket. */
    int32_t protocol;      /**< IP protocol. */
    int32_t socket_status;
} uCxSocketListStatus_t;

typedef struct
{
    int32_t protocol;      /**< IP protocol. */
    int32_t socket_status;
} uCxSocketGetStatus_t;


/* ------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * ---------------------------------------------------------- */

/**
 * Creates a socket and associates it with the specified protocol (TCP or UDP).
 * 
 * Output AT command:
 * > AT+USOCR=<protocol>
 *
 * @param[in]  puCxHandle:    uCX API handle
 * @param      protocol:      IP protocol.
 * @param[out] pSocketHandle: Socket identifier be used for any operation on that socket.
 * @return                    0 on success, negative value on error.
 */
int32_t uCxSocketCreate1(uCxHandle_t * puCxHandle, uProtocol_t protocol, int32_t * pSocketHandle);

/**
 * Creates a socket and associates it with the specified protocol (TCP or UDP).
 * 
 * Output AT command:
 * > AT+USOCR=<protocol>,<preferred_protocol_type>
 *
 * @param[in]  puCxHandle:              uCX API handle
 * @param      protocol:                IP protocol.
 * @param      preferred_protocol_type: Selects the IP address type to use.
 * @param[out] pSocketHandle:           Socket identifier be used for any operation on that socket.
 * @return                              0 on success, negative value on error.
 */
int32_t uCxSocketCreate2(uCxHandle_t * puCxHandle, uProtocol_t protocol, uPreferredProtocolType_t preferred_protocol_type, int32_t * pSocketHandle);

/**
 * Add a TLS context to a socket. This is only valid for TCP client sockets.
 * 
 * Output AT command:
 * > AT+USOTLS=<socket_handle>,<tls_version>
 *
 * @param[in]  puCxHandle:    uCX API handle
 * @param      socket_handle: Socket identifier be used for any operation on that socket.
 * @param      tls_version:   TLS version to use
 * @return                    0 on success, negative value on error.
 */
int32_t uCxSocketSetTLS2(uCxHandle_t * puCxHandle, int32_t socket_handle, uTlsVersion_t tls_version);

/**
 * Add a TLS context to a socket. This is only valid for TCP client sockets.
 * 
 * Output AT command:
 * > AT+USOTLS=<socket_handle>,<tls_version>,<ca_name>
 *
 * @param[in]  puCxHandle:    uCX API handle
 * @param      socket_handle: Socket identifier be used for any operation on that socket.
 * @param      tls_version:   TLS version to use
 * @param      ca_name:       Name of the certificate authority (CA) certificate to use
 * @return                    0 on success, negative value on error.
 */
int32_t uCxSocketSetTLS3(uCxHandle_t * puCxHandle, int32_t socket_handle, uTlsVersion_t tls_version, const char * ca_name);

/**
 * Add a TLS context to a socket. This is only valid for TCP client sockets.
 * 
 * Output AT command:
 * > AT+USOTLS=<socket_handle>,<tls_version>,<ca_name>,<client_cert_name>,<client_key_name>
 *
 * @param[in]  puCxHandle:       uCX API handle
 * @param      socket_handle:    Socket identifier be used for any operation on that socket.
 * @param      tls_version:      TLS version to use
 * @param      ca_name:          Name of the certificate authority (CA) certificate to use
 * @param      client_cert_name: Name of the client certificate to use
 * @param      client_key_name:  Name of the private key for client certificate
 * @return                       0 on success, negative value on error.
 */
int32_t uCxSocketSetTLS5(uCxHandle_t * puCxHandle, int32_t socket_handle, uTlsVersion_t tls_version, const char * ca_name, const char * client_cert_name, const char * client_key_name);

/**
 * Get the TLS context information for a socket.
 * 
 * Output AT command:
 * > AT+USOTLS=<socket_handle>
 *
 * @param[in]  puCxHandle:       uCX API handle
 * @param      socket_handle:    Socket identifier be used for any operation on that socket.
 * @param[out] pSocketGetTLSRsp: Please see \ref uCxSocketGetTLS_t
 * @return                       true on success, false on error (error code will be returned by uCxEnd()).
 *
 * NOTES:
 * Must be terminated by calling uCxEnd()
 */
bool uCxSocketGetTLSBegin(uCxHandle_t * puCxHandle, int32_t socket_handle, uCxSocketGetTLS_t * pSocketGetTLSRsp);

/**
 * Establish a peer-to-peer connection to the specified remote host on the given remote port.
 * 
 * Output AT command:
 * > AT+USOC=<socket_handle>,<host_address>,<remote_port>
 *
 * @param[in]  puCxHandle:    uCX API handle
 * @param      socket_handle: Socket identifier be used for any operation on that socket.
 * @param      host_address:  Remote host IP address or domain name of the remote host.
 * @param      remote_port:   The port of the remote peer.
 * @return                    0 on success, negative value on error.
 */
int32_t uCxSocketConnect(uCxHandle_t * puCxHandle, int32_t socket_handle, const char * host_address, int32_t remote_port);

/**
 * Set the mode in which you would like to receive data in AT mode.
 * 
 * 
 * Notes:
 * Any created sockets or created persistent sockets will use the same receive mode as was configured when they were
 * created
 * Requires AT&W and a reboot before taking effect.
 * 
 * Output AT command:
 * > AT+USORM=<read_mode>
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      read_mode:  Modes to read data in AT
 * @return                 0 on success, negative value on error.
 */
int32_t uCxSocketSetReadMode(uCxHandle_t * puCxHandle, uReadMode_t read_mode);

/**
 * Read the current receive mode
 * 
 * Output AT command:
 * > AT+USORM?
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param[out] pReadMode:  Modes to read data in AT
 * @return                 0 on success, negative value on error.
 */
int32_t uCxSocketGetReadMode(uCxHandle_t * puCxHandle, uReadMode_t * pReadMode);

/**
 * Writes binary data to the specified socket in binary mode.
 * 
 * Output AT command:
 * > AT+USOWB=<socket_handle>,<binary_data>,<binary_data_len>
 *
 * @param[in]  puCxHandle:      uCX API handle
 * @param      socket_handle:   Socket identifier be used for any operation on that socket.
 * @param      binary_data:     The data to write.
 * @param      binary_data_len: length of binary_data
 * @return                      Negative value on error. On success:
 *                              Data length that was actually written to socket.
 */
int32_t uCxSocketWrite(uCxHandle_t * puCxHandle, int32_t socket_handle, const uint8_t * binary_data, int32_t binary_data_len);

/**
 * Closes the specified socket.
 * 
 * The command blocks the AT command interface until the completion of the socket close operation.
 * When this function returns OK the socket is cleaned up and fully closed.
 * 
 * Output AT command:
 * > AT+USOCL=<socket_handle>
 *
 * @param[in]  puCxHandle:    uCX API handle
 * @param      socket_handle: Socket identifier to be used for any future operation on that socket.
 * @return                    0 on success, negative value on error.
 */
int32_t uCxSocketClose(uCxHandle_t * puCxHandle, int32_t socket_handle);

/**
 * Reads the specified amount of data from the specified socket in binary mode.
 * 
 * Output AT command:
 * > AT+USORB=<socket_handle>,<length>
 *
 * @param[in]  puCxHandle:    uCX API handle
 * @param      socket_handle: Socket identifier be used for any operation on that socket.
 * @param      length:        Number of bytes to read.
 * @param[out] pDataBuf:      Output data buffer
 * @return                    Number of bytes read or negative value on error.
 */
int32_t uCxSocketRead(uCxHandle_t * puCxHandle, int32_t socket_handle, int32_t length, uint8_t * pDataBuf);

/**
 * Retrieves the last error that occurred in any socket operation, stored in the socket errno.
 * 
 * Output AT command:
 * > AT+USOE
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param[out] pErrorCode: BSD error code. See BSD standard for error code definitions.
 * @return                 0 on success, negative value on error.
 */
int32_t uCxSocketGetLastError(uCxHandle_t * puCxHandle, int32_t * pErrorCode);

/**
 * Binds the specified socket to the specified local port.
 * 
 * Output AT command:
 * > AT+USOB=<socket_handle>,<local_port>
 *
 * @param[in]  puCxHandle:    uCX API handle
 * @param      socket_handle: Socket identifier be used for any operation on that socket.
 * @param      local_port:    Local port of service, range 1-65535. Port numbers below 1024 are not recommended since
 *                            they are usually reserved
 * @return                    0 on success, negative value on error.
 */
int32_t uCxSocketBind(uCxHandle_t * puCxHandle, int32_t socket_handle, int32_t local_port);

/**
 * Sets the specified socket in listening mode on the specified port of service, waiting for incoming connections (TCP) or
 * data (UDP).
 * 
 * Output AT command:
 * > AT+USOL=<socket_handle>
 *
 * @param[in]  puCxHandle:    uCX API handle
 * @param      socket_handle: Socket identifier be used for any operation on that socket.
 * @return                    0 on success, negative value on error.
 */
int32_t uCxSocketListen1(uCxHandle_t * puCxHandle, int32_t socket_handle);

/**
 * Sets the specified socket in listening mode on the specified port of service, waiting for incoming connections (TCP) or
 * data (UDP).
 * 
 * Output AT command:
 * > AT+USOL=<socket_handle>,<port>
 *
 * @param[in]  puCxHandle:    uCX API handle
 * @param      socket_handle: Socket identifier be used for any operation on that socket.
 * @param      port:          Port of service, range 1-65535. Port numbers below 1024 are not recommended since they are
 *                            usually reserved
 * @return                    0 on success, negative value on error.
 */
int32_t uCxSocketListen2(uCxHandle_t * puCxHandle, int32_t socket_handle, int32_t port);

/**
 * Reads the specified amount of data from the specified UDP socket.
 * 
 * Output AT command:
 * > AT+USORFB=<socket_handle>,<length>
 *
 * @param[in]  puCxHandle:            uCX API handle
 * @param      socket_handle:         Socket identifier be used for any operation on that socket.
 * @param      length:                Number of bytes to read.
 * @param[out] pDataBuf:              Output data buffer
 * @param[out] pSocketReceiveFromRsp: Please see \ref uCxSocketReceiveFrom_t
 * @return                            Number of bytes read or negative value on error.
 */
int32_t uCxSocketReceiveFrom(uCxHandle_t * puCxHandle, int32_t socket_handle, int32_t length, uint8_t * pDataBuf, uCxSocketReceiveFrom_t * pSocketReceiveFromRsp);

/**
 * Get the address of remote peer.
 * 
 * Output AT command:
 * > AT+USOPA=<socket_handle>
 *
 * @param[in]  puCxHandle:               uCX API handle
 * @param      socket_handle:            Socket identifier be used for any operation on that socket.
 * @param[out] pSocketGetPeerAddressRsp: Please see \ref uCxSocketGetPeerAddress_t
 * @return                               0 on success, negative value on error.
 */
int32_t uCxSocketGetPeerAddress(uCxHandle_t * puCxHandle, int32_t socket_handle, uCxSocketGetPeerAddress_t * pSocketGetPeerAddressRsp);

/**
 * List status for all created sockets.
 * 
 * Output AT command:
 * > AT+USOST?
 *
 * @param[in]  puCxHandle: uCX API handle
 * @return                 true on success, false on error (error code will be returned by uCxEnd()).
 *
 * NOTES:
 * Must be terminated by calling uCxEnd()
 */
void uCxSocketListStatusBegin(uCxHandle_t * puCxHandle);

/**
 * 
 *
 * @param[in]  puCxHandle:           uCX API handle
 * @param[out] pSocketListStatusRsp: Please see \ref uCxSocketListStatus_t
 * @return                           true on success, false when there are no more entries or on error (uCxEnd() will return
 *                                   error code in this case).
 */
bool uCxSocketListStatusGetNext(uCxHandle_t * puCxHandle, uCxSocketListStatus_t * pSocketListStatusRsp);

/**
 * Get the status of a specific socket.
 * 
 * Output AT command:
 * > AT+USOST=<socket_handle>
 *
 * @param[in]  puCxHandle:          uCX API handle
 * @param      socket_handle:       Socket identifier be used for any operation on that socket.
 * @param[out] pSocketGetStatusRsp: Please see \ref uCxSocketGetStatus_t
 * @return                          0 on success, negative value on error.
 */
int32_t uCxSocketGetStatus(uCxHandle_t * puCxHandle, int32_t socket_handle, uCxSocketGetStatus_t * pSocketGetStatusRsp);

/**
 * Set a socket option. See available options below.
 * 
 * Output AT command:
 * > AT+USOO=<socket_handle>,<option>,<value>
 *
 * @param[in]  puCxHandle:    uCX API handle
 * @param      socket_handle: Socket identifier be used for any operation on that socket.
 * @param      option:        Available options to set
 * @param      value:         See option parameter
 * @return                    0 on success, negative value on error.
 */
int32_t uCxSocketSetOption(uCxHandle_t * puCxHandle, int32_t socket_handle, uOption_t option, int32_t value);

/**
 * Read a socket option for a socket
 * 
 * Output AT command:
 * > AT+USOO=<socket_handle>,<option>
 *
 * @param[in]  puCxHandle:    uCX API handle
 * @param      socket_handle: Socket identifier be used for any operation on that socket.
 * @param      option:        Available options to set
 * @param[out] pValue:        See option parameter
 * @return                    0 on success, negative value on error.
 */
int32_t uCxSocketGetOption(uCxHandle_t * puCxHandle, int32_t socket_handle, uOption_t option, int32_t * pValue);

/**
 * Does a DNS lookup of a host name and returns the IP address.
 * 
 * Output AT command:
 * > AT+USOH=<host_name>
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      host_name:  Name to lookup.
 * @param[out] pHostIp:    The ip address of the host.
 * @return                 0 on success, negative value on error.
 */
int32_t uCxSocketGetHostByName(uCxHandle_t * puCxHandle, const char * host_name, uSockIpAddress_t * pHostIp);

/**
 * Register Connect event callback
 * 
 * Event is sent out after a successful connection to a remote peer.
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      callback:   callback to register. Set to NULL to unregister.
 */
void uCxSocketRegisterConnect(uCxHandle_t * puCxHandle, uUESOC_t callback);

/**
 * Register DataAvailable event callback
 * 
 * Data is available to be read. This will be sent out when using the buffered data mode.
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      callback:   callback to register. Set to NULL to unregister.
 */
void uCxSocketRegisterDataAvailable(uCxHandle_t * puCxHandle, uUESODA_t callback);

/**
 * Register Closed event callback
 * 
 * Event is sent out either when a socket was closed (by the remote or timed out) or when a connection to a remote peer has
 * failed.
 * When this event is sent out the socket has been fully closed and the handle can be re-used.
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      callback:   callback to register. Set to NULL to unregister.
 */
void uCxSocketRegisterClosed(uCxHandle_t * puCxHandle, uUESOCL_t callback);

/**
 * Register IncomingConnection event callback
 * 
 * This event is sent when there is an incoming connection for a server socket.
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      callback:   callback to register. Set to NULL to unregister.
 */
void uCxSocketRegisterIncomingConnection(uCxHandle_t * puCxHandle, uUESOIC_t callback);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _U_CX_SOCKET_H_ */