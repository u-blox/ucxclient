ucxclient-x64 v1.0.0-beta.1
================================

Release Date: November 24, 2025
Platform: Windows x64

BETA RELEASE - For Testing Purposes
------------------------------------
This is a beta release for testing and validation. Not recommended for production use.

Contents:
---------
- ucxclient-x64-signed.exe - Signed application executable
  (includes embedded ftd2xx64.dll, auto-extracted on first run)

Requirements:
-------------
- Windows 10/11 (64-bit)
- FTDI USB-to-Serial driver installed
- u-blox NORA-W36 module or compatible device

Installation:
-------------
1. Copy ucxclient-x64-signed.exe to a directory of your choice
2. Ensure FTDI drivers are installed
3. Run ucxclient-x64-signed.exe (ftd2xx64.dll will be auto-extracted on first run)

Known Issues:
-------------
- Some cleanup and testing still in progress
- See GitHub issues for full list

Support:
--------
For issues and questions:
- GitHub: https://github.com/u-blox/ucxclient/issues
- Branch: cmag_win64_port

Changelog:
----------
Initial beta release with:
- Complete Windows port for NORA-W36
- WiFi configuration and management
- Bluetooth scanning and operations
- HTTP client functionality
- Firmware update support
- XMODEM file transfer
- Extended error code display
- Interactive menu-driven interface
