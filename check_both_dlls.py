import sys
import os
sys.path.insert(0, r'c:\u-blox\ucxclient\windows_gui')
from extract_dll_functions import get_dll_exports_from_file

debug_dll = r'c:\u-blox\ucxclient\build\Debug\ucxclient_windows.dll'
release_dll = r'c:\u-blox\ucxclient\build\Release\ucxclient_windows.dll'

print("=" * 80)
print("CHECKING DEBUG DLL")
print("=" * 80)
debug_exports = get_dll_exports_from_file(debug_dll)
debug_firmware = [f for f in debug_exports if 'Firmware' in f or 'firmware' in f]
debug_xmodem = [f for f in debug_exports if 'modem' in f.lower() or 'Xmodem' in f]

print(f"\nTotal exports: {len(debug_exports)}")
print(f"Firmware functions: {len(debug_firmware)}")
for f in debug_firmware:
    print(f"  {f}")
print(f"XMODEM functions: {len(debug_xmodem)}")
for f in debug_xmodem:
    print(f"  {f}")

print("\n" + "=" * 80)
print("CHECKING RELEASE DLL")
print("=" * 80)
release_exports = get_dll_exports_from_file(release_dll)
release_firmware = [f for f in release_exports if 'Firmware' in f or 'firmware' in f]
release_xmodem = [f for f in release_exports if 'modem' in f.lower() or 'Xmodem' in f]

print(f"\nTotal exports: {len(release_exports)}")
print(f"Firmware functions: {len(release_firmware)}")
for f in release_firmware:
    print(f"  {f}")
print(f"XMODEM functions: {len(release_xmodem)}")
for f in release_xmodem:
    print(f"  {f}")

print("\n" + "=" * 80)
print("COMPARISON")
print("=" * 80)
print(f"Debug DLL size: {os.path.getsize(debug_dll):,} bytes")
print(f"Release DLL size: {os.path.getsize(release_dll):,} bytes")
print(f"Debug timestamp: {os.path.getmtime(debug_dll)}")
print(f"Release timestamp: {os.path.getmtime(release_dll)}")

# Check if firmware files exist in source
import glob
firmware_c = glob.glob(r'c:\u-blox\ucxclient\ucx_api\*firmware*.c')
xmodem_c = glob.glob(r'c:\u-blox\ucxclient\src\*xmodem*.c')
print(f"\nSource files:")
print(f"  Firmware: {firmware_c}")
print(f"  XMODEM: {xmodem_c}")
