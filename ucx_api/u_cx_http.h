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
#ifndef _U_CX_HTTP_H_
#define _U_CX_HTTP_H_

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
    int32_t session_id;            /**< Unique http session identifier. Currently only one session is supported, 0. */
    int32_t tls_version;           /**< TLS version to use */
    const char * ca_name;          /**< Name of the certificate authority (CA) certificate to use */
    const char * client_cert_name; /**< Name of the client certificate to use */
    const char * client_key_name;  /**< Name of the private key for client certificate */
} uCxHttpGetHttpTls_t;

typedef struct
{
    int32_t session_id;           /**< Unique http session identifier. Currently only one session is supported, 0. */
    int32_t more_to_read;         /**< Indicates if there is more data to be read. */
    uByteArray_t byte_array_data; /**< data encoded as a ascii chars. */
} uCxHttpHeaderGet_t;

typedef struct
{
    int32_t more_to_read;         /**< Indicates if there is more data to be read. */
    int32_t data_length;          /**< Length of the data to be read */
    uByteArray_t byte_array_data; /**< data encoded as a ascii chars. */
} uCxHttpGetHttpBodyString_t;

typedef struct
{
    int32_t session_id; /**< Unique http session identifier. Currently only one session is supported, 0. */
    const char * path;  /**< The path to the HTTP request. */
} uCxHttpHttpGetPath_t;

typedef struct
{
    int32_t session_id;       /**< Unique http session identifier. Currently only one session is supported, 0. */
    const char * header_data; /**< data encoded as a ascii chars. */
} uCxHttpGetRequestHeader_t;

typedef struct
{
    int32_t session_id;       /**< Unique http session identifier. Currently only one session is supported, 0. */
    const char * header_data; /**< data encoded as a ascii chars. */
} uCxHttpDeleteRequestHeader_t;

typedef struct
{
    int32_t session_id;     /**< Unique http session identifier. Currently only one session is supported, 0. */
    int32_t written_length; /**< Data length that was written as body. */
} uCxHttpPostRequestBinary_t;

typedef struct
{
    int32_t session_id;       /**< Unique http session identifier. Currently only one session is supported, 0. */
    const char * header_data; /**< data encoded as a ascii chars. */
} uCxHttpPostRequestHeader_t;

typedef struct
{
    int32_t session_id;     /**< Unique http session identifier. Currently only one session is supported, 0. */
    int32_t written_length; /**< Data length that was written as body. */
} uCxHttpPutRequestBinary_t;

typedef struct
{
    int32_t session_id;       /**< Unique http session identifier. Currently only one session is supported, 0. */
    const char * header_data; /**< data encoded as a ascii chars. */
} uCxHttpPutRequestHeader_t;


/* ------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * ---------------------------------------------------------- */

/**
 * Set the HTTP connection parameters.
 * 
 * Output AT command:
 * > AT+UHTCCP=<session_id>,<host>
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      session_id: Unique http session identifier. Currently only one session is supported, 0.
 * @param      host:       The host for the http session
 * @return                 0 on success, negative value on error.
 */
int32_t uCxHttpSetConnectionParams2(uCxHandle_t * puCxHandle, int32_t session_id, const char * host);

/**
 * Set the HTTP connection parameters.
 * 
 * Output AT command:
 * > AT+UHTCCP=<session_id>,<host>,<port>
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      session_id: Unique http session identifier. Currently only one session is supported, 0.
 * @param      host:       The host for the http session
 * @param      port:       The port for the http session
 * @return                 0 on success, negative value on error.
 */
int32_t uCxHttpSetConnectionParams3(uCxHandle_t * puCxHandle, int32_t session_id, const char * host, int32_t port);

/**
 * Add a TLS context to a http session.
 * 
 * Output AT command:
 * > AT+UHTCTLS=<session_id>,<tls_version>
 *
 * @param[in]  puCxHandle:  uCX API handle
 * @param      session_id:  Unique http session identifier. Currently only one session is supported, 0.
 * @param      tls_version: TLS version to use
 * @return                  0 on success, negative value on error.
 */
int32_t uCxHttpSetHttpTLS2(uCxHandle_t * puCxHandle, int32_t session_id, uTlsVersion_t tls_version);

