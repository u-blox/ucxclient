# AT Commands Changes: u-connectXpress 3.0.0 for NORA-W36 → u-connectXpress 3.1.0 for NORA-W36

This document shows the differences between **u-connectXpress 3.0.0 for NORA-W36** and **u-connectXpress 3.1.0 for NORA-W36** of the AT commands specification.

## Summary

- **Added commands:** 0
- **Removed commands:** 0
- **Renamed commands:** 0
- **Modified commands:** 11


## 🔄 Modified Commands

#### Bluetooth

### AT+UBTBSM - Bluetooth Bond Security Mode

Set the **minimum** security mode ~~of the device.~~ **required when bonding.** This command works together with AT+UBTIOC to determine the bonding procedure used. **The module will try to use the highest level possible based on the settings of this command, AT+UBTIOC and the capabilities of the remote device. If the remote device does not support the required security level, the bonding procedure will fail.**

**Syntax**<br>
| <div style="width:350px">AT Command</div> | Description |
| ----------|----------|
| `AT+UBTBSM=<bt_security_mode>` | Writes the security mode<br><br>Notes:<br>Can be stored using [AT&W](#atw). |
| `AT+UBTBSM?` | Reads the security mode |

| <div style="width:350px">Response</div> | Description |
| ----------|----------|
| `+UBTBSM:<bt_security_mode>` | Successful read response for AT+UBTBSM? |

**Defined values**<br>
| Parameter | Type | Description |
| ----------|----------|----------|
| bt\_security\_mode | enumerator | Valid values:<br>0: Security ~~Disabled.<br>1: Allow~~ **not required. No encryption enforced.<br>1: Require at least** unauthenticated bonding.<br>2: ~~Only allow~~ **Require** authenticated bonding. No secure connections.<br>3: ~~Only allow~~ **Require** authenticated ~~bonding with encrypted Bluetooth link.~~ **bonding.** Support secure connections. Fallback to simple pairing if the remote side does not support secure connections.<br>4: ~~Only allow~~ **Require** authenticated ~~bonding with encrypted Bluetooth link.~~ **bonding.** Strictly uses secure connections.<br><br>Default value: 0 |

### AT+UBTM - Bluetooth Mode

Set and read Bluetooth Mode.

**Syntax**<br>
| <div style="width:350px">AT Command</div> | Description |
| ----------|----------|
| `AT+UBTM=<bt_mode>` | Set Bluetooth Mode.<br><br>Notes:<br>Can be stored using [AT&W](#atw). |
| `AT+UBTM?` | Read Bluetooth Mode. |

| <div style="width:350px">Response</div> | Description |
| ----------|----------|
| `+UBTM:<bt_mode>` | Successful read response. |

**Defined values**<br>
| Parameter | Type | Description |
| ----------|----------|----------|
| bt\_mode | enumerator | Valid values:<br>0: Disabled.<br>1: Bluetooth Low Energy Central. In this mode, starting advertisements, direct advertisements and other functions associated with the Peripheral role is not possible. <br>2: Bluetooth Low Energy Peripheral. In this mode, initiating connections, discovery and other functions associated with the Central role is not possible. <br>3: Bluetooth Low Energy Simultaneous Central and Peripheral. This is the factory ~~default for NORA-W36.~~ **default.** |

### AT+UBTSS - Bluetooth Scan Settings

Get and Set Scan Settings.

**Syntax**<br>
| <div style="width:350px">AT Command</div> | Description |
| ----------|----------|
| `AT+UBTSS0=<scan_interval>` | Write scan interval. |
| `AT+UBTSS0?` | Read scan Interval. |
| `AT+UBTSS1=<scan_window>` | Write scan window. |
| `AT+UBTSS1?` | Read scan Interval. |
| `AT+UBTSS?` | Read all scanning parameter setting values. |

| <div style="width:350px">Response</div> | Description |
| ----------|----------|
| `+UBTSS0:<scan_interval>` | Successful read of scan interval. |
| `+UBTSS1:<scan_window>` | Successful read of scan window. |
| `+UBTSS:<scan_param>,<value>` | Successful read response for AT+UBTSS?. |

**Defined values**<br>
| Parameter | Type | Description |
| ----------|----------|----------|
| scan\_interval | integer | Scan interval (must be >= Scan window. <br>Default: 160.<br>Calculation: scan_interval * 0.625 ms)<br><br>Valid values: 16..16384<br><br>Default value: 160 |
| scan\_window | integer | Scan window (must be <= Scan interval. <br>Default: 128.<br>Calculation: scan_interval * 0.625 ms)<br><br>Valid values: 16..16384<br><br>Default value: 128 |
| value | integer | Value of scan parameter.<br><br>Valid values: 0..65535 |
| scan\_param | enumerator | Scan parameter.<br><br>Valid values:<br>0: Scan interval.<br><br>1: Scan ~~interval~~**window**. |

#### HTTP

### AT+UHTCTLS - HTTP Client TLS Configuration


**Syntax**<br>
| <div style="width:350px">AT Command</div> | Description |
| ----------|----------|
| `AT+UHTCTLS=<session_id>,<tls_version>[,<ca_name>[,<client_cert_name>,<client_key_name>]]` | Add a TLS context to a http session. |
| `AT+UHTCTLS=<session_id>` | Get the TLS context information for a http session. |

| <div style="width:350px">Response</div> | Description |
| ----------|----------|
| `+UHTCTLS:<session_id>,<tls_version>,<ca_name>,<client_cert_name>,<client_key_name>` | Provides the TLS context information for the specified socket, including the TLS version, CA name, client certificate name, and client key name. This response is returned upon successfully retrieving the TLS configuration. |

**Defined values**<br>
| Parameter | Type | Description |
| ----------|----------|----------|
| session\_id | integer | Unique http session identifier. Currently only one session is supported, 0.<br><br>Valid values: 0..0 |
| tls\_version | enumerator | TLS version to use<br><br>Valid values:<br>0: Disable TLS<br><br>1: TLS 1.2 ~~or up~~<br><br>**2: TLS 1.3**<br><br>**3: TLS 1.2 or 1.3 (negotiate highest)** |
| ca\_name | string | Name of the certificate authority (CA) certificate to use<br><br>Length: 1..32 |
| client\_cert\_name | string | Name of the client certificate to use<br><br>Length: 1..32 |
| client\_key\_name | string | Name of the private key for client certificate<br><br>Length: 1..32 |

#### MQTT

### AT+UMQTLS - MQTT TLS Configuration


**Syntax**<br>
| <div style="width:350px">AT Command</div> | Description |
| ----------|----------|
| `AT+UMQTLS=<mqtt_id>,<tls_version>[,<ca_name>[,<client_cert_name>,<client_key_name>]]` | Setup MQTT TLS config. Certs do not have to be uploaded until connection.<br><br>Notes:<br>Can be stored using [AT&W](#atw). |
| `AT+UMQTLS=<mqtt_id>` | Get TLS config |

| <div style="width:350px">Response</div> | Description |
| ----------|----------|
| `+UMQTLS:<mqtt_id>,<tls_version>,<ca_name>,<client_cert_name>,<client_key_name>` | Successful read response |
| `+UMQTLS:<mqtt_id>,<tls_version>` | Successful read response with TLS off |

**Defined values**<br>
| Parameter | Type | Description |
| ----------|----------|----------|
| mqtt\_id | integer | MQTT Config ID<br><br>Valid values: 0..0 |
| tls\_version | enumerator | TLS version to use<br><br>Valid values:<br>0: Disable TLS<br><br>1: TLS 1.2 ~~or up~~<br><br>**2: TLS 1.3**<br><br>**3: TLS 1.2 or 1.3 (negotiate highest)** |
| ca\_name | string | Name of the certificate authority (CA) certificate to use<br><br>Length: 1..32 |
| client\_cert\_name | string | Name of the client certificate to use<br><br>Length: 1..32 |
| client\_key\_name | string | Name of the private key for client certificate<br><br>Length: 1..32 |

#### SPS

### AT+USPS - SPS - Enable/Disable Service

Enable or disable the SPS ~~service~~ **service. SPS Service will use the security mode set in AT+UBTBSM**

**Syntax**<br>
| <div style="width:350px">AT Command</div> | Description |
| ----------|----------|
| `AT+USPS=<sps_service_option>` | Enables or disable the SPS Service.<br><br>Notes:<br>Can be stored using [AT&W](#atw). |
| `AT+USPS?` | Read if the SPS service is enabled or disabled. |

| <div style="width:350px">Response</div> | Description |
| ----------|----------|
| `+USPS:<sps_service_option>` | Successful read response. |

**Defined values**<br>
| Parameter | Type | Description |
| ----------|----------|----------|
| sps\_service\_option | enumerator | Valid values:<br>0: This option disables the SPS service after saving the configuration and restarting the device. (Default)<br>1: This option enables the SPS service directly.<br>If this option is set, and the configuration is saved,<br>SPS will be enabled after reboot. |

#### Socket

### AT+USOTLS - Socket TLS


**Syntax**<br>
| <div style="width:350px">AT Command</div> | Description |
| ----------|----------|
| `AT+USOTLS=<socket_handle>,<tls_version>[,<ca_name>[,<client_cert_name>,<client_key_name>]]` | Add a TLS context to a socket. This is only valid for TCP client sockets. |
| `AT+USOTLS=<socket_handle>` | Get the TLS context information for a socket. |

| <div style="width:350px">Response</div> | Description |
| ----------|----------|
| `+USOTLS:<socket_handle>,<tls_version>,<ca_name>,<client_cert_name>,<client_key_name>` | Successful response with the TLS context information for the specified socket. The response includes the TLS version, CA name, client certificate name, and client key name. |

**Defined values**<br>
| Parameter | Type | Description |
| ----------|----------|----------|
| socket\_handle | integer | Socket identifier be used for any operation on that socket. |
| tls\_version | enumerator | TLS version to use<br><br>Valid values:<br>0: Disable TLS<br><br>1: TLS 1.2 ~~or up~~<br><br>**2: TLS 1.3**<br><br>**3: TLS 1.2 or 1.3 (negotiate highest)** |
| ca\_name | string | Name of the certificate authority (CA) certificate to use<br><br>Length: 1..32 |
| client\_cert\_name | string | Name of the client certificate to use<br><br>Length: 1..32 |
| client\_key\_name | string | Name of the private key for client certificate<br><br>Length: 1..32 |

#### System

### AT+USYLA - Local Address


**Syntax**<br>
| <div style="width:350px">AT Command</div> | Description |
| ----------|----------|
| `AT+USYLA=<interface_id>` | Get interface address |
| `AT+USYLA=<interface_id>,<address>` | Set interface address<br><br>Notes:<br>Can be stored using [AT&W](#atw). |

| <div style="width:350px">Response</div> | Description |
| ----------|----------|
| `+USYLA:<address>` | Successful read response |

**Defined values**<br>
| Parameter | Type | Description |
| ----------|----------|----------|
| interface\_id | enumerator | Valid values:<br>0: Bluetooth<br>1: Wi-Fi station<br>2: Wi-Fi Access point |
| address | mac_addr | MAC address of the interface id. If the address is set to 000000000000, the local address will be restored to factory-programmed value. The least significant bit of the first octet of the ~~<address>~~ **address** must be 0. |

#### Wi-Fi

### AT+UWSS - Wi-Fi Security Config

Read the current security parameter configuration, for Wi-Fi station

**Syntax**<br>
| <div style="width:350px">AT Command</div> | Description |
| ----------|----------|
| `AT+UWSS=<wlan_handle>` | Get the current Wi-Fi station security config |

| <div style="width:350px">Response</div> | Description |
| ----------|----------|
| `+UWSS:<wlan_handle>,<security_mode>,<wpa_threshold>` | Response if security mode is WPA |
| `+UWSS:<wlan_handle>,<security_mode>` | Response if security mode is Open |
| `+UWSS:<wlan_handle>,<security_mode>,<ca_name>,<client_cert_name>,<client_key_name>,<identity>` | Response if security mode is EAP-TLS. Emitted when security was configured<br>using AT+UWSSE without <tls_version> (DEPRECATED). Prefer configuring with<br><tls_version> so that the +UWSS variant including <tls_version> is emitted instead |
| `+UWSS:<wlan_handle>,<security_mode>,<username>,<ca_name>` | Response if security mode is PEAP. Emitted when security was configured<br>using AT+UWSSP without <tls_version> (DEPRECATED). Prefer configuring with<br><tls_version> so that the +UWSS variant including <tls_version> is emitted instead |
| **`+UWSS:<wlan_handle>,<security_mode>,<tls_version>,<ca_name>,<client_cert_name>,<client_key_name>,<identity>`** | **Response if security mode is EAP-TLS** |
| **`+UWSS:<wlan_handle>,<security_mode>,<tls_version>,<username>,<ca_name>`** | **Response if security mode is PEAP** |

**Defined values**<br>
| Parameter | Type | Description |
| ----------|----------|----------|
| wlan\_handle | integer | Handle to use for Wi-Fi config and connection<br><br>Valid values: 0..0 |
| security\_mode | enumerator | The current security mode.<br><br>Valid values:<br>0: Open security<br>1: WPA security<br>2: EAP-TLS security<br>3: PEAP security |
| wpa\_threshold | enumerator | Lowest WPA version to connect to<br><br>Valid values:<br>0: Only connect to access points that support WPA2 or up<br>1: Only connect to access points that support WPA3<br><br>Default value: 0 |
| ca\_name | string | Name of the certificate authority (CA) certificate to use<br><br>Length: 1..32 |
| client\_cert\_name | string | Name of the client certificate to use<br><br>Length: 1..32 |
| client\_key\_name | string | Name of the private key for client certificate<br><br>Length: 1..32 |
| username | string | User name for PEAP authentication.<br><br>Length: 1..31 |
| identity | string | Identity for EAP-TLS<br><br>Length: 1..31 |
| **tls\_version** | **enumerator** | **TLS version to use<br><br>**Valid values:**<br>0: Disable TLS<br>1: TLS 1.2<br>2: TLS 1.3<br>3: TLS 1.2 or 1.3 (negotiate highest)** |

### AT+UWSSE - Wi-Fi Station Enterprise security

Configure enterprise security. Certificates must be uploaded to the module before being used in this command, see AT+USECUB

**Syntax**<br>
| <div style="width:350px">AT Command</div> | Description |
| ----------|----------|
| `AT+UWSSE=<wlan_handle>,<ca_name>,<client_cert_name>,<client_key_name>[,<identity>]` | Set the EAP-TLS connection parameters to use. (DEPRECATED - Use the version with tls_version parameter instead)<br><br>Notes:<br>Can be stored using [AT&W](#atw). |
| **`AT+UWSSE=<wlan_handle>,<tls_version>,<ca_name>,<client_cert_name>,<client_key_name>[,<identity>]`** | **Set the EAP-TLS connection parameters to use.<br><br>Notes:<br>Can be stored using [AT&W](#atw).** |

**Defined values**<br>
| Parameter | Type | Description |
| ----------|----------|----------|
| wlan\_handle | integer | Handle to use for Wi-Fi config and connection<br><br>Valid values: 0..0 |
| ca\_name | string | Name of the certificate authority (CA) certificate to use<br><br>Length: 1..32 |
| client\_cert\_name | string | Name of the client certificate to use<br><br>Length: 1..32 |
| client\_key\_name | string | Name of the private key for client certificate<br><br>Length: 1..32 |
| identity | string | Identity for EAP-TLS<br><br>Length: 1..31 |
| **tls\_version** | **enumerator** | **TLS version to use<br><br>**Valid values:**<br>0: Disable TLS<br>1: TLS 1.2<br>2: TLS 1.3<br>3: TLS 1.2 or 1.3 (negotiate highest)** |

### AT+UWSSP - Wi-Fi Station PEAP security

Configure PEAP security. CA certificate must be uploaded first if used here, see AT+USECUB

**Syntax**<br>
| <div style="width:350px">AT Command</div> | Description |
| ----------|----------|
| `AT+UWSSP=<wlan_handle>,<peap_user>,<peap_password>[,<ca_name>]` | Set the PEAP connection parameters to use. (DEPRECATED - Use the version with tls_version parameter instead)<br><br>Notes:<br>Can be stored using [AT&W](#atw). |
| **`AT+UWSSP=<wlan_handle>,<tls_version>,<peap_user>,<peap_password>[,<ca_name>]`** | **Set the PEAP connection parameters to use.<br><br>Notes:<br>Can be stored using [AT&W](#atw).** |

**Defined values**<br>
| Parameter | Type | Description |
| ----------|----------|----------|
| wlan\_handle | integer | Handle to use for Wi-Fi config and connection<br><br>Valid values: 0..0 |
| peap\_user | string | User name for PEAP authentication. Could be either only username or username@domain. Use @ as separator<br><br>Length: 1..31 |
| peap\_password | string | Password for PEAP authentication.<br><br>Length: 1..31 |
| ca\_name | string | Name of the certificate authority (CA) certificate to use<br><br>Length: 1..32 |
| **tls\_version** | **enumerator** | **TLS version to use<br><br>**Valid values:**<br>0: Disable TLS<br>1: TLS 1.2<br>2: TLS 1.3<br>3: TLS 1.2 or 1.3 (negotiate highest)** |

