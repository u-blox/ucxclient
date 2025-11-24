# Changelog
All notable changes to the u-connectXpress software relevant for `NORA-W36` will be documented in this file. For added or changed AT commands see at_commands_changes.md

# [3.1.0] - 2025-09-28
### Improvements
- Bluetooth: Support up to 23 characteristics with notification, indication (UCS_DEV_2-1529)
- Security: Support for TLS 1.3 added
- Security: Mbed TLS upgraded to version 3.6.4
    Note! Server Name Identification is now enforced by default during Server Authentication, even for TLS 1.2. This may cause problems if using an old certificate without the server name included. See AT+USETE0 for how to turn this off.
- MQTT: `AT+UMQRB` MQTT Read Binary now supports reading up to 5000 bytes (UCS_DEV_2-1460)

### Fixed
- HTTP client: Get Body feature not working with TLS and string mode (UCS_DEV_2-1509)
- HTTP client: Fixed POST truncation between requests by resetting contentLength after request completion (UCS_DEV_2-1524)
- Bluetooth: Crash if >8 characteristics used on one GATT service (UCS_DEV_2-1468)
- System: Escape sequence not working after AT+USYUS (UCS_DEV_2-1469)
- System: Escape sequence (+++) leaks as payload (UCS_DEV_2-1489)

### Known limitations
- Wi-Fi: Crash may occur when AP is brought down while station is associating (UCS_DEV_2-595)
- Network: TCP “No Delay” feature reduces throughput (UCS_DEV_2-833)
- Bluetooth: GATT read/write with authentication fails if bond not pre-established (UCS_DEV_2-851)
- System: Partial escape sequence never timeout. If you, in transparent mode, send a message that contains part of the escape sequence for leaving transparent mode, such as ++, this will not be transmitted to peer until yet another character is sent. (UCS_DEV_2-1490)
- SPS: Some SPS data sent in persistent mode will be lost when remote disconnects. Data sent over a persistent SPS link after the remote device stops acking BLE data until local device receives a BLE disconnect will be  lost. The window of lost data will be the BLE supervision timeout if for example the remote device is reset. (UCS_DEV_2-1487)
- Bluetooth: The longest advertisement data for extended advertisement is limited to 226 bytes. (UCS_DEV_2-1471)
- Bluetooth: Only one advertisement can by activated at the same time (UCS_DEV_2-1470)
- Security: TLS extension SNI (Server Name Identification) is not supported for EAP-TLS (UCS_DEV_2-1543)

---

## [3.0.0] - 2025-06-27
### Improvements
- Application: Added HTTP client (UCS_DEV_2-393)
- Network: Added `AT+USOB` (Socket Bind) command (UCS_DEV_2-1353)
- Network: Added support for Persistent Socket in `AT+USOL` and `AT+USOB` (UCS_DEV_2-1333)
- Network: Added SNTP support (UCS_DEV_2-1356)
- Security: Added `AT+USECD` (Security Certificates Details) command (UCS_DEV_2-1357)
- Wi-Fi: Added `AT+UWCL` (Wi-Fi Channel List) command (UCS_DEV_2-1177)

### Changed behavior
- Network: `AT+USOL` (Socket Listen) split into `AT+USOL` and `AT+USOB` (UCS_DEV_2-1419)
- Network: Changed response to `AT+USOPL?` (Socket Persistent List) (UCS_DEV_2-1363)
- Wi-Fi: Added more domains in `AT+UWRD` (Wi-Fi Regulatory Domains) (UCS_DEV_2-1206)
- Bluetooth: Renamed advertise commands:
  - `AT+UBTA` → `AT+UBTAL` (Legacy Enable)
  - `AT+UBTAD` → `AT+UBTADL` (Legacy Data)
  - `AT+UBTDA` → `AT+UBTAD` (Directed Advertisement)

### Fixed
- Bluetooth: Encryption not used when reconnecting to bonded device (UCS_DEV_2-1417)
- Application: Memory leak during MQTT publish (UCS_DEV_2-1429)
- Security: EAP-TLS connection failed with certificates >2048 bytes (UCS_DEV_2-812)
- Wi-Fi: Adaptivity not enabled (UCS_DEV_2-1330)
- Application: `AT+UMQPS` and `AT+UMQPB` returned OK too early (UCS_DEV_2-1362)
- Bluetooth: Module crash during SPS data transmission in buffered receive mode with `AT+USPSRM`(UCS_DEV_2-1347)
- Bluetooth: Missing support for SPS write without response and notification (UCS_DEV_2-1336)
- Wi-Fi: Wrong `authentication_suites` shown for EAP in scan results (UCS_DEV_2-1461)