/**
 * Add a TLS context to a http session.
 * 
 * Output AT command:
 * > AT+UHTCTLS=<session_id>,<tls_version>,<ca_name>
 *
 * @param[in]  puCxHandle:  uCX API handle
 * @param      session_id:  Unique http session identifier. Currently only one session is supported, 0.
 * @param      tls_version: TLS version to use
 * @param      ca_name:     Name of the certificate authority (CA) certificate to use
 * @return                  0 on success, negative value on error.
 */
int32_t uCxHttpSetHttpTLS3(uCxHandle_t * puCxHandle, int32_t session_id, uTlsVersion_t tls_version, const char * ca_name);

/**
 * Add a TLS context to a http session.
 * 
 * Output AT command:
 * > AT+UHTCTLS=<session_id>,<tls_version>,<ca_name>,<client_cert_name>,<client_key_name>
 *
 * @param[in]  puCxHandle:       uCX API handle
 * @param      session_id:       Unique http session identifier. Currently only one session is supported, 0.
 * @param      tls_version:      TLS version to use
 * @param      ca_name:          Name of the certificate authority (CA) certificate to use
 * @param      client_cert_name: Name of the client certificate to use
 * @param      client_key_name:  Name of the private key for client certificate
 * @return                       0 on success, negative value on error.
 */
int32_t uCxHttpSetHttpTLS5(uCxHandle_t * puCxHandle, int32_t session_id, uTlsVersion_t tls_version, const char * ca_name, const char * client_cert_name, const char * client_key_name);

/**
 * Get the TLS context information for a http session.
 * 
 * Output AT command:
 * > AT+UHTCTLS=<session_id>
 *
 * @param[in]  puCxHandle:         uCX API handle
 * @param      session_id:         Unique http session identifier. Currently only one session is supported, 0.
 * @param[out] pHttpGetHttpTlsRsp: Please see \ref uCxHttpGetHttpTls_t
 * @return                         true on success, false on error (error code will be returned by uCxEnd()).
 *
 * NOTES:
 * Must be terminated by calling uCxEnd()
 */
bool uCxHttpGetHttpTlsBegin(uCxHandle_t * puCxHandle, int32_t session_id, uCxHttpGetHttpTls_t * pHttpGetHttpTlsRsp);

/**
 * Disconnects the HTTP session identified by `<session_id>`.
 * 
 * Output AT command:
 * > AT+UHTCDC=<session_id>
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      session_id: Unique http session identifier. Currently only one session is supported, 0.
 * @return                 0 on success, negative value on error.
 */
int32_t uCxHttpDisconnectHttpSession(uCxHandle_t * puCxHandle, int32_t session_id);

/**
 * Read the HTTP response header up to `<data_length>` bytes. Several calls may be needed to get the complete header, if
 * its larger than `<data_length>`
 * 
 * Output AT command:
 * > AT+UHTCGH=<session_id>
 *
 * @param[in]  puCxHandle:        uCX API handle
 * @param      session_id:        Unique http session identifier. Currently only one session is supported, 0.
 * @param[out] pHttpHeaderGetRsp: Please see \ref uCxHttpHeaderGet_t
 * @return                        true on success, false on error (error code will be returned by uCxEnd()).
 *
 * NOTES:
 * Must be terminated by calling uCxEnd()
 */
bool uCxHttpHeaderGet1Begin(uCxHandle_t * puCxHandle, int32_t session_id, uCxHttpHeaderGet_t * pHttpHeaderGetRsp);

/**
 * Read the HTTP response header up to `<data_length>` bytes. Several calls may be needed to get the complete header, if
 * its larger than `<data_length>`
 * 
 * Output AT command:
 * > AT+UHTCGH=<session_id>,<data_length>
 *
 * @param[in]  puCxHandle:        uCX API handle
 * @param      session_id:        Unique http session identifier. Currently only one session is supported, 0.
 * @param      data_length:       Length of the data to be read
 * @param[out] pHttpHeaderGetRsp: Please see \ref uCxHttpHeaderGet_t
 * @return                        true on success, false on error (error code will be returned by uCxEnd()).
 *
 * NOTES:
 * Must be terminated by calling uCxEnd()
 */
bool uCxHttpHeaderGet2Begin(uCxHandle_t * puCxHandle, int32_t session_id, int32_t data_length, uCxHttpHeaderGet_t * pHttpHeaderGetRsp);

