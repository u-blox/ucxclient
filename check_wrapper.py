import sys
import os

# Add the directory to path
sys.path.insert(0, r'c:\u-blox\ucxclient\windows_gui')

from ucx_wrapper import UcxClientWrapper

dll_path = r'c:\u-blox\ucxclient\build\Release\ucxclient_windows.dll'
wrapper = UcxClientWrapper(dll_path)

print("Checking wrapper attributes:")
print(f"  wrapper.lib exists: {hasattr(wrapper, 'lib')}")
print(f"  wrapper.lib is wrapper._dll: {wrapper.lib is wrapper._dll}")

print("\nChecking firmware functions on wrapper.lib:")
print(f"  hasattr(wrapper.lib, 'uCxFirmwareUpdate'): {hasattr(wrapper.lib, 'uCxFirmwareUpdate')}")
print(f"  hasattr(wrapper.lib, 'uCxFirmwareUpdateFromData'): {hasattr(wrapper.lib, 'uCxFirmwareUpdateFromData')}")
print(f"  hasattr(wrapper.lib, 'uCxFirmwareUpdateProgress_t'): {hasattr(wrapper.lib, 'uCxFirmwareUpdateProgress_t')}")

print("\nChecking firmware functions on wrapper._dll:")
print(f"  hasattr(wrapper._dll, 'uCxFirmwareUpdate'): {hasattr(wrapper._dll, 'uCxFirmwareUpdate')}")
print(f"  hasattr(wrapper._dll, 'uCxFirmwareUpdateFromData'): {hasattr(wrapper._dll, 'uCxFirmwareUpdateFromData')}")
print(f"  hasattr(wrapper._dll, 'uCxFirmwareUpdateProgress_t'): {hasattr(wrapper._dll, 'uCxFirmwareUpdateProgress_t')}")

# Try to access the function directly
try:
    func = wrapper.lib.uCxFirmwareUpdate
    print(f"\n✓ Successfully accessed uCxFirmwareUpdate: {func}")
except AttributeError as e:
    print(f"\n✗ Failed to access uCxFirmwareUpdate: {e}")

# Check if the hasattr check in ucx_wrapper.py passed
try:
    callback_type = wrapper.lib.uCxFirmwareUpdateProgress_t
    print(f"✓ Successfully accessed uCxFirmwareUpdateProgress_t: {callback_type}")
except AttributeError as e:
    print(f"✗ Failed to access uCxFirmwareUpdateProgress_t: {e}")
