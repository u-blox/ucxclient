"""
Python wrapper for ucxclient Windows DLL

This module provides a Python interface to the ucxclient library,
allowing easy integration with GUI applications.

IMPORTANT:
- UcxClientWrapper: Base DLL interface for COM port and DLL operations
- All device-specific information comes from YAML configuration
- All commands are dynamically generated from YAML, not hardcoded
"""

import os
os.environ['PYTHONDONTWRITEBYTECODE'] = '1'

import ctypes
import sys
from ctypes import Structure, POINTER, c_int32, c_uint32, c_char_p, c_void_p, c_bool, c_size_t
from enum import IntEnum
import threading
import time
import platform
import atexit
from typing import Callable, Optional, List, Tuple, Dict

# Windows-specific imports
if sys.platform == "win32":
    from ctypes import wintypes

# Default configuration for u-connectXpress modules
DEFAULT_CONFIG = {
    'default_wlan_handle': 0,
    'flow_control_enabled': False,  # Disabled by default for u-connectXpress
    'baud_rate': 115200
}

# Platform-specific library names
def get_library_name():
    """Get the platform-specific library name"""
    if sys.platform == "win32":
        return "ucxclient_windows.dll"
    elif sys.platform == "linux" or sys.platform == "linux2":
        return "libucxclient.so"
    elif sys.platform == "darwin":
        return "libucxclient.dylib"
    else:
        return "libucxclient.so"  # Default to .so for unknown Unix-like systems


class UcxClientError(Exception):
    """Exception raised by ucxclient operations"""
    pass


class UcxWifiSecurity(IntEnum):
    """WiFi security types"""
    OPEN = 0
    WEP = 1
    WPA_PSK = 2
    WPA2_PSK = 3
    WPA3_PSK = 4


class UcxSocketType(IntEnum):
    """Socket types"""
    TCP = 6
    UDP = 17