/**
 * Read the body of the last HTTP response, up to `<data_length>` bytes, as binary data.
 * Can be used several times, until all bytes of the body has been read or the server closes the connection.
 * If there is more data to be read this will be indicated by the response parameter <more_to_read>.
 * The total length of the body may be found in the Content-Length field of the header.
 * 
 * Output AT command:
 * > AT+UHTCGBB=<session_id>,<data_length>
 *
 * @param[in]  puCxHandle:  uCX API handle
 * @param      session_id:  Unique http session identifier. Currently only one session is supported, 0.
 * @param      data_length: Length of the data to be read
 * @param[out] pMoreToRead: Indicates if there is more data to be read.
 * @return                  0 on success, negative value on error.
 */
int32_t uCxHttpGetHttpBodyBinary(uCxHandle_t * puCxHandle, int32_t session_id, int32_t data_length, int32_t * pMoreToRead);

/**
 * Read the body of the last HTTP response, up to `<data_length>` bytes, as string data.
 * Can be used several times, until all bytes of the body has been read or the server closes the connection.
 * If there is more data to be read this will be indicated by the response parameter <more_to_read>.
 * The total length of the body may be found in the Content-Length field of the header.
 * 
 * Output AT command:
 * > AT+UHTCGBS=<session_id>,<data_length>
 *
 * @param[in]  puCxHandle:                uCX API handle
 * @param      session_id:                Unique http session identifier. Currently only one session is supported, 0.
 * @param      data_length:               Length of the data to be read
 * @param[out] pHttpGetHttpBodyStringRsp: Please see \ref uCxHttpGetHttpBodyString_t
 * @return                                true on success, false on error (error code will be returned by uCxEnd()).
 *
 * NOTES:
 * Must be terminated by calling uCxEnd()
 */
bool uCxHttpGetHttpBodyStringBegin(uCxHandle_t * puCxHandle, int32_t session_id, int32_t data_length, uCxHttpGetHttpBodyString_t * pHttpGetHttpBodyStringRsp);

/**
 * Add a custom header field to the current request. Using this will override any custom header set by {ref:AT+UHTCRHSC}.
 * Up to 10 header fields can be added to one same http request
 * 
 * Output AT command:
 * > AT+UHTCRHAF=<session_id>,<field_name>,<field_value>
 *
 * @param[in]  puCxHandle:  uCX API handle
 * @param      session_id:  Unique http session identifier. Currently only one session is supported, 0.
 * @param      field_name:  
 * @param      field_value: 
 * @return                  0 on success, negative value on error.
 */
int32_t uCxHttpHeaderAddField(uCxHandle_t * puCxHandle, int32_t session_id, const char * field_name, const char * field_value);

/**
 * Set a complete custom header for the HTTP request. This will override any configuration made with {ref:AT+UHTCRHAF}.
 * 
 * Output AT command:
 * > AT+UHTCRHCS=<session_id>,<header_data>
 *
 * @param[in]  puCxHandle:  uCX API handle
 * @param      session_id:  Unique http session identifier. Currently only one session is supported, 0.
 * @param      header_data: data encoded as a ascii chars.
 * @return                  0 on success, negative value on error.
 */
int32_t uCxHttpHeaderSetCustom(uCxHandle_t * puCxHandle, int32_t session_id, const char * header_data);

/**
 * Clear all header configurations made by {ref:AT+UHTCRHCS} or {ref:AT+UHTCRHAF} for index
 * 
 * Output AT command:
 * > AT+UHTCRHCC=<session_id>
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      session_id: Unique http session identifier. Currently only one session is supported, 0.
 * @return                 0 on success, negative value on error.
 */
int32_t uCxHttpHeaderClearCustom(uCxHandle_t * puCxHandle, int32_t session_id);

/**
 * Configure the path for the current http request"
 * 
 * Output AT command:
 * > AT+UHTCRP=<session_id>,<path>
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      session_id: Unique http session identifier. Currently only one session is supported, 0.
 * @param      path:       The path to the HTTP request.
 * @return                 0 on success, negative value on error.
 */
int32_t uCxHttpHttpSetPath(uCxHandle_t * puCxHandle, int32_t session_id, const char * path);

/**
 * Read the configured path for the current http request
 * 
 * Output AT command:
 * > AT+UHTCRP=<session_id>
 *
 * @param[in]  puCxHandle:          uCX API handle
 * @param      session_id:          Unique http session identifier. Currently only one session is supported, 0.
 * @param[out] pHttpHttpGetPathRsp: Please see \ref uCxHttpHttpGetPath_t
 * @return                          true on success, false on error (error code will be returned by uCxEnd()).
 *
 * NOTES:
 * Must be terminated by calling uCxEnd()
 */