### Known limitations
- Wi-Fi: Crash may occur when AP is brought down while station is associating (UCS_DEV_2-595)
- Network: TCP “No Delay” feature reduces throughput (UCS_DEV_2-833)
- Bluetooth: GATT read/write with authentication fails if bond not pre-established (UCS_DEV_2-851)
- Bluetooth: Crash if >8 characteristics used on one GATT service (UCS_DEV_2-1468)

---

## [2.0.0] - 2024-12-17
### Improvements
- Wi-Fi: Roaming settings `AT+UWSROS` can be changed without reboot.
- Bluetooth: AT+UBTB disconnects the ACL connection after the bonding procedure is completed. If the ACL connection was established before the bonding procedure it will not be disconnected.
- Application: Added optional parameter to set Identity during EAP-TLS authentication in `AT+UWSSE` command.
- Application: Added support for backslash escape codes for special characters.
- Application: Added support for persistent UDP sockets in `AT+USOP` and `AT+USOPCR`.
- Application: Increased max simultaneous sockets from 3 → 6.

### Fixed
- Connection issue against some servers due to TLS handshake failure. (UCS_DEV_2-1111)
- Reset when sending LE notify/indicate to unsupported characteristic (UCS_DEV_2-1142)
- Strings without quotes wrongly accepted (UCS_DEV_2-1152)
- Incorrect `OK` response on exit from transparent mode (UCS_DEV_2-1187)
- AT+UTMP prevented entering command mode with escape sequence (UCS_DEV_2-1195)
- Baudrates higher than 115200 (AT+USYUS) and auto sleep (AT+UPMPSL=1) can be configured even if it is not a supported combination. This results in problems waking up from sleep mode (UCS_DEV_2-1196)
- Reset when using TCP (UCS_DEV_2-1200)
- Wi-Fi network down (+UEWSND) sometimes arrives before link down (+UEWLD) (UCS_DEV_2-1223)

### Known limitations
- Wi-Fi: Crash may occur when AP is brought down while station is associating (UCS_DEV_2-595)
- Security: EAP-TLS fails with certificates >2048 bytes (UCS_DEV_2-812)
- Network: TCP “No Delay” feature reduces throughput (UCS_DEV_2-833)
- Bluetooth: GATT read/write with authentication fails if bond not pre-established (UCS_DEV_2-851)

### Changed behavior
- All AT string parameters must now be enclosed in quotation marks.

---

## [1.2.0] - 2024-10-18
### Improvements
- Wi-Fi: Roaming between Wi-Fi access points based-on Received Signal Strength Indicator, RSSI.
- Application: Auto sleep mode for optimizing consumption depending on configuration and activity.

### Fixed
- Could accept unauthenticated bonding even when configured to demand authenticated bonding (UCS_DEV_2-1067)
- Ping fails if sent both from the module and to the module at the same time (UCS_DEV_2-1075)
- Wi-Fi station could end up with the wrong color LED if disconnect was sent during an ongoing connection attempt (UCS_DEV_2-1133)
- Throughput reduced over time when sending data in one direction over a TCP socket (UCS_DEV_2-1127)
- Module crashes during automatic reconnect to a Wi-Fi Access Point (UCS_DEV_2-1130)

### Known limitations
- Wi-Fi: Crash may occur when AP is brought down while station is associating (UCS_DEV_2-595)
- Security: EAP-TLS fails with certificates >2048 bytes (UCS_DEV_2-812)
- Network: TCP “No Delay” feature reduces throughput (UCS_DEV_2-833)
- Bluetooth: GATT read/write with authentication fails if bond not pre-established (UCS_DEV_2-851)
- Connection issue against some servers due to TLS handshake failure. (UCS_DEV_2-1111)

### Changed behavior

---

## [1.0.0] - 2024-05-15
### Improvements
- NORA-W36 introduced
### Known limitations
- Wi-Fi: Crash may occur when AP is brought down while station is associating (UCS_DEV_2-595)
- Security: EAP-TLS fails with certificates >2048 bytes (UCS_DEV_2-812)
- Network: TCP “No Delay” feature reduces throughput (UCS_DEV_2-833)
- Bluetooth: GATT read/write with authentication fails if bond not pre-established (UCS_DEV_2-851)