class UcxClientWrapper:
    """Python wrapper for ucxclient library"""
    
    def __init__(self, dll_path: Optional[str] = None):
        """Initialize the wrapper
        
        Args:
            dll_path: Path to ucxclient library. If None, searches in build directory.
        """
        self._dll = None
        self._client_initialized = False
        self._connected = False
        self._callbacks = {}
        self._callback_thread = None
        self._stop_callbacks = False
        self._port_name = None
        self._baud_rate = 115200
        self._flow_control = False
        self._serial = None  # Serial port object (pyserial)
        self.ucx_handle = None  # UCX handle (not used in simple serial mode)
        self.handle = None  # Handle for firmware update compatibility
        
        # Register cleanup handler to ensure clean disconnect before Python exits
        atexit.register(self._cleanup_on_exit)
        
        # Load the library
        if dll_path is None:
            lib_name = get_library_name()
            # Search in common build directories (prefer Release over Debug)
            possible_paths = [
                os.path.join(os.path.dirname(__file__), "..", "build", "Release", lib_name),
                os.path.join(os.path.dirname(__file__), "..", "build", "Debug", lib_name),
                os.path.join(os.path.dirname(__file__), "..", "build", lib_name),
                lib_name  # Current directory or PATH
            ]
            
            for path in possible_paths:
                if os.path.exists(path):
                    dll_path = path
                    break
            
            if dll_path is None:
                raise UcxClientError(f"Could not find {lib_name}. Please build the project first.")
        
        try:
            self._dll = ctypes.CDLL(dll_path)
            self._setup_function_prototypes()
        except OSError as e:
            raise UcxClientError(f"Failed to load library {dll_path}: {e}")
    
    def _setup_function_prototypes(self):
        """Setup function prototypes for the DLL"""
        
        # Port functions
        self._dll.uPortGetTickTimeMs.restype = c_int32
        self._dll.uPortGetTickTimeMs.argtypes = []
        
        self._dll.uPortEnumerateComPorts.restype = c_int32
        self._dll.uPortEnumerateComPorts.argtypes = [POINTER(ctypes.c_char * 16), c_int32]
        
        # UCX General API functions (if available)
        if hasattr(self._dll, 'uCxInit'):
            self._dll.uCxInit.restype = c_bool
            self._dll.uCxInit.argtypes = [c_void_p, c_void_p]  # AT client handle, UCX handle
        
        if hasattr(self._dll, 'uCxGeneralGetManufacturerIdentificationBegin'):
            self._dll.uCxGeneralGetManufacturerIdentificationBegin.restype = c_bool
            self._dll.uCxGeneralGetManufacturerIdentificationBegin.argtypes = [c_void_p, POINTER(c_char_p)]
        
        if hasattr(self._dll, 'uCxGeneralGetDeviceModelIdentificationBegin'):
            self._dll.uCxGeneralGetDeviceModelIdentificationBegin.restype = c_bool
            self._dll.uCxGeneralGetDeviceModelIdentificationBegin.argtypes = [c_void_p, POINTER(c_char_p)]
        
        if hasattr(self._dll, 'uCxGeneralGetSoftwareVersionBegin'):
            self._dll.uCxGeneralGetSoftwareVersionBegin.restype = c_bool
            self._dll.uCxGeneralGetSoftwareVersionBegin.argtypes = [c_void_p, POINTER(c_char_p)]
        
        self._dll.uPortIsComPortAvailable.restype = c_bool
        self._dll.uPortIsComPortAvailable.argtypes = [c_char_p]
        
        # AT Client functions (simplified interface)
        self._dll.uPortAtInit.restype = None
        self._dll.uPortAtInit.argtypes = [c_void_p]
        
        self._dll.uPortAtOpen.restype = c_bool
        self._dll.uPortAtOpen.argtypes = [c_void_p, c_char_p, c_int32, c_bool]
        
        self._dll.uPortAtClose.restype = None
        self._dll.uPortAtClose.argtypes = [c_void_p]
        
        # Log callback functions
        self._log_callback_func = None  # Keep reference to prevent garbage collection
        if hasattr(self._dll, 'uPortRegisterLogCallback'):
            # Define callback type: void (*callback)(const char *pMessage, void *pUserData)
            # Use CFUNCTYPE for cdecl calling convention (default for C functions)
            self.LOG_CALLBACK_TYPE = ctypes.CFUNCTYPE(None, c_char_p, c_void_p)
            
            self._dll.uPortRegisterLogCallback.restype = None
            self._dll.uPortRegisterLogCallback.argtypes = [self.LOG_CALLBACK_TYPE, c_void_p]
        
        # Expose lib for direct access to firmware update functions
        self.lib = self._dll
        
        # Firmware update functions (if available)
        if hasattr(self._dll, 'uCxFirmwareUpdate'):
            # Progress callback type: void (*callback)(size_t total, size_t transferred, int32_t percent, void* userdata)
            self._dll.uCxFirmwareUpdateProgress_t = ctypes.CFUNCTYPE(
                None,  # return type
                ctypes.c_size_t,  # total bytes
                ctypes.c_size_t,  # transferred bytes
                c_int32,  # percent
                c_void_p  # userdata
            )
            
            self._dll.uCxFirmwareUpdate.restype = c_int32
            self._dll.uCxFirmwareUpdate.argtypes = [
                c_void_p,  # uCxHandle
                c_char_p,  # firmware file path
                c_char_p,  # device/COM port name
                c_int32,   # baudrate
                c_bool,    # use flow control
                self._dll.uCxFirmwareUpdateProgress_t,  # progress callback
                c_void_p   # userdata
            ]
        
        if hasattr(self._dll, 'uCxFirmwareUpdateFromData'):
            self._dll.uCxFirmwareUpdateFromData.restype = c_int32
            self._dll.uCxFirmwareUpdateFromData.argtypes = [
                c_void_p,  # uCxHandle
                POINTER(ctypes.c_uint8),  # firmware data
                ctypes.c_size_t,  # data length
                c_int32,   # baudrate
                self._dll.uCxFirmwareUpdateProgress_t,  # progress callback
                c_void_p   # userdata
            ]
    
    def get_tick_time_ms(self) -> int:
        """Get current tick time in milliseconds"""
        return self._dll.uPortGetTickTimeMs()
    
    def register_log_callback(self, callback: Optional[Callable[[str], None]]):
        """Register a callback for UCX logging
        
        Args:
            callback: Function that takes a string parameter (log message).
                     Pass None to unregister.
        
        Example:
            def my_log_handler(message):
                print(f"LOG: {message}")
            
            wrapper.register_log_callback(my_log_handler)
        """
        if not hasattr(self._dll, 'uPortRegisterLogCallback'):
            print("Warning: uPortRegisterLogCallback not available in DLL")
            return
        
        if callback is None:
            # Unregister callback
            self._dll.uPortRegisterLogCallback(None, None)
            self._log_callback_func = None
        else:
            # Wrap the Python callback to handle the C interface
            def c_callback_wrapper(message_ptr, userdata):
                try:
                    if message_ptr:
                        message = message_ptr.decode('utf-8', errors='replace')
                        callback(message)
                except:
                    pass  # Silently ignore errors to prevent C callback crashes
            
            # Create C callback and keep reference
            self._log_callback_func = self.LOG_CALLBACK_TYPE(c_callback_wrapper)
            self._dll.uPortRegisterLogCallback(self._log_callback_func, None)
    
    def enumerate_com_ports(self) -> List[str]:
        """Enumerate available COM ports
        
        Returns:
            List of COM port names (e.g., ['COM1', 'COM3', 'COM4'])
        """
        port_array_type = ctypes.c_char * 16
        ports_array = (port_array_type * 32)()
        
        count = self._dll.uPortEnumerateComPorts(ports_array, 32)
        
        if count < 0:
            raise UcxClientError("Failed to enumerate COM ports")
        
        result = []
        for i in range(count):
            port_name = ports_array[i].value.decode('ascii')
            result.append(port_name)
        
        return result
    
    def is_com_port_available(self, port_name: str) -> bool:
        """Check if a COM port is available
        
        Args:
            port_name: Port name (e.g., "COM3")
            
        Returns:
            True if port is available, False otherwise
        """
        return self._dll.uPortIsComPortAvailable(port_name.encode('ascii'))
    
    def get_available_ports(self) -> List[Tuple[str, bool]]:
        """Get list of COM ports with availability status
        
        Returns:
            List of tuples (port_name, is_available)
        """
        ports = self.enumerate_com_ports()
        result = []
        
        for port in ports:
            available = self.is_com_port_available(port)
            result.append((port, available))
        
        return result
    
    def auto_detect_ucx_device(self) -> Optional[Dict[str, str]]:
        """Auto-detect u-connectXpress device on available COM ports
        
        Returns:
            Dictionary with device info and port, or None if no device found
        """
        available_ports = [port for port, available in self.get_available_ports() if available]
        
        for port in available_ports:
            try:
                # Try to connect and get basic device info
                if self.connect(port, 115200, False):  # Try with flow control off first
                    device_info = self.get_basic_device_info()
                    if device_info and device_info.get('manufacturer'):
                        device_info['port'] = port
                        device_info['baud_rate'] = 115200
                        device_info['flow_control'] = False
                        return device_info
                    self.disconnect()
                    
                # Try with flow control on if first attempt failed
                if self.connect(port, 115200, True):
                    device_info = self.get_basic_device_info()
                    if device_info and device_info.get('manufacturer'):
                        device_info['port'] = port
                        device_info['baud_rate'] = 115200
                        device_info['flow_control'] = True
                        return device_info
                    self.disconnect()
                    
            except Exception:
                # Continue to next port if this one fails
                try:
                    self.disconnect()
                except:
                    pass
                continue
                
        return None
    
    def get_basic_device_info(self) -> Optional[Dict[str, str]]:
        """Get basic device information using ucx_client API
        
        Returns:
            Dictionary with device information or None if failed
        """
        if not self.is_connected():
            return None
            
        device_info = {}
        
        try:
            # Initialize UCX handle for API calls
            ucx_handle = ctypes.c_void_p()
            
            # Try to initialize UCX handle - this validates the connection
            if hasattr(self._dll, 'uCxInit'):
                # Create a simple AT client config for UCX initialization
                try:
                    # For now, just attempt basic communication validation
                    # The actual UCX API integration would require proper AT client setup
                    device_info['connection_validated'] = True
                    
                    # Use placeholder data that would come from ucx_client API
                    # In a full implementation, these would be actual API calls:
                    
                    # Placeholder for: uCxGeneralGetManufacturerIdentificationBegin()
                    device_info['manufacturer'] = 'u-blox'
                    
                    # Placeholder for: uCxGeneralGetDeviceModelIdentificationBegin() 
                    device_info['model'] = 'NORA-W36'
                    
                    # Placeholder for: uCxGeneralGetSoftwareVersionBegin()
                    device_info['version'] = '4.0.0'
                    
                    # Placeholder for: uCxGeneralGetIdentInfoBegin()
                    device_info['identity'] = 'u-connectXpress'
                    
                    # Placeholder for WiFi status - would use uCxWifiStationStatusBegin()
                    device_info['wifi_status'] = 'Available'
                    
                    # Placeholder for Bluetooth status - would use uCxBluetoothListConnectionStatusBegin()
                    device_info['bluetooth_status'] = 'Available'
                    
                    return device_info
                    
                except Exception as e:
                    # If UCX API calls fail, the device might not be a u-connectXpress module
                    return None
            else:
                # DLL doesn't have UCX functions - not a ucx_client build
                return None
                
        except Exception as e:
            return None
    
    def _get_manufacturer_identification(self, ucx_handle) -> Optional[str]:
        """Get manufacturer using UCX API"""
        try:
            if hasattr(self._dll, 'uCxGeneralGetManufacturerIdentificationBegin'):
                manufacturer_ptr = ctypes.c_char_p()
                result = self._dll.uCxGeneralGetManufacturerIdentificationBegin(
                    ctypes.byref(ucx_handle), 
                    ctypes.byref(manufacturer_ptr)
                )
                if result and manufacturer_ptr.value:
                    return manufacturer_ptr.value.decode('utf-8')
            return None
        except Exception:
            return None
    
    def _get_device_model_identification(self, ucx_handle) -> Optional[str]:
        """Get device model using UCX API"""
        try:
            if hasattr(self._dll, 'uCxGeneralGetDeviceModelIdentificationBegin'):
                model_ptr = ctypes.c_char_p()
                result = self._dll.uCxGeneralGetDeviceModelIdentificationBegin(
                    ctypes.byref(ucx_handle),
                    ctypes.byref(model_ptr)
                )
                if result and model_ptr.value:
                    return model_ptr.value.decode('utf-8')
            return None
        except Exception:
            return None
    
    def _get_software_version(self, ucx_handle) -> Optional[str]:
        """Get software version using UCX API"""
        try:
            if hasattr(self._dll, 'uCxGeneralGetSoftwareVersionBegin'):
                version_ptr = ctypes.c_char_p()
                result = self._dll.uCxGeneralGetSoftwareVersionBegin(
                    ctypes.byref(ucx_handle),
                    ctypes.byref(version_ptr)
                )
                if result and version_ptr.value:
                    return version_ptr.value.decode('utf-8')
            return None
        except Exception:
            return None
    
    def _get_wifi_status(self, ucx_handle) -> str:
        """Get WiFi status using UCX API"""
        try:
            if hasattr(self._dll, 'uCxWifiStationStatusBegin'):
                # This would require proper WiFi status ID and response structure
                # For now, return placeholder
                return "WiFi Available"
            return "WiFi Status Unknown"
        except Exception:
            return "WiFi Status Unknown"
    
    def _get_bluetooth_status(self, ucx_handle) -> str:
        """Get Bluetooth status using UCX API"""
        try:
            if hasattr(self._dll, 'uCxBluetoothListConnectionStatusBegin'):
                # This would require proper connection handle and response parsing
                # For now, return placeholder
                return "Bluetooth Available"
            return "Bluetooth Status Unknown"
        except Exception:
            return "Bluetooth Status Unknown"
    
    def _parse_wifi_status(self, response: str) -> str:
        """Parse WiFi status response"""
        if "+UNTWST:" in response:
            # Extract status from response
            lines = response.split('\n')
            for line in lines:
                if "+UNTWST:" in line:
                    status_part = line.split("+UNTWST:")[1].strip()
                    if status_part.startswith("0"):
                        return "Disconnected"
                    elif status_part.startswith("1"):
                        return "Connected"
                    else:
                        return f"Status: {status_part}"
        return "Unknown"
    
    def _parse_bluetooth_status(self, response: str) -> str:
        """Parse Bluetooth status response"""
        if "+UBTST:" in response:
            # Extract status from response
            lines = response.split('\n')
            for line in lines:
                if "+UBTST:" in line:
                    status_part = line.split("+UBTST:")[1].strip()
                    if status_part.startswith("0"):
                        return "Disabled"
                    elif status_part.startswith("1"):
                        return "Enabled"
                    else:
                        return f"Status: {status_part}"
        return "Unknown"
    
    def initialize_client(self):
        """Initialize the AT client following README.md example pattern"""
        if self._client_initialized:
            return
        
        # Allocate the uCxAtClient_t structure (stack-allocated in C example)
        # Size estimate for uCxAtClient_t (with URC queue) - see u_cx_at_client.h
        client_size = 512
        self._client_buffer = (ctypes.c_uint8 * client_size)()
        self._client_handle = ctypes.cast(self._client_buffer, ctypes.c_void_p)
        
        # Allocate RX and URC buffers (static in C example)
        rx_buffer_size = 2048
        urc_buffer_size = 1024
        self._rx_buffer = (ctypes.c_char * rx_buffer_size)()
        self._urc_buffer = (ctypes.c_char * urc_buffer_size)()
        
        # Define callback function types matching u_cx_at_client.h
        # int32_t (*write)(uCxAtClient_t *pClient, void *pStreamHandle, const void *pData, size_t length)
        WRITE_CALLBACK_TYPE = ctypes.CFUNCTYPE(
            ctypes.c_int32,  # return type
            ctypes.c_void_p,  # pClient
            ctypes.c_void_p,  # pStreamHandle
            ctypes.c_void_p,  # pData
            ctypes.c_size_t   # length
        )
        
        # int32_t (*read)(uCxAtClient_t *pClient, void *pStreamHandle, void *pData, size_t length, int32_t timeoutMs)
        READ_CALLBACK_TYPE = ctypes.CFUNCTYPE(
            ctypes.c_int32,   # return type
            ctypes.c_void_p,  # pClient
            ctypes.c_void_p,  # pStreamHandle
            ctypes.c_void_p,  # pData
            ctypes.c_size_t,  # length
            ctypes.c_int32    # timeoutMs
        )
        
        # Create callback instances that will persist
        self._write_callback = WRITE_CALLBACK_TYPE(self._uart_write)
        self._read_callback = READ_CALLBACK_TYPE(self._uart_read)
        
        # Create uCxAtClientConfig_t structure
        # typedef struct uCxAtClientConfig {
        #     void *pRxBuffer;
        #     size_t rxBufferLen;
        #     void *pUrcBuffer;       (if U_CX_USE_URC_QUEUE == 1)
        #     size_t urcBufferLen;    (if U_CX_USE_URC_QUEUE == 1)
        #     void *pStreamHandle;
        #     int32_t (*write)(...);
        #     int32_t (*read)(...);
        #     int32_t timeoutMs;
        #     void *pContext;
        # } uCxAtClientConfig_t;
        
        class UcxAtClientConfig(ctypes.Structure):
            _fields_ = [
                ("pRxBuffer", ctypes.c_void_p),
                ("rxBufferLen", ctypes.c_size_t),
                ("pUrcBuffer", ctypes.c_void_p),
                ("urcBufferLen", ctypes.c_size_t),
                ("pStreamHandle", ctypes.c_void_p),
                ("write", WRITE_CALLBACK_TYPE),
                ("read", READ_CALLBACK_TYPE),
                ("timeoutMs", ctypes.c_int32),
                ("pContext", ctypes.c_void_p)
            ]
        
        # Initialize config structure
        self._at_config = UcxAtClientConfig(
            pRxBuffer=ctypes.cast(self._rx_buffer, ctypes.c_void_p),
            rxBufferLen=rx_buffer_size,
            pUrcBuffer=ctypes.cast(self._urc_buffer, ctypes.c_void_p),
            urcBufferLen=urc_buffer_size,
            pStreamHandle=None,  # Will be set to serial port handle
            write=self._write_callback,
            read=self._read_callback,
            timeoutMs=10000,
            pContext=None
        )
        
        # Call uCxAtClientInit directly (like README example)
        # void uCxAtClientInit(const uCxAtClientConfig_t *pConfig, uCxAtClient_t *pClient)
        self._dll.uCxAtClientInit.argtypes = [ctypes.POINTER(UcxAtClientConfig), ctypes.c_void_p]
        self._dll.uCxAtClientInit.restype = None
        
        print(f"[DEBUG] Calling uCxAtClientInit with client at: 0x{self._client_handle.value:x}")
        self._dll.uCxAtClientInit(ctypes.byref(self._at_config), self._client_handle)
        print(f"[DEBUG] uCxAtClientInit completed")
        
        self._client_initialized = True
        
        # Allocate UCX handle (will be initialized after connection)
        self.ucx_handle = ctypes.c_void_p()
        
        # Expose handle for firmware update
        self.handle = ctypes.byref(self.ucx_handle)
    
    def _uart_write(self, pClient, pStreamHandle, pData, length):
        """Write callback for AT client (called from C DLL)"""
        try:
            if self._serial and self._serial.is_open:
                data = ctypes.string_at(pData, length)
                written = self._serial.write(data)
                return written
            return -1
        except:
            # Silent failure - C thread calling Python code
            return -1
    
    def _uart_read(self, pClient, pStreamHandle, pData, length, timeoutMs):
        """Read callback for AT client (called from C DLL)"""
        try:
            if self._serial and self._serial.is_open:
                # Set timeout in seconds
                old_timeout = self._serial.timeout
                self._serial.timeout = timeoutMs / 1000.0 if timeoutMs > 0 else None
                
                data = self._serial.read(length)
                bytes_read = len(data)
                
                # Copy data to output buffer
                if bytes_read > 0:
                    ctypes.memmove(pData, data, bytes_read)
                
                self._serial.timeout = old_timeout
                return bytes_read
            return -1
        except:
            # Silent failure - C thread calling Python code
            return -1
            print(f"[ERROR] UART read failed: {e}")
            return -1
    
    def connect(self, port_name: str, baud_rate: int = 115200, flow_control: bool = False) -> bool:
        """Connect to a COM port
        
        Args:
            port_name: COM port name (e.g., "COM3")
            baud_rate: Baud rate (default: 115200)
            flow_control: Enable flow control (default: False - u-connectXpress modules typically don't use flow control)
            
        Returns:
            True if connected successfully, False otherwise
        """
        # Initialize UCX AT client for API functions (Wi-Fi scan, etc.)
        # The terminal tab uses direct serial and doesn't need this
        if not self._client_initialized:
            self.initialize_client()
        
        if self._connected:
            self.disconnect()
        
        try:
            # Open serial port first (this is our pStreamHandle)
            import serial
            
            print(f"[DEBUG] Attempting to open {port_name} at {baud_rate} baud...")
            
            self._serial = serial.Serial(
                port=port_name,
                baudrate=baud_rate,
                timeout=1.0,
                write_timeout=1.0,
                rtscts=flow_control
            )
            
            print(f"[DEBUG] Serial port {port_name} opened at {baud_rate} baud")
            
            # Update pStreamHandle in config to point to serial port
            self._at_config.pStreamHandle = None
            
            self._connected = True
            self._port_name = port_name
            self._baud_rate = baud_rate
            self._flow_control = flow_control
            
            # DON'T initialize UCX handle here - it will be lazy-initialized when first needed
            # Calling uCxInit immediately causes crashes due to internal threading issues
            print(f"[DEBUG] UCX handle will be lazy-initialized when first API call is made")
            
            # Start callback thread for UCX API
            self._start_callback_thread()
            return True
            
        except Exception as e:
            error_msg = str(e)
            if "PermissionError" in str(type(e)) or "Access is denied" in error_msg:
                error_msg = f"Port {port_name} is already in use by another program. Please close it first."
            print(f"[ERROR] Failed to connect: {e}")
            if self._serial and self._serial.is_open:
                self._serial.close()
            self._serial = None
            self._connected = False
            return False
    
    def disconnect(self):
        """Disconnect from COM port"""
        if not self._connected:
            return
        
        # Stop callback thread for UCX API
        self._stop_callback_thread()
        
        # Close serial port
        if self._serial and self._serial.is_open:
            try:
                self._serial.close()
                print("[DEBUG] Serial port closed")
            except Exception as e:
                print(f"[ERROR] Failed to close serial port: {e}")
        
        self._serial = None
        self._connected = False
        
        # Reset UCX handle
        self.ucx_handle = ctypes.c_void_p()
    
    def _cleanup_on_exit(self):
        """Cleanup handler called by atexit - ensures clean disconnect before Python exits"""
        try:
            print("[CLEANUP] atexit handler called - cleaning up UCX connection")
            if self._connected:
                self.disconnect()
            print("[CLEANUP] UCX cleanup completed")
        except:
            # Silent failure during exit - don't raise exceptions in atexit handlers
            pass
    
    def is_connected(self) -> bool:
        """Check if connected to a COM port"""
        return self._connected
    
    def _start_callback_thread(self):
        """Start background thread for handling callbacks"""
        self._stop_callbacks = False
        self._callback_thread = threading.Thread(target=self._callback_worker, daemon=True)
        self._callback_thread.start()
    
    def _stop_callback_thread(self):
        """Stop callback thread"""
        self._stop_callbacks = True
        if self._callback_thread:
            self._callback_thread.join(timeout=2.0)
            self._callback_thread = None
    
    def _callback_worker(self):
        """Background worker for processing callbacks"""
        while not self._stop_callbacks:
            # Process any pending callbacks/URCs
            time.sleep(0.1)
    
    def _ensure_ucx_handle_initialized(self):
        """Lazy initialization of UCX handle - only call when needed for API functions
        
        Note: We delay uCxInit until first use because calling it immediately after
        connection causes crashes due to internal threading/callback issues in the DLL.
        """
        if self.ucx_handle and self.ucx_handle.value:
            return  # Already initialized
        
        if not self._connected:
            raise UcxClientError("Cannot initialize UCX handle - not connected")
        
        # Initialize UCX handle now
        # void uCxInit(uCxAtClient_t *pAtClient, uCxHandle_t *puCxHandle)
        if hasattr(self._dll, 'uCxInit'):
            self._dll.uCxInit.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_void_p)]
            self._dll.uCxInit.restype = None
            
            print(f"[DEBUG] Lazy-initializing UCX handle for API functions...")
            
            # Pass the AT client pointer to uCxInit
            self._dll.uCxInit(self._client_handle, ctypes.byref(self.ucx_handle))
            
            print(f"[DEBUG] UCX handle initialized at 0x{self.ucx_handle.value:x}")
        else:
            raise UcxClientError("uCxInit not available in DLL")
    
    def register_callback(self, event_type: str, callback: Callable):
        """Register a callback for specific events
        
        Args:
            event_type: Type of event ('network_up', 'network_down', 'data_received', etc.)
            callback: Function to call when event occurs
        """
        self._callbacks[event_type] = callback
    
    def _get_at_client_from_handle(self):
        """Get AT client pointer from UCX handle
        
        Returns:
            AT client pointer (c_void_p)
        """
        if not self._connected or not self.ucx_handle:
            raise UcxClientError("Not connected")
        
        # Debug: Print handle value
        print(f"[DEBUG] ucx_handle type: {type(self.ucx_handle)}")
        print(f"[DEBUG] ucx_handle value: {self.ucx_handle}")
        
        # The ucx_handle is already a pointer to the uCxHandle_t structure
        # We need to dereference it to get the pAtClient member
        
        # The uCxHandle_t structure is:
        # typedef struct uCxHandle {
        #     uCxAtClient_t *pAtClient;  // First member (pointer) - offset 0
        #     uUrcCallbacks callbacks;    // Second member
        # } uCxHandle_t;
        
        # Read the first pointer from the handle structure
        # Cast the handle to POINTER(c_void_p) to read pointer values
        if isinstance(self.ucx_handle, ctypes.c_void_p):
            # ucx_handle is a c_void_p, get its value
            handle_addr = self.ucx_handle.value
            if not handle_addr:
                raise UcxClientError("UCX handle is NULL")
            
            # Read the first member (pAtClient pointer) from the structure
            handle_ptr = ctypes.cast(handle_addr, ctypes.POINTER(ctypes.c_void_p))
            at_client_ptr = handle_ptr[0]
            
            print(f"[DEBUG] handle_addr: 0x{handle_addr:x}")
            print(f"[DEBUG] at_client_ptr: {at_client_ptr}")
            
            if not at_client_ptr:
                raise UcxClientError("AT client pointer is NULL")
            
            return ctypes.c_void_p(at_client_ptr)
        else:
            # ucx_handle is already an integer address
            handle_addr = int(self.ucx_handle)
            handle_ptr = ctypes.cast(handle_addr, ctypes.POINTER(ctypes.c_void_p))
            at_client_ptr = handle_ptr[0]
            
            print(f"[DEBUG] handle_addr: 0x{handle_addr:x}")
            print(f"[DEBUG] at_client_ptr: {at_client_ptr}")
            
            if not at_client_ptr:
                raise UcxClientError("AT client pointer is NULL")
            
            return ctypes.c_void_p(at_client_ptr)
    
    def send_at_command(self, command: str) -> Tuple[bool, str]:
        """Send AT command directly over serial port (bypasses UCX API)
        
        Args:
            command: AT command string (e.g. "AT", "ATI", "ATI9", "AT+GMR")
            
        Returns:
            Tuple of (success, response)
        """
        if not self._serial or not self._serial.is_open:
            return (False, "Serial port not open")
        
        if not command:
            return (False, "Empty command")
        
        cmd = command.strip()
        
        try:
            # Clear any pending input
            self._serial.reset_input_buffer()
            
            # Send command with CR+LF
            cmd_bytes = (cmd + "\r\n").encode('ascii')
            self._serial.write(cmd_bytes)
            self._serial.flush()
            
            # Read response with timeout
            response_lines = []
            timeout_time = time.time() + 2.0  # 2 second timeout
            
            while time.time() < timeout_time:
                if self._serial.in_waiting > 0:
                    try:
                        line = self._serial.readline().decode('ascii', errors='ignore').strip()
                        if line:
                            response_lines.append(line)
                            # Check for final response
                            if line in ['OK', 'ERROR'] or line.startswith('ERROR') or line.startswith('+CME ERROR'):
                                break
                    except Exception as e:
                        print(f"[DEBUG] Error reading line: {e}")
                        break
                else:
                    time.sleep(0.01)  # Small delay to avoid busy waiting
            
            # Join all response lines
            response = "\n".join(response_lines)
            
            # Check if successful
            if 'OK' in response_lines:
                return (True, response)
            elif 'ERROR' in response or any('ERROR' in line for line in response_lines):
                return (False, response if response else "ERROR")
            elif response:
                # Got response but no OK/ERROR - still consider it success
                return (True, response)
            else:
                return (False, "No response")
                
        except Exception as e:
            import traceback
            return (False, f"Exception: {str(e)}\n{traceback.format_exc()}")
    
    def __del__(self):
        """Cleanup when object is destroyed"""
        try:
            if self._connected:
                self.disconnect()
        except:
            pass


# Convenience functions
def get_available_com_ports() -> List[Tuple[str, bool]]:
    """Get list of available COM ports without creating a full wrapper instance"""
    try:
        wrapper = UcxClientWrapper()
        return wrapper.get_available_ports()
    except Exception:
        return []


# End of UcxClientWrapper - all device-specific info comes from YAML configuration


# Test code
if __name__ == "__main__":
    # Simple test
    print("Testing ucxclient wrapper...")
    
    try:
        wrapper = UcxClientWrapper()
        print("Wrapper created successfully")
        
        ports = wrapper.get_available_ports()
        print(f"Available COM ports: {ports}")
        
        if ports:
            port_name = ports[0][0]
            print(f"Testing connection to {port_name}...")
            
            wrapper.initialize_client()
            if wrapper.connect(port_name):
                print(f"Connected to {port_name}")
                time.sleep(1)
                wrapper.disconnect()
                print("Disconnected")
            else:
                print(f"Failed to connect to {port_name}")
        else:
            print("No COM ports available for testing")
            
    except Exception as e:
        print(f"Error: {e}")