bool uCxHttpHttpGetPathBegin(uCxHandle_t * puCxHandle, int32_t session_id, uCxHttpHttpGetPath_t * pHttpHttpGetPathRsp);

/**
 * Send a **GET** request to the configured URL with a minimal default header, or using the configured header if there is a
 * configuration. Content-Length will always be 0.
 * 
 * Output AT command:
 * > AT+UHTCRG=<session_id>
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      session_id: Unique http session identifier. Currently only one session is supported, 0.
 * @return                 0 on success, negative value on error.
 */
int32_t uCxHttpGetRequest(uCxHandle_t * puCxHandle, int32_t session_id);

/**
 * Return the HTTP header for a GET request
 * 
 * Output AT command:
 * > AT+UHTCRGH=<session_id>
 *
 * @param[in]  puCxHandle:               uCX API handle
 * @param      session_id:               Unique http session identifier. Currently only one session is supported, 0.
 * @param[out] pHttpGetRequestHeaderRsp: Please see \ref uCxHttpGetRequestHeader_t
 * @return                               true on success, false on error (error code will be returned by uCxEnd()).
 *
 * NOTES:
 * Must be terminated by calling uCxEnd()
 */
bool uCxHttpGetRequestHeaderBegin(uCxHandle_t * puCxHandle, int32_t session_id, uCxHttpGetRequestHeader_t * pHttpGetRequestHeaderRsp);

/**
 * Send a **HTTP DELETE** request to the configured URL with a minimal default header, or using the configured header if
 * there is a configuration.
 * 
 * Output AT command:
 * > AT+UHTCRD=<session_id>
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      session_id: Unique http session identifier. Currently only one session is supported, 0.
 * @return                 0 on success, negative value on error.
 */
int32_t uCxHttpDeleteRequest1(uCxHandle_t * puCxHandle, int32_t session_id);

/**
 * Send a **HTTP DELETE** request to the configured URL with a minimal default header, or using the configured header if
 * there is a configuration.
 * 
 * Output AT command:
 * > AT+UHTCRD=<session_id>,<byte_array_data>,<byte_array_data_len>
 *
 * @param[in]  puCxHandle:          uCX API handle
 * @param      session_id:          Unique http session identifier. Currently only one session is supported, 0.
 * @param      byte_array_data:     data encoded as a ascii chars.
 * @param      byte_array_data_len: length of byte_array_data
 * @return                          0 on success, negative value on error.
 */
int32_t uCxHttpDeleteRequest2(uCxHandle_t * puCxHandle, int32_t session_id, const char * byte_array_data, int32_t byte_array_data_len);

/**
 * Return the HTTP header for a DELETE request
 * 
 * Output AT command:
 * > AT+UHTCRDH=<session_id>
 *
 * @param[in]  puCxHandle:                  uCX API handle
 * @param      session_id:                  Unique http session identifier. Currently only one session is supported, 0.
 * @param[out] pHttpDeleteRequestHeaderRsp: Please see \ref uCxHttpDeleteRequestHeader_t
 * @return                                  true on success, false on error (error code will be returned by uCxEnd()).
 *
 * NOTES:
 * Must be terminated by calling uCxEnd()
 */
bool uCxHttpDeleteRequestHeaderBegin(uCxHandle_t * puCxHandle, int32_t session_id, uCxHttpDeleteRequestHeader_t * pHttpDeleteRequestHeaderRsp);

/**
 * Sends an HTTP POST request using a string as body.
 * 
 * Output AT command:
 * > AT+UHTCRPOS=<session_id>,<byte_array_data>,<byte_array_data_len>
 *
 * @param[in]  puCxHandle:          uCX API handle
 * @param      session_id:          Unique http session identifier. Currently only one session is supported, 0.
 * @param      byte_array_data:     data encoded as a ascii chars.
 * @param      byte_array_data_len: length of byte_array_data
 * @return                          0 on success, negative value on error.
 */
int32_t uCxHttpPostRequestString(uCxHandle_t * puCxHandle, int32_t session_id, const char * byte_array_data, int32_t byte_array_data_len);

