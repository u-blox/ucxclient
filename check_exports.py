import ctypes
import os

dll_path = r"c:\u-blox\ucxclient\build\Release\ucxclient_windows.dll"
dll = ctypes.CDLL(dll_path)

print(f"Checking exports from: {dll_path}")
print(f"DLL size: {os.path.getsize(dll_path):,} bytes\n")

functions_to_check = [
    'uCxFirmwareUpdate',
    'uCxFirmwareUpdateFromData', 
    'enterFirmwareUpdateMode',
    'uCxAtClientXmodemSend',
    'uCxAtClientXmodemReceive'
]

print("Checking for firmware/XMODEM functions:")
for func_name in functions_to_check:
    try:
        func = getattr(dll, func_name)
        print(f"  ✓ {func_name}: FOUND")
    except AttributeError:
        print(f"  ✗ {func_name}: NOT FOUND")

# Try to count total exports
print("\nTrying to enumerate all exports...")
import subprocess
try:
    # Try using objdump if available (from MinGW/MSYS)
    result = subprocess.run(['objdump', '-p', dll_path], capture_output=True, text=True, timeout=5)
    if result.returncode == 0:
        exports = [line for line in result.stdout.split('\n') if '\t' in line and not line.startswith('\t\t')]
        print(f"Total exports found: {len(exports)}")
except:
    print("Could not enumerate exports (objdump not available)")