/**
 * Sends an HTTP POST request using binary data as body.
 * 
 * Output AT command:
 * > AT+UHTCRPOB=<session_id>,<binary_data>,<binary_data_len>
 *
 * @param[in]  puCxHandle:                uCX API handle
 * @param      session_id:                Unique http session identifier. Currently only one session is supported, 0.
 * @param      binary_data:               
 * @param      binary_data_len:           length of binary_data
 * @param[out] pHttpPostRequestBinaryRsp: Please see \ref uCxHttpPostRequestBinary_t
 * @return                                0 on success, negative value on error.
 */
int32_t uCxHttpPostRequestBinary(uCxHandle_t * puCxHandle, int32_t session_id, const uint8_t * binary_data, int32_t binary_data_len, uCxHttpPostRequestBinary_t * pHttpPostRequestBinaryRsp);

/**
 * Return the HTTP header for a POST request
 * 
 * Output AT command:
 * > AT+UHTCRPOH=<session_id>
 *
 * @param[in]  puCxHandle:                uCX API handle
 * @param      session_id:                Unique http session identifier. Currently only one session is supported, 0.
 * @param[out] pHttpPostRequestHeaderRsp: Please see \ref uCxHttpPostRequestHeader_t
 * @return                                true on success, false on error (error code will be returned by uCxEnd()).
 *
 * NOTES:
 * Must be terminated by calling uCxEnd()
 */
bool uCxHttpPostRequestHeaderBegin(uCxHandle_t * puCxHandle, int32_t session_id, uCxHttpPostRequestHeader_t * pHttpPostRequestHeaderRsp);

/**
 * Sends an HTTP PUT request using a string as body.
 * 
 * Output AT command:
 * > AT+UHTCRPUS=<session_id>,<byte_array_data>,<byte_array_data_len>
 *
 * @param[in]  puCxHandle:          uCX API handle
 * @param      session_id:          Unique http session identifier. Currently only one session is supported, 0.
 * @param      byte_array_data:     data encoded as a ascii chars.
 * @param      byte_array_data_len: length of byte_array_data
 * @return                          0 on success, negative value on error.
 */
int32_t uCxHttpPutRequestString(uCxHandle_t * puCxHandle, int32_t session_id, const char * byte_array_data, int32_t byte_array_data_len);

/**
 * Sends an HTTP PUT request using binary data as body.
 * 
 * Output AT command:
 * > AT+UHTCRPUB=<session_id>,<binary_data>,<binary_data_len>
 *
 * @param[in]  puCxHandle:               uCX API handle
 * @param      session_id:               Unique http session identifier. Currently only one session is supported, 0.
 * @param      binary_data:              
 * @param      binary_data_len:          length of binary_data
 * @param[out] pHttpPutRequestBinaryRsp: Please see \ref uCxHttpPutRequestBinary_t
 * @return                               0 on success, negative value on error.
 */
int32_t uCxHttpPutRequestBinary(uCxHandle_t * puCxHandle, int32_t session_id, const uint8_t * binary_data, int32_t binary_data_len, uCxHttpPutRequestBinary_t * pHttpPutRequestBinaryRsp);

/**
 * Return the HTTP header for a PUT request
 * 
 * Output AT command:
 * > AT+UHTCRPUH=<session_id>
 *
 * @param[in]  puCxHandle:               uCX API handle
 * @param      session_id:               Unique http session identifier. Currently only one session is supported, 0.
 * @param[out] pHttpPutRequestHeaderRsp: Please see \ref uCxHttpPutRequestHeader_t
 * @return                               true on success, false on error (error code will be returned by uCxEnd()).
 *
 * NOTES:
 * Must be terminated by calling uCxEnd()
 */
bool uCxHttpPutRequestHeaderBegin(uCxHandle_t * puCxHandle, int32_t session_id, uCxHttpPutRequestHeader_t * pHttpPutRequestHeaderRsp);

/**
 * Register Disconnect event callback
 * 
 * Disconnected from HTTP server
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      callback:   callback to register. Set to NULL to unregister.
 */
void uCxHttpRegisterDisconnect(uCxHandle_t * puCxHandle, uUEHTCDC_t callback);

/**
 * Register HttpRequestStatus event callback
 * 
 * Response status from the latest HTTP request.
 *
 * @param[in]  puCxHandle: uCX API handle
 * @param      callback:   callback to register. Set to NULL to unregister.
 */
void uCxHttpRegisterHttpRequestStatus(uCxHandle_t * puCxHandle, uUEHTCRS_t callback);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _U_CX_HTTP_H_ */