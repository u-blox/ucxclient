"""
UCX API Command Executor

This module executes UCX API function calls based on AT command mappings.
No AT commands are sent directly - everything goes through the UCX API layer.
"""

import os
os.environ['PYTHONDONTWRITEBYTECODE'] = '1'

import ctypes
import gc  # Import at module level to avoid GC during import
from typing import Dict, List, Optional, Any, Tuple, Union
from dataclasses import dataclass
from datetime import datetime
from at_to_api_mapper import APIMapping, APICallType, at_to_api_mapper

def debug_print(message):
    """Print debug message with timestamp"""
    timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
    print(f"[{timestamp}] {message}")

@dataclass
class APICallResult:
    """Result of an API function call"""
    success: bool
    result_data: Any = None
    error_message: str = ""
    raw_response: str = ""

class UCXAPIExecutor:
    """Executes UCX API functions based on AT command mappings"""
    
    # URC Event flags (like C example)
    URC_FLAG_NETWORK_UP = (1 << 0)
    URC_FLAG_NETWORK_DOWN = (1 << 1)
    
    def __init__(self, ucx_wrapper):
        """Initialize with UCX wrapper that has the DLL loaded"""
        self.ucx_wrapper = ucx_wrapper
        self.dll = ucx_wrapper._dll
        self.ucx_handle = ctypes.c_void_p()
        
        # URC event flags
        self.urc_event_flags = 0
        
        # Keep references to callback functions to prevent garbage collection
        self._network_up_callback = None
        self._network_down_callback = None
        
        self._setup_api_prototypes()
        self._initialize_ucx_handle()
        self._register_urc_callbacks()
    
    def _setup_api_prototypes(self):
        """Setup UCX API function prototypes"""
        # This would be extended based on available API functions
        api_functions = [
            "uCxInit",
            "uCxGeneralGetManufacturerIdentificationBegin",
            "uCxGeneralGetDeviceModelIdentificationBegin", 
            "uCxGeneralGetSoftwareVersionBegin",
            "uCxGeneralGetIdentInfoBegin",
            "uCxWifiStationScanDefaultBegin",
            "uCxWifiStationConnectBegin",
            "uCxWifiStationDisconnectBegin",
            "uCxWifiStationStatusBegin",
            "uCxSocketCreateBegin",
            "uCxSocketConnectBegin",
            "uCxSocketCloseBegin",
            "uCxBluetoothListConnectionStatusBegin"
        ]
        
        for func_name in api_functions:
            if hasattr(self.dll, func_name):
                self._setup_function_prototype(func_name)
    
    def _setup_function_prototype(self, func_name: str):
        """Setup prototype for a specific function"""
        func = getattr(self.dll, func_name)
        
        # General setup - would be customized per function
        if "Begin" in func_name:
            func.restype = ctypes.c_bool
            # Arguments would be set based on specific function requirements
    
    def _extract_parameters_from_at_command(self, at_command: str) -> List[str]:
        """Extract parameter values from AT command string
        
        Examples:
            AT+UWHN=myhost -> ['myhost']
            AT+UWSC=0,2,1 -> ['0', '2', '1']
            AT+UWHN? -> []
        """
        # Check if command has parameters (contains =)
        if '=' not in at_command:
            return []
        
        # Split by = and get the parameter part
        parts = at_command.split('=', 1)
        
        if len(parts) < 2 or not parts[1].strip():
            return []
        
        param_string = parts[1].strip()
        
        # Split by comma to get individual parameters
        # Handle quoted strings if needed
        parameters = []
        current_param = ""
        in_quotes = False
        
        for char in param_string:
            if char == '"':
                in_quotes = not in_quotes
            elif char == ',' and not in_quotes:
                if current_param:
                    parameters.append(current_param.strip())
                current_param = ""
            else:
                current_param += char
        
        # Add the last parameter
        if current_param:
            parameters.append(current_param.strip())
        
        return parameters
    
    def _initialize_ucx_handle(self):
        """Initialize the UCX handle using uCxInit"""
        try:
            # DON'T trigger lazy initialization here - it will be done on first API call
            # Calling it during init can cause crashes
            
            # Get the UCX handle from the wrapper
            # The wrapper initializes the AT client and UCX handle when connecting
            if hasattr(self.ucx_wrapper, 'ucx_handle'):
                self.ucx_handle = self.ucx_wrapper.ucx_handle
                if self.ucx_handle and self.ucx_handle.value:
                    print(f"Using UCX handle from wrapper: {self.ucx_handle.value:#x}")
                else:
                    print("Note: UCX handle will be initialized on first API call")
            else:
                print("Warning: UCX wrapper does not expose ucx_handle")
                self.ucx_handle = None
        except Exception as e:
            print(f"Warning: Could not get UCX handle: {e}")
            self.ucx_handle = None
    
    def _register_urc_callbacks(self):
        """Register URC callbacks for WiFi events (like C example)"""
        try:
            # Define callback function type: void callback(struct uCxHandle *puCxHandle)
            NETWORK_UP_CALLBACK = ctypes.CFUNCTYPE(None, ctypes.c_void_p)
            
            # Create Python callback that sets event flag
            def network_up_callback(ucx_handle_ptr):
                self.urc_event_flags |= self.URC_FLAG_NETWORK_UP
            
            def network_down_callback(ucx_handle_ptr):
                self.urc_event_flags |= self.URC_FLAG_NETWORK_DOWN
            
            # Convert to C callbacks and keep references
            self._network_up_callback = NETWORK_UP_CALLBACK(network_up_callback)
            self._network_down_callback = NETWORK_UP_CALLBACK(network_down_callback)
            
            # Setup register functions
            register_up_func = self.dll.uCxWifiRegisterStationNetworkUp
            register_up_func.argtypes = [ctypes.POINTER(ctypes.c_void_p), NETWORK_UP_CALLBACK]
            register_up_func.restype = None
            
            register_down_func = self.dll.uCxWifiRegisterStationNetworkDown
            register_down_func.argtypes = [ctypes.POINTER(ctypes.c_void_p), NETWORK_UP_CALLBACK]
            register_down_func.restype = None
            
        except Exception as e:
            print(f"Warning: Could not register URC callbacks: {e}")
    
    def _ensure_ucx_handle_ready(self):
        """Ensure UCX handle is initialized before making API calls"""
        # Trigger lazy initialization if not already done
        if hasattr(self.ucx_wrapper, '_ensure_ucx_handle_initialized'):
            try:
                debug_print("[DEBUG] Calling _ensure_ucx_handle_initialized()...")
                self.ucx_wrapper._ensure_ucx_handle_initialized()
                debug_print("[DEBUG] Lazy initialization completed")
            except Exception as e:
                debug_print(f"[DEBUG] Lazy initialization failed: {e}")
        
        # Update our handle reference
        if hasattr(self.ucx_wrapper, 'ucx_handle'):
            self.ucx_handle = self.ucx_wrapper.ucx_handle
            if self.ucx_handle and self.ucx_handle.value:
                debug_print(f"[DEBUG] UCX handle updated: 0x{self.ucx_handle.value:x}")
            else:
                print(f"[DEBUG] UCX handle still not initialized: {self.ucx_handle}")
    
    def _signal_event(self, event_flag):
        """Signal an event (like C example)"""
        self.urc_event_flags |= event_flag
    
    def _wait_event(self, event_flag, timeout_seconds):
        """Wait for an event flag (like C example waitEvent)"""
        import time
        
        if not hasattr(self.ucx_wrapper, '_client_handle'):
            return False
        
        at_client_handle = self.ucx_wrapper._client_handle
        
        # Setup uCxAtClientHandleRx function
        handle_rx_func = self.dll.uCxAtClientHandleRx
        handle_rx_func.argtypes = [ctypes.c_void_p]
        handle_rx_func.restype = None
        
        start_time = time.time()
        timeout_ms = timeout_seconds * 1000
        
        while (time.time() - start_time) < timeout_seconds:
            # Handle incoming URCs (this is what C example does)
            try:
                handle_rx_func(at_client_handle)
            except Exception as e:
                pass  # Ignore errors
            
            # Check if event flag is set
            if self.urc_event_flags & event_flag:
                return True
            
            time.sleep(0.05)  # 50ms sleep between polls
        
        return False
    
    def execute_at_command(self, at_command: str, parameters: List[str] = None) -> APICallResult:
        """Execute an AT command using the corresponding UCX API function"""
        # Ensure UCX handle is initialized before executing any API call
        try:
            self._ensure_ucx_handle_ready()
        except Exception as e:
            return APICallResult(
                success=False,
                error_message=f"Failed to initialize UCX handle: {e}"
            )
        
        # Extract parameters from AT command if not provided
        if parameters is None:
            parameters = self._extract_parameters_from_at_command(at_command)
        
        # Get the API mapping for this AT command
        mapping = at_to_api_mapper.get_api_mapping(at_command)
        
        if not mapping:
            return APICallResult(
                success=False,
                error_message=f"No API mapping found for AT command: {at_command}"
            )
        
        # Execute based on API call type
        if mapping.api_type == APICallType.SIMPLE:
            return self._execute_simple_call(mapping, parameters)
        elif mapping.api_type == APICallType.BEGIN_END:
            return self._execute_begin_end_call(mapping, parameters)
        elif mapping.api_type == APICallType.ASYNC:
            return self._execute_async_call(mapping, parameters) 
        elif mapping.api_type == APICallType.COMPLEX:
            return self._execute_complex_call(mapping, parameters)
        else:
            return APICallResult(
                success=False,
                error_message=f"Unsupported API call type: {mapping.api_type}"
            )
    
    def _execute_simple_call(self, mapping: APIMapping, parameters: List[str]) -> APICallResult:
        """Execute a simple API function call"""
        try:
            # Try the mapped function name first
            func_name = mapping.api_function
            if not hasattr(self.dll, func_name):
                # Try alternative naming patterns
                if func_name.endswith("Begin"):
                    # If it ends with "Begin", also try without it (SIMPLE pattern)
                    func_name_without_begin = func_name[:-5]  # Remove "Begin"
                    if hasattr(self.dll, func_name_without_begin):
                        func_name = func_name_without_begin
                        # Keep as SIMPLE call
                    else:
                        return APICallResult(
                            success=False,
                            error_message=f"API function not found: {mapping.api_function} or {func_name_without_begin}"
                        )
                else:
                    # If it doesn't end with "Begin", try with "Begin" suffix (BEGIN_END pattern)
                    func_name_with_begin = f"{func_name}Begin"
                    if hasattr(self.dll, func_name_with_begin):
                        func_name = func_name_with_begin
                        # Also update the API type to BEGIN_END
                        mapping.api_type = APICallType.BEGIN_END
                        # Delegate to begin_end handler
                        return self._execute_begin_end_call(mapping, parameters)
                    else:
                        return APICallResult(
                            success=False,
                            error_message=f"API function not found: {mapping.api_function} or {func_name_with_begin}"
                        )
            
            func = getattr(self.dll, func_name)
            
            # Ensure we have the UCX handle
            if not self.ucx_handle:
                self.ucx_handle = self.ucx_wrapper.ucx_handle
            
            # Setup function signature
            # Build argtypes: handle pointer + parameter types
            argtypes = [ctypes.POINTER(ctypes.c_void_p)]
            args = [ctypes.byref(self.ucx_handle)]
            
            # Convert parameters to appropriate C types
            for param in parameters:
                # Try to determine parameter type
                if param.isdigit() or (param.startswith('-') and param[1:].isdigit()):
                    # Integer parameter
                    argtypes.append(ctypes.c_int32)
                    args.append(ctypes.c_int32(int(param)))
                else:
                    # String parameter
                    argtypes.append(ctypes.c_char_p)
                    args.append(param.encode('utf-8'))
            
            # Set function signature
            func.argtypes = argtypes
            func.restype = ctypes.c_int32
            
            # Call the function
            result = func(*args)
            
            # Check return code (0 = success, negative = error)
            if result < 0:
                return APICallResult(
                    success=False,
                    error_message=f"API call '{func_name}' returned error code: {result}. Check that device is connected and parameters are valid."
                )
            
            return APICallResult(
                success=True,
                result_data=result,
                raw_response=f"Called {func_name}() - Success (code: {result})"
            )
            
        except Exception as e:
            return APICallResult(
                success=False,
                error_message=f"API call failed: {e}"
            )
    
    def _execute_begin_end_call(self, mapping: APIMapping, parameters: List[str]) -> APICallResult:
        """Execute a Begin/End pattern API call"""
        try:
            # For BEGIN_END calls, the DLL functions have "Begin" suffix
            # Check if we need to add it
            func_name_to_check = mapping.api_function
            if not func_name_to_check.endswith("Begin"):
                func_name_to_check = f"{mapping.api_function}Begin"
            
            # Check if DLL function exists
            if not hasattr(self.dll, func_name_to_check):
                return APICallResult(
                    success=False,
                    error_message=f"API function not found: {func_name_to_check}"
                )
            
            # Special handling for functions that need Begin/GetNext/End pattern
            # Check both with and without "Begin" suffix
            api_func_base = mapping.api_function.replace("Begin", "") if mapping.api_function.endswith("Begin") else mapping.api_function
            
            if api_func_base == "uCxWifiStationScanDefault":
                return self._wifi_scan_begin_end(parameters)
            elif api_func_base == "uCxWifiStationSetConnectionParams":
                return self._wifi_set_connection_params(parameters)
            elif api_func_base == "uCxWifiStationConnect":
                return self._wifi_connect(parameters)
            elif api_func_base == "uCxWifiStationDisconnect":
                return self._wifi_disconnect(parameters)
            elif api_func_base == "uCxBluetoothDiscoveryDefault":
                return self._bluetooth_scan_begin_end(parameters)
            elif api_func_base == "uCxBluetoothConnect":
                return self._bluetooth_connect(parameters)
            elif api_func_base == "uCxBluetoothDisconnect":
                return self._bluetooth_disconnect(parameters)
            
            # Generic Begin/End call for DLL functions
            return self._generic_begin_end_call(mapping, parameters)
                
        except Exception as e:
            return APICallResult(
                success=False,
                error_message=f"Begin/End API call failed: {e}"
            )
    
    def _wifi_scan_begin_end(self, parameters: List[str]) -> APICallResult:
        """Execute WiFi scan with Begin/GetNext/End pattern"""
        # Ensure UCX handle is initialized
        self._ensure_ucx_handle_ready()
        
        # Get latest handle from wrapper
        if hasattr(self.ucx_wrapper, 'ucx_handle'):
            self.ucx_handle = self.ucx_wrapper.ucx_handle
        
        if self.ucx_handle is None or not self.ucx_handle.value:
            return APICallResult(success=False, error_message="UCX handle not initialized")
        
        try:
            # Define the scan result structure
            class MacAddress(ctypes.Structure):
                _fields_ = [("address", ctypes.c_uint8 * 6)]
            
            class WifiScanResult(ctypes.Structure):
                _fields_ = [
                    ("bssid", MacAddress),
                    ("ssid", ctypes.c_char_p),
                    ("channel", ctypes.c_int32),
                    ("rssi", ctypes.c_int32),
                    ("authentication_suites", ctypes.c_int32),
                    ("unicast_ciphers", ctypes.c_int32),
                    ("group_ciphers", ctypes.c_int32)
                ]
            
            # Setup function prototypes
            begin_func = self.dll.uCxWifiStationScanDefaultBegin
            begin_func.argtypes = [ctypes.POINTER(ctypes.c_void_p)]
            begin_func.restype = None
            
            get_next_func = self.dll.uCxWifiStationScanDefaultGetNext
            get_next_func.argtypes = [ctypes.POINTER(ctypes.c_void_p), ctypes.POINTER(WifiScanResult)]
            get_next_func.restype = ctypes.c_bool
            
            end_func = self.dll.uCxEnd
            end_func.argtypes = [ctypes.POINTER(ctypes.c_void_p)]
            end_func.restype = ctypes.c_int32
            
            # Call Begin
            begin_func(ctypes.byref(self.ucx_handle))
            
            # Get all results
            networks = []
            while True:
                result = WifiScanResult()
                has_more = get_next_func(ctypes.byref(self.ucx_handle), ctypes.byref(result))
                if not has_more:
                    break
                
                # Convert MAC address to string
                bssid = ':'.join(f'{b:02x}' for b in result.bssid.address)
                
                # Get SSID (null-terminated string)
                ssid = result.ssid.decode('utf-8') if result.ssid else ""
                
                networks.append({
                    'ssid': ssid,
                    'bssid': bssid,
                    'channel': result.channel,
                    'rssi': result.rssi,
                    'security': f'Auth:{result.authentication_suites}'
                })
            
            # Call End
            status = end_func(ctypes.byref(self.ucx_handle))
            
            return APICallResult(
                success=(status == 0),
                result_data=networks,
                raw_response=f"WiFi scan completed: {len(networks)} networks found"
            )
            
        except Exception as e:
            return APICallResult(success=False, error_message=f"WiFi scan failed: {e}")
    
    def _wifi_get_hostname(self, parameters: List[str]) -> APICallResult:
        """Get WiFi hostname"""
        print(f"DEBUG: _wifi_get_hostname() called")
        
        # Get latest handle from wrapper
        if hasattr(self.ucx_wrapper, 'ucx_handle'):
            self.ucx_handle = self.ucx_wrapper.ucx_handle
        
        if self.ucx_handle is None or not self.ucx_handle.value:
            return APICallResult(success=False, error_message="UCX handle not initialized")
        
        try:
            # Setup function prototype
            # bool uCxWifiGetHostnameBegin(uCxHandle_t * puCxHandle, const char ** ppHostName)
            begin_func = self.dll.uCxWifiGetHostnameBegin
            begin_func.argtypes = [ctypes.POINTER(ctypes.c_void_p), ctypes.POINTER(ctypes.c_char_p)]
            begin_func.restype = ctypes.c_bool
            
            # Create output variable for hostname
            hostname_ptr = ctypes.c_char_p()
            
            # Call Begin function
            print(f"DEBUG: Calling uCxWifiGetHostnameBegin()")
            success = begin_func(ctypes.byref(self.ucx_handle), ctypes.byref(hostname_ptr))
            print(f"DEBUG: uCxWifiGetHostnameBegin returned: {success}")
            
            if success and hostname_ptr.value:
                hostname = hostname_ptr.value.decode('utf-8')
                print(f"DEBUG: Retrieved hostname: '{hostname}'")
                
                # Call End
                end_func = self.dll.uCxEnd
                end_func.argtypes = [ctypes.POINTER(ctypes.c_void_p)]
                end_func.restype = ctypes.c_int32
                end_func(ctypes.byref(self.ucx_handle))
                
                return APICallResult(
                    success=True,
                    result_data=hostname,
                    raw_response=f"Hostname: {hostname}"
                )
            else:
                return APICallResult(success=False, error_message="Failed to get hostname")
                
        except Exception as e:
            print(f"DEBUG: Exception in _wifi_get_hostname: {e}")
            return APICallResult(success=False, error_message=f"Get hostname failed: {e}")
    
    def _wifi_set_connection_params(self, parameters: List[str]) -> APICallResult:
        """Set WiFi connection parameters (SSID and password)"""
        
        if hasattr(self.ucx_wrapper, 'ucx_handle'):
            self.ucx_handle = self.ucx_wrapper.ucx_handle
        
        if self.ucx_handle is None or not self.ucx_handle.value:
            return APICallResult(success=False, error_message="UCX handle not initialized")
        
        if len(parameters) < 2:
            return APICallResult(success=False, error_message="Missing SSID or password")
        
        try:
            ssid = parameters[0]
            password = parameters[1] if len(parameters) > 1 else ""
            wlan_handle = 0  # Default WiFi handle
            
            # Set connection params (SSID) first
            set_params_func = self.dll.uCxWifiStationSetConnectionParams
            set_params_func.argtypes = [ctypes.POINTER(ctypes.c_void_p), ctypes.c_int32, ctypes.c_char_p]
            set_params_func.restype = ctypes.c_int32
            
            status = set_params_func(ctypes.byref(self.ucx_handle), wlan_handle, ssid.encode('utf-8'))
            if status != 0:
                return APICallResult(success=False, error_message=f"Failed to set SSID: {status}")
            
            # Set security (WPA2) if password is provided
            if password:
                set_security_func = self.dll.uCxWifiStationSetSecurityWpa
                set_security_func.argtypes = [ctypes.POINTER(ctypes.c_void_p), ctypes.c_int32, ctypes.c_char_p, ctypes.c_int32]
                set_security_func.restype = ctypes.c_int32
                
                # U_WPA_THRESHOLD_WPA2 = 0 (supports WPA2 or higher, including WPA3)
                # U_WPA_THRESHOLD_WPA3 = 1 (only WPA3)
                status = set_security_func(ctypes.byref(self.ucx_handle), wlan_handle, password.encode('utf-8'), 0)
                if status != 0:
                    return APICallResult(success=False, error_message=f"Failed to set WiFi security (WPA2/WPA3): {status}")
            else:
                # Set open security if no password
                set_open_func = self.dll.uCxWifiStationSetSecurityOpen
                set_open_func.argtypes = [ctypes.POINTER(ctypes.c_void_p), ctypes.c_int32]
                set_open_func.restype = ctypes.c_int32
                
                status = set_open_func(ctypes.byref(self.ucx_handle), wlan_handle)
                if status != 0:
                    return APICallResult(success=False, error_message=f"Failed to set open security: {status}")
            
            return APICallResult(
                success=True,
                result_data={"ssid": ssid, "configured": True},
                raw_response=f"WiFi configured: {ssid}"
            )
            
        except Exception as e:
            return APICallResult(success=False, error_message=f"WiFi config failed: {e}")
    
    def _wifi_connect(self, parameters: List[str]) -> APICallResult:
        """Connect to WiFi network and wait for Network Up event (like C example)"""
        
        if hasattr(self.ucx_wrapper, 'ucx_handle'):
            self.ucx_handle = self.ucx_wrapper.ucx_handle
        
        if self.ucx_handle is None or not self.ucx_handle.value:
            return APICallResult(success=False, error_message="UCX handle not initialized")
        
        try:
            wlan_handle = 0  # Default WiFi handle
            
            # Clear event flags before connecting
            self.urc_event_flags = 0
            
            # Register URC callbacks if not already registered
            if self._network_up_callback is None:
                self._register_urc_callbacks()
            
            # Now register callbacks with the UCX handle (must be done after handle is initialized)
            try:
                NETWORK_UP_CALLBACK = ctypes.CFUNCTYPE(None, ctypes.c_void_p)
                
                register_up_func = self.dll.uCxWifiRegisterStationNetworkUp
                register_up_func.argtypes = [ctypes.POINTER(ctypes.c_void_p), NETWORK_UP_CALLBACK]
                register_up_func.restype = None
                
                register_down_func = self.dll.uCxWifiRegisterStationNetworkDown
                register_down_func.argtypes = [ctypes.POINTER(ctypes.c_void_p), NETWORK_UP_CALLBACK]
                register_down_func.restype = None
                
                # Register the callbacks with UCX handle
                register_up_func(ctypes.byref(self.ucx_handle), self._network_up_callback)
                register_down_func(ctypes.byref(self.ucx_handle), self._network_down_callback)
            except Exception as e:
                print(f"Warning: Could not register URC callbacks with handle: {e}")
            
            # Start connection
            connect_func = self.dll.uCxWifiStationConnect
            connect_func.argtypes = [ctypes.POINTER(ctypes.c_void_p), ctypes.c_int32]
            connect_func.restype = ctypes.c_int32
            
            status = connect_func(ctypes.byref(self.ucx_handle), wlan_handle)
            if status != 0:
                return APICallResult(success=False, error_message=f"Failed to start connection: {status}")
            
            # Wait for Network Up event (like C example: waitEvent(URC_FLAG_NETWORK_UP, 20))
            if self._wait_event(self.URC_FLAG_NETWORK_UP, 20):
                # Network is up! Now get IP address
                try:
                    # Define the network status structures
                    class IpAddressUnion(ctypes.Union):
                        _fields_ = [
                            ("ipv4", ctypes.c_uint32),
                            ("ipv6", ctypes.c_uint32 * 4)
                        ]
                    
                    class SockIpAddress(ctypes.Structure):
                        _fields_ = [
                            ("type", ctypes.c_int32),
                            ("address", IpAddressUnion)
                        ]
                    
                    class NetworkStatus(ctypes.Structure):
                        _fields_ = [
                            ("status_id", ctypes.c_int32),
                            ("status_val", SockIpAddress)
                        ]
                    
                    # Setup ListNetworkStatus functions
                    begin_func = self.dll.uCxWifiStationListNetworkStatusBegin
                    begin_func.argtypes = [ctypes.POINTER(ctypes.c_void_p)]
                    begin_func.restype = None
                    
                    get_next_func = self.dll.uCxWifiStationListNetworkStatusGetNext
                    get_next_func.argtypes = [ctypes.POINTER(ctypes.c_void_p), ctypes.POINTER(NetworkStatus)]
                    get_next_func.restype = ctypes.c_bool
                    
                    end_func = self.dll.uCxEnd
                    end_func.argtypes = [ctypes.POINTER(ctypes.c_void_p)]
                    end_func.restype = ctypes.c_int32
                    
                    # Query network status to get IP
                    begin_func(ctypes.byref(self.ucx_handle))
                    
                    ip_str = "unknown"
                    while True:
                        status_entry = NetworkStatus()
                        has_more = get_next_func(ctypes.byref(self.ucx_handle), ctypes.byref(status_entry))
                        
                        if not has_more:
                            break
                        
                        # U_STATUS_ID_IPV4 = 0
                        if status_entry.status_id == 0 and status_entry.status_val.type == 0:
                            ipv4_address = status_entry.status_val.address.ipv4
                            if ipv4_address != 0:
                                # IP address is in network byte order (big-endian), need to reverse bytes
                                ip_str = f"{(ipv4_address >> 24) & 0xFF}.{(ipv4_address >> 16) & 0xFF}.{(ipv4_address >> 8) & 0xFF}.{ipv4_address & 0xFF}"
                                break
                    
                    end_func(ctypes.byref(self.ucx_handle))
                    
                    return APICallResult(
                        success=True,
                        result_data={"connected": True, "status": "WiFi Up and Network Up", "ip": ip_str},
                        raw_response=f"✓ Connected - WiFi Up and Network Up\nIP Address: {ip_str}"
                    )
                
                except Exception as e:
                    print(f"Warning: Could not get IP address: {e}")
                    return APICallResult(
                        success=True,
                        result_data={"connected": True, "status": "WiFi Up and Network Up"},
                        raw_response="✓ Connected - WiFi Up and Network Up"
                    )
            else:
                return APICallResult(
                    success=False,
                    error_message="Connection timeout - Network did not come up within 20 seconds"
                )
            
        except Exception as e:
            return APICallResult(success=False, error_message=f"WiFi connect failed: {e}")
    
    def _wifi_disconnect(self, parameters: List[str]) -> APICallResult:
        """Disconnect from WiFi network"""
        
        if hasattr(self.ucx_wrapper, 'ucx_handle'):
            self.ucx_handle = self.ucx_wrapper.ucx_handle
        
        if self.ucx_handle is None or not self.ucx_handle.value:
            return APICallResult(success=False, error_message="UCX handle not initialized")
        
        try:
            disconnect_func = self.dll.uCxWifiStationDisconnect
            disconnect_func.argtypes = [ctypes.POINTER(ctypes.c_void_p)]
            disconnect_func.restype = ctypes.c_int32
            
            status = disconnect_func(ctypes.byref(self.ucx_handle))
            
            return APICallResult(
                success=(status == 0),
                result_data={"disconnected": (status == 0)},
                raw_response=f"WiFi disconnect status: {status}"
            )
            
        except Exception as e:
            return APICallResult(success=False, error_message=f"WiFi disconnect failed: {e}")
    
    def _bluetooth_scan_begin_end(self, parameters: List[str]) -> APICallResult:
        """Execute Bluetooth discovery with Begin/GetNext/End pattern"""
        
        if hasattr(self.ucx_wrapper, 'ucx_handle'):
            self.ucx_handle = self.ucx_wrapper.ucx_handle
        
        if self.ucx_handle is None or not self.ucx_handle.value:
            return APICallResult(success=False, error_message="UCX handle not initialized")
        
        try:
            # First, ensure Bluetooth is enabled in Central or Central+Peripheral mode
            set_mode_func = self.dll.uCxBluetoothSetMode
            set_mode_func.argtypes = [ctypes.POINTER(ctypes.c_void_p), ctypes.c_int32]
            set_mode_func.restype = ctypes.c_int32
            
            # Set to Central+Peripheral mode (3) to enable scanning
            mode_result = set_mode_func(ctypes.byref(self.ucx_handle), 3)
            if mode_result != 0:
                return APICallResult(success=False, error_message=f"Failed to enable Bluetooth mode: {mode_result}")
            
            # Small delay to let mode change take effect
            import time
            time.sleep(0.5)
            # Define BT address structure matching uBtLeAddress_t
            # typedef struct { uint8_t address[6]; uBdAddressType_t type; } uBtLeAddress_t;
            class BtLeAddress(ctypes.Structure):
                _fields_ = [
                    ("address", ctypes.c_uint8 * 6),
                    ("type", ctypes.c_int32)  # uBdAddressType_t is an enum (int)
                ]
            
            # Define byte array structure
            class ByteArray(ctypes.Structure):
                _fields_ = [
                    ("pData", ctypes.POINTER(ctypes.c_uint8)),
                    ("length", ctypes.c_size_t)
                ]
            
            # Define Bluetooth discovery result structure
            class BluetoothDiscoveryResult(ctypes.Structure):
                _fields_ = [
                    ("bd_addr", BtLeAddress),
                    ("rssi", ctypes.c_int32),
                    ("device_name", ctypes.c_char_p),
                    ("data_type", ctypes.c_int32),
                    ("data", ByteArray)
                ]
            
            # Setup function prototypes
            begin_func = self.dll.uCxBluetoothDiscoveryDefaultBegin
            begin_func.argtypes = [ctypes.POINTER(ctypes.c_void_p)]
            begin_func.restype = None
            
            get_next_func = self.dll.uCxBluetoothDiscoveryDefaultGetNext
            get_next_func.argtypes = [ctypes.POINTER(ctypes.c_void_p), ctypes.POINTER(BluetoothDiscoveryResult)]
            get_next_func.restype = ctypes.c_bool
            
            end_func = self.dll.uCxEnd
            end_func.argtypes = [ctypes.POINTER(ctypes.c_void_p)]
            end_func.restype = ctypes.c_int32
            
            # Call Begin (starts discovery)
            begin_func(ctypes.byref(self.ucx_handle))
            
            # Get all results
            devices = []
            while True:
                result = BluetoothDiscoveryResult()
                if not get_next_func(ctypes.byref(self.ucx_handle), ctypes.byref(result)):
                    break
                
                # Convert BT address to string
                address = ':'.join(f'{b:02x}' for b in result.bd_addr.address)
                
                # Get device name
                name = result.device_name.decode('utf-8') if result.device_name else "Unknown"
                
                # Determine device type
                device_type = "BLE" if result.bd_addr.type == 0 else f"Type {result.bd_addr.type}"
                
                devices.append({
                    'name': name,
                    'address': address,
                    'rssi': result.rssi,
                    'type': device_type
                })
            
            # Call End
            status = end_func(ctypes.byref(self.ucx_handle))
            
            return APICallResult(
                success=(status == 0),
                result_data=devices,
                raw_response=f"Bluetooth scan completed: {len(devices)} devices found"
            )
            
        except Exception as e:
            return APICallResult(success=False, error_message=f"Bluetooth scan failed: {e}")
    
    def _bluetooth_connect(self, parameters: List[str]) -> APICallResult:
        """Connect to Bluetooth device"""
        
        if hasattr(self.ucx_wrapper, 'ucx_handle'):
            self.ucx_handle = self.ucx_wrapper.ucx_handle
        
        if self.ucx_handle is None or not self.ucx_handle.value:
            return APICallResult(success=False, error_message="UCX handle not initialized")
        
        if len(parameters) < 1:
            return APICallResult(success=False, error_message="Missing BT address")
        
        try:
            # Parse BT address (format: "00:11:22:33:44:55")
            address_str = parameters[0]
            address_bytes = bytes.fromhex(address_str.replace(':', ''))
            
            if len(address_bytes) != 6:
                return APICallResult(success=False, error_message="Invalid BT address format")
            
            # Define BT address structure
            class BtLeAddress(ctypes.Structure):
                _fields_ = [
                    ("address", ctypes.c_uint8 * 6),
                    ("type", ctypes.c_int32)
                ]
            
            bt_addr = BtLeAddress()
            for i in range(6):
                bt_addr.address[i] = address_bytes[i]
            bt_addr.type = 0  # Public address
            
            connect_func = self.dll.uCxBluetoothConnect
            connect_func.argtypes = [ctypes.POINTER(ctypes.c_void_p), ctypes.POINTER(BtLeAddress)]
            connect_func.restype = ctypes.c_int32
            
            status = connect_func(ctypes.byref(self.ucx_handle), ctypes.byref(bt_addr))
            
            return APICallResult(
                success=(status >= 0),
                result_data={"conn_handle": status} if status >= 0 else None,
                raw_response=f"Bluetooth connect: handle={status}"
            )
            
        except Exception as e:
            return APICallResult(success=False, error_message=f"Bluetooth connect failed: {e}")
    
    def _bluetooth_disconnect(self, parameters: List[str]) -> APICallResult:
        """Disconnect from Bluetooth device"""
        
        if hasattr(self.ucx_wrapper, 'ucx_handle'):
            self.ucx_handle = self.ucx_wrapper.ucx_handle
        
        if self.ucx_handle is None or not self.ucx_handle.value:
            return APICallResult(success=False, error_message="UCX handle not initialized")
        
        if len(parameters) < 1:
            return APICallResult(success=False, error_message="Missing connection handle")
        
        try:
            conn_handle = int(parameters[0])
            
            disconnect_func = self.dll.uCxBluetoothDisconnect
            disconnect_func.argtypes = [ctypes.POINTER(ctypes.c_void_p), ctypes.c_int32]
            disconnect_func.restype = ctypes.c_int32
            
            status = disconnect_func(ctypes.byref(self.ucx_handle), conn_handle)
            
            return APICallResult(
                success=(status == 0),
                result_data={"disconnected": (status == 0)},
                raw_response=f"Bluetooth disconnect status: {status}"
            )
            
        except Exception as e:
            return APICallResult(success=False, error_message=f"Bluetooth disconnect failed: {e}")
    
    def _generic_begin_end_call(self, mapping: APIMapping, parameters: List[str]) -> APICallResult:
        """Generic Begin/End API call handler for simple Get operations
        
        Handles functions like:
        - bool uCxXxxGetYyyBegin(uCxHandle_t * puCxHandle, const char ** ppOutput)
        
        These functions return a single string value via output parameter.
        """
        
        # Get latest handle from wrapper (it's initialized on connect)
        if hasattr(self.ucx_wrapper, 'ucx_handle'):
            self.ucx_handle = self.ucx_wrapper.ucx_handle
        
        # Check if UCX handle is available and initialized
        if self.ucx_handle is None or not self.ucx_handle.value:
            return APICallResult(
                success=False,
                error_message="UCX handle not initialized. Connect to device first."
            )
        
        try:
            # Get the Begin function from DLL - add "Begin" suffix if needed
            func_name = mapping.api_function
            if not func_name.endswith("Begin"):
                func_name = f"{func_name}Begin"
            
            begin_func = getattr(self.dll, func_name)
            
            # For Get operations, setup function signature:
            # bool FunctionBegin(handle, output_string_ptr)
            begin_func.argtypes = [ctypes.POINTER(ctypes.c_void_p), ctypes.POINTER(ctypes.c_char_p)]
            begin_func.restype = ctypes.c_bool
            
            # Create output variable
            output_ptr = ctypes.c_char_p()
            
            # Call Begin function
            success = begin_func(ctypes.byref(self.ucx_handle), ctypes.byref(output_ptr))
            
            if success and output_ptr.value:
                result_value = output_ptr.value.decode('utf-8')
                
                # Call End
                end_func = self.dll.uCxEnd
                end_func.argtypes = [ctypes.POINTER(ctypes.c_void_p)]
                end_func.restype = ctypes.c_int32
                end_func(ctypes.byref(self.ucx_handle))
                
                return APICallResult(
                    success=True,
                    result_data=result_value,
                    raw_response=f"{func_name}() -> {result_value}"
                )
            else:
                return APICallResult(
                    success=False,
                    error_message=f"{func_name} returned no data or failed"
                )
                
        except Exception as e:
            return APICallResult(success=False, error_message=f"DLL call failed: {e}")
    
    def _execute_async_call(self, mapping: APIMapping, parameters: List[str]) -> APICallResult:
        """Execute an asynchronous API call"""
        # Async calls treated as synchronous Begin/End calls
        return self._execute_begin_end_call(mapping, parameters)
    
    def _execute_complex_call(self, mapping: APIMapping, parameters: List[str]) -> APICallResult:
        """Execute a complex API call that requires multiple steps"""
        # Complex calls also use standard Begin/End pattern
        return self._execute_begin_end_call(mapping, parameters)
    
    def get_available_api_functions(self) -> List[str]:
        """Get list of available UCX API functions"""
        available = []
        for mapping in at_to_api_mapper.get_all_mappings().values():
            if hasattr(self.dll, mapping.api_function):
                available.append(mapping.api_function)
        return available
    
    # Quick command API wrappers
    
    def at_test(self) -> APICallResult:
        """Execute AT test command (basic communication test)"""
        try:
            # Ensure UCX handle is initialized
            self._ensure_ucx_handle_ready()
            
            # AT command is just "OK" - we verify communication is working
            # Just check if the handle is valid
            if self.ucx_handle and self.ucx_handle.value:
                return APICallResult(
                    success=True,
                    result_data="OK",
                    raw_response="AT test: Communication OK"
                )
            else:
                return APICallResult(success=False, error_message="UCX handle not initialized")
        except Exception as e:
            return APICallResult(success=False, error_message=f"AT test failed: {e}")
    
    def ati9_info(self) -> APICallResult:
        """Execute ATI9 command (get comprehensive device info) using UCX API"""
        try:
            debug_print("[DEBUG] ati9_info() called")
            
            # Ensure UCX handle is initialized
            self._ensure_ucx_handle_ready()
            
            # Check if API function exists
            if not hasattr(self.dll, 'uCxGeneralGetIdentInfoBegin'):
                return APICallResult(
                    success=False,
                    error_message="uCxGeneralGetIdentInfoBegin not available in DLL"
                )
            
            # Define the response structure matching uCxGeneralGetIdentInfo_t
            class GetIdentInfoResponse(ctypes.Structure):
                _fields_ = [
                    ("application_version", ctypes.c_char_p),
                    ("unique_identifier", ctypes.c_char_p)
                ]
            
            # Setup function prototype
            begin_func = self.dll.uCxGeneralGetIdentInfoBegin
            begin_func.argtypes = [ctypes.POINTER(ctypes.c_void_p), ctypes.POINTER(GetIdentInfoResponse)]
            begin_func.restype = ctypes.c_bool
            
            # Call the API
            response = GetIdentInfoResponse()
            debug_print("[DEBUG] Calling uCxGeneralGetIdentInfoBegin...")
            debug_print(f"[DEBUG] UCX handle value: 0x{self.ucx_handle.value:x}" if self.ucx_handle.value else "[DEBUG] UCX handle is NULL!")
            
            if begin_func(ctypes.byref(self.ucx_handle), ctypes.byref(response)):
                debug_print("[DEBUG] uCxGeneralGetIdentInfoBegin returned True")
                debug_print(f"[DEBUG] Raw response.application_version: {response.application_version}")
                debug_print(f"[DEBUG] Raw response.unique_identifier: {response.unique_identifier}")
                
                # IMPORTANT: Copy the strings BEFORE calling uCxEnd, as the buffer may be reused
                info_lines = []
                if response.application_version:
                    version_str = response.application_version.decode('utf-8')
                    info_lines.append(f"Software Version: {version_str}")
                    debug_print(f"[DEBUG] Copied version: {version_str}")
                if response.unique_identifier:
                    id_str = response.unique_identifier.decode('utf-8')
                    info_lines.append(f"Unique Identifier: {id_str}")
                    debug_print(f"[DEBUG] Copied identifier: {id_str}")
                
                # Call uCxEnd to complete the command
                if hasattr(self.dll, 'uCxEnd'):
                    end_func = self.dll.uCxEnd
                    end_func.argtypes = [ctypes.POINTER(ctypes.c_void_p)]
                    end_func.restype = ctypes.c_int32
                    error_code = end_func(ctypes.byref(self.ucx_handle))
                    debug_print(f"[DEBUG] uCxEnd returned error code: {error_code}")
                    
                    if error_code == 0:
                        result_data = "\n".join(info_lines) if info_lines else "No device information available"
                        debug_print(f"[DEBUG] ATI9 success: {result_data}")
                        
                        return APICallResult(
                            success=True,
                            result_data=result_data
                        )
                    else:
                        return APICallResult(
                            success=False,
                            error_message=f"uCxEnd returned error code: {error_code}"
                        )
                else:
                    return APICallResult(
                        success=False,
                        error_message="uCxEnd function not available"
                    )
            else:
                debug_print("[DEBUG] uCxGeneralGetIdentInfoBegin returned False")
                return APICallResult(
                    success=False,
                    error_message="Failed to get device identification info"
                )
                
        except Exception as e:
            import traceback
            error_details = traceback.format_exc()
            debug_print(f"[DEBUG] ATI9 exception: {error_details}")
            return APICallResult(success=False, error_message=f"ATI9 failed: {e}")
    
    def store_configuration(self) -> APICallResult:
        """Execute AT&W command (store configuration to flash) using UCX API"""
        try:
            # Ensure UCX handle is initialized
            self._ensure_ucx_handle_ready()
            
            if not hasattr(self.dll, 'uCxSystemStoreConfiguration'):
                return APICallResult(success=False, error_message="uCxSystemStoreConfiguration not available")
            
            func = self.dll.uCxSystemStoreConfiguration
            func.argtypes = [ctypes.POINTER(ctypes.c_void_p)]
            func.restype = ctypes.c_int32
            
            result = func(ctypes.byref(self.ucx_handle))
            
            if result == 0:
                return APICallResult(
                    success=True,
                    result_data="Configuration stored to flash"
                )
            else:
                return APICallResult(success=False, error_message=f"Store configuration failed with error code: {result}")
                
        except Exception as e:
            import traceback
            debug_print(f"[DEBUG] Store config exception: {traceback.format_exc()}")
            return APICallResult(success=False, error_message=f"Store configuration failed: {e}")
    
    def reset_to_defaults(self) -> APICallResult:
        """Execute AT&F command (reset all settings to default values) using UCX API"""
        try:
            # Ensure UCX handle is initialized
            self._ensure_ucx_handle_ready()
            
            if not hasattr(self.dll, 'uCxSystemDefaultSettings'):
                return APICallResult(success=False, error_message="uCxSystemDefaultSettings not available")
            
            func = self.dll.uCxSystemDefaultSettings
            func.argtypes = [ctypes.POINTER(ctypes.c_void_p)]
            func.restype = ctypes.c_int32
            
            result = func(ctypes.byref(self.ucx_handle))
            
            if result == 0:
                return APICallResult(
                    success=True,
                    result_data="Settings reset to defaults (certificates and BT bonding preserved)"
                )
            else:
                return APICallResult(success=False, error_message=f"Reset to defaults failed with error code: {result}")
                
        except Exception as e:
            import traceback
            debug_print(f"[DEBUG] Reset to defaults exception: {traceback.format_exc()}")
            return APICallResult(success=False, error_message=f"Reset to defaults failed: {e}")
    
    def factory_reset(self) -> APICallResult:
        """Execute AT+USYFR command (complete factory reset - removes all settings, certificates, and BT bonding)"""
        try:
            if not hasattr(self.dll, 'uCxSystemFactoryReset'):
                return APICallResult(success=False, error_message="uCxSystemFactoryReset not available")
            
            func = self.dll.uCxSystemFactoryReset
            func.argtypes = [ctypes.POINTER(ctypes.c_void_p)]
            func.restype = ctypes.c_int32
            
            result = func(ctypes.byref(self.ucx_handle))
            
            if result == 0:
                return APICallResult(
                    success=True,
                    result_data="Factory reset complete - all settings, certificates, and BT bonding removed",
                    raw_response="AT+USYFR: Factory reset successful"
                )
            else:
                return APICallResult(success=False, error_message=f"Factory reset failed with error code: {result}")
                
        except Exception as e:
            return APICallResult(success=False, error_message=f"Factory reset failed: {e}")
    
    def reboot_device(self) -> APICallResult:
        """Execute AT+CPWROFF command (reboot the device) using UCX API"""
        try:
            # Ensure UCX handle is initialized
            self._ensure_ucx_handle_ready()
            
            if not hasattr(self.dll, 'uCxSystemReboot'):
                return APICallResult(success=False, error_message="uCxSystemReboot not available")
            
            func = self.dll.uCxSystemReboot
            func.argtypes = [ctypes.POINTER(ctypes.c_void_p)]
            func.restype = ctypes.c_int32
            
            result = func(ctypes.byref(self.ucx_handle))
            
            if result == 0:
                return APICallResult(
                    success=True,
                    result_data="Device rebooting..."
                )
            else:
                return APICallResult(success=False, error_message=f"Reboot failed with error code: {result}")
                
        except Exception as e:
            import traceback
            debug_print(f"[DEBUG] Reboot exception: {traceback.format_exc()}")
            return APICallResult(success=False, error_message=f"Reboot failed: {e}")
    
    def bluetooth_get_mode(self) -> APICallResult:
        """Get current Bluetooth mode"""
        try:
            debug_print("[DEBUG] bluetooth_get_mode() called")
            self._ensure_ucx_handle_ready()
            
            if not hasattr(self.dll, 'uCxBluetoothGetMode'):
                return APICallResult(success=False, error_message="uCxBluetoothGetMode not available")
            
            debug_print(f"[DEBUG] UCX handle: 0x{self.ucx_handle.value:x}" if self.ucx_handle.value else "[DEBUG] UCX handle is NULL!")
            
            # No need for API lock - UCX library has internal mutex for command serialization
            func = self.dll.uCxBluetoothGetMode
            func.argtypes = [ctypes.POINTER(ctypes.c_void_p), ctypes.POINTER(ctypes.c_int32)]
            func.restype = ctypes.c_int32
            
            bt_mode = ctypes.c_int32()
            debug_print("[DEBUG] Calling uCxBluetoothGetMode...")
            result = func(ctypes.byref(self.ucx_handle), ctypes.byref(bt_mode))
            debug_print(f"[DEBUG] uCxBluetoothGetMode returned: {result}, bt_mode value: {bt_mode.value}")
            
            # Retry once on timeout (error -65536)
            if result == -65536:
                debug_print("[DEBUG] Timeout on first attempt, retrying...")
                import time
                time.sleep(0.1)  # Brief delay before retry
                result = func(ctypes.byref(self.ucx_handle), ctypes.byref(bt_mode))
                debug_print(f"[DEBUG] Retry result: {result}, bt_mode value: {bt_mode.value}")
            
            if result == 0:
                mode_names = {
                    0: "Disabled",
                    1: "Central",
                    2: "Peripheral", 
                    3: "Central + Peripheral"
                }
                mode_str = mode_names.get(bt_mode.value, f"Unknown ({bt_mode.value})")
                
                return APICallResult(
                    success=True,
                    result_data=f"Bluetooth Mode: {mode_str}"
                )
            else:
                return APICallResult(success=False, error_message=f"Get BT mode failed with error code: {result}")
                
        except Exception as e:
            import traceback
            debug_print(f"[DEBUG] BT mode exception: {traceback.format_exc()}")
            return APICallResult(success=False, error_message=f"Get BT mode failed: {e}")
    
    def bluetooth_get_status(self) -> APICallResult:
        """Get Bluetooth status including active connections"""
        try:
            debug_print("[DEBUG] bluetooth_get_status() called")
            self._ensure_ucx_handle_ready()
            
            if not hasattr(self.dll, 'uCxBluetoothGetMode'):
                return APICallResult(success=False, error_message="Bluetooth mode API not available")
            
            if not hasattr(self.dll, 'uCxBluetoothListConnectionsBegin'):
                return APICallResult(success=False, error_message="Bluetooth list connections API not available")
            
            if not hasattr(self.dll, 'uCxBluetoothListConnectionsGetNext'):
                return APICallResult(success=False, error_message="Bluetooth get next connection API not available")
                
            if not hasattr(self.dll, 'uCxEnd'):
                return APICallResult(success=False, error_message="uCxEnd API not available")
            
            # First get the Bluetooth mode
            func_mode = self.dll.uCxBluetoothGetMode
            func_mode.argtypes = [ctypes.POINTER(ctypes.c_void_p), ctypes.POINTER(ctypes.c_int32)]
            func_mode.restype = ctypes.c_int32
            
            bt_mode = ctypes.c_int32()
            debug_print("[DEBUG] Calling uCxBluetoothGetMode...")
            result = func_mode(ctypes.byref(self.ucx_handle), ctypes.byref(bt_mode))
            debug_print(f"[DEBUG] uCxBluetoothGetMode returned: {result}, bt_mode value: {bt_mode.value}")
            
            if result != 0:
                return APICallResult(success=False, error_message=f"Get BT mode failed with error code: {result}")
            
            mode_names = {
                0: "Disabled",
                1: "Central",
                2: "Peripheral", 
                3: "Central + Peripheral"
            }
            mode_str = mode_names.get(bt_mode.value, f"Unknown ({bt_mode.value})")
            
            if bt_mode.value == 0:
                return APICallResult(
                    success=True,
                    result_data="Bluetooth Status: Disabled"
                )
            
            # Define structures for listing connections
            class MacAddress(ctypes.Structure):
                _fields_ = [
                    ("address", ctypes.c_uint8 * 6)
                ]
            
            class BtLeAddress(ctypes.Structure):
                _fields_ = [
                    ("type", ctypes.c_int32),
                    ("address", MacAddress)
                ]
            
            class BluetoothListConnections(ctypes.Structure):
                _fields_ = [
                    ("conn_handle", ctypes.c_int32),
                    ("bd_addr", BtLeAddress)
                ]
            
            # Set up function signatures
            func_begin = self.dll.uCxBluetoothListConnectionsBegin
            func_begin.argtypes = [ctypes.POINTER(ctypes.c_void_p)]
            func_begin.restype = None
            
            func_get_next = self.dll.uCxBluetoothListConnectionsGetNext
            func_get_next.argtypes = [ctypes.POINTER(ctypes.c_void_p), ctypes.POINTER(BluetoothListConnections)]
            func_get_next.restype = ctypes.c_bool
            
            func_end = self.dll.uCxEnd
            func_end.argtypes = [ctypes.POINTER(ctypes.c_void_p)]
            func_end.restype = ctypes.c_int32
            
            # List all connections
            debug_print("[DEBUG] Calling uCxBluetoothListConnectionsBegin...")
            func_begin(ctypes.byref(self.ucx_handle))
            
            connections = []
            while True:
                conn = BluetoothListConnections()
                result = func_get_next(ctypes.byref(self.ucx_handle), ctypes.byref(conn))
                if not result:
                    break
                
                # Format MAC address
                mac_str = ':'.join(f'{conn.bd_addr.address.address[i]:02X}' for i in range(6))
                addr_type = "Public" if conn.bd_addr.type == 0 else "Random"
                connections.append(f"  Handle {conn.conn_handle}: {mac_str} ({addr_type})")
                debug_print(f"[DEBUG] Found connection - Handle: {conn.conn_handle}, MAC: {mac_str}")
            
            end_result = func_end(ctypes.byref(self.ucx_handle))
            debug_print(f"[DEBUG] uCxEnd returned: {end_result}")
            
            # Build status message
            status_msg = f"Bluetooth Status: {mode_str}\n"
            if connections:
                status_msg += f"Active Connections: {len(connections)}\n"
                status_msg += "\n".join(connections)
            else:
                status_msg += "Active Connections: 0 (No devices connected)"
            
            return APICallResult(
                success=True,
                result_data=status_msg
            )
                
        except Exception as e:
            import traceback
            debug_print(f"[DEBUG] BT status exception: {traceback.format_exc()}")
            return APICallResult(success=False, error_message=f"Get BT status failed: {e}")

    
    def wifi_get_status(self) -> APICallResult:
        """Get WiFi station status including connection state and SSID"""
        try:
            debug_print("[DEBUG] wifi_get_status() called")
            self._ensure_ucx_handle_ready()
            
            if not hasattr(self.dll, 'uCxWifiStationStatusBegin'):
                return APICallResult(success=False, error_message="WiFi status API not available")
            
            if not hasattr(self.dll, 'uCxEnd'):
                return APICallResult(success=False, error_message="uCxEnd API not available")
            
            # Define WiFi status structures matching u_cx_wifi.h
            class MacAddress(ctypes.Structure):
                _fields_ = [("address", ctypes.c_uint8 * 6)]
            
            class IpAddress(ctypes.Structure):
                _fields_ = [
                    ("address", ctypes.c_uint8 * 16),
                    ("type", ctypes.c_int32)
                ]
            
            class WifiStationStatusRspStr(ctypes.Structure):
                _fields_ = [
                    ("wifi_status_id", ctypes.c_int32),
                    ("ssid", ctypes.c_char_p)
                ]
            
            class WifiStationStatusRspMac(ctypes.Structure):
                _fields_ = [
                    ("wifi_status_id", ctypes.c_int32),
                    ("bssid", MacAddress)
                ]
            
            class WifiStationStatusRspInt(ctypes.Structure):
                _fields_ = [
                    ("wifi_status_id", ctypes.c_int32),
                    ("int_val", ctypes.c_int32)
                ]
            
            class WifiStationStatusUnion(ctypes.Union):
                _fields_ = [
                    ("rspWifiStatusIdStr", WifiStationStatusRspStr),
                    ("rspWifiStatusIdMac", WifiStationStatusRspMac),
                    ("rspWifiStatusIdInt", WifiStationStatusRspInt)
                ]
            
            class WifiStationStatus(ctypes.Structure):
                _anonymous_ = ("u",)
                _fields_ = [
                    ("type", ctypes.c_int32),
                    ("u", WifiStationStatusUnion)
                ]
            
            # WiFi Status ID enum values from u_cx_types.h
            U_WIFI_STATUS_ID_SSID = 0
            U_WIFI_STATUS_ID_BSSID = 1
            U_WIFI_STATUS_ID_CHANNEL = 2
            U_WIFI_STATUS_ID_CONNECTION = 3
            U_WIFI_STATUS_ID_RSSI = 4
            
            # Set up function signatures
            func_begin = self.dll.uCxWifiStationStatusBegin
            func_begin.argtypes = [ctypes.POINTER(ctypes.c_void_p), ctypes.c_int32, ctypes.POINTER(WifiStationStatus)]
            func_begin.restype = ctypes.c_bool
            
            func_end = self.dll.uCxEnd
            func_end.argtypes = [ctypes.POINTER(ctypes.c_void_p)]
            func_end.restype = ctypes.c_int32
            
            # Get connection status first
            status_connection = WifiStationStatus()
            debug_print("[DEBUG] Calling uCxWifiStationStatusBegin for connection status...")
            result = func_begin(ctypes.byref(self.ucx_handle), U_WIFI_STATUS_ID_CONNECTION, ctypes.byref(status_connection))
            end_result = func_end(ctypes.byref(self.ucx_handle))
            
            debug_print(f"[DEBUG] uCxWifiStationStatusBegin returned: {result}, uCxEnd: {end_result}")
            
            if not result or end_result != 0:
                return APICallResult(
                    success=True,
                    result_data="WiFi Status: Not connected or WiFi disabled"
                )
            
            # Connection status: 1 = not connected, 2 = connected
            connection_state = status_connection.rspWifiStatusIdInt.int_val
            debug_print(f"[DEBUG] Connection state: {connection_state}")
            
            if connection_state != 2:
                return APICallResult(
                    success=True,
                    result_data="WiFi Status: Not connected"
                )
            
            # Get SSID
            status_ssid = WifiStationStatus()
            result_ssid = func_begin(ctypes.byref(self.ucx_handle), U_WIFI_STATUS_ID_SSID, ctypes.byref(status_ssid))
            end_result_ssid = func_end(ctypes.byref(self.ucx_handle))
            
            ssid = ""
            if result_ssid and end_result_ssid == 0:
                ssid = status_ssid.rspWifiStatusIdStr.ssid.decode('utf-8') if status_ssid.rspWifiStatusIdStr.ssid else "Unknown"
            
            # Get RSSI
            status_rssi = WifiStationStatus()
            result_rssi = func_begin(ctypes.byref(self.ucx_handle), U_WIFI_STATUS_ID_RSSI, ctypes.byref(status_rssi))
            end_result_rssi = func_end(ctypes.byref(self.ucx_handle))
            
            rssi = ""
            if result_rssi and end_result_rssi == 0:
                rssi_val = status_rssi.rspWifiStatusIdInt.int_val
                if rssi_val != -32768:  # -32768 means not connected
                    rssi = f"\nRSSI: {rssi_val} dBm"
            
            return APICallResult(
                success=True,
                result_data=f"WiFi Status: Connected\nSSID: {ssid}{rssi}"
            )
                
        except Exception as e:
            import traceback
            debug_print(f"[DEBUG] WiFi status exception: {traceback.format_exc()}")
            return APICallResult(success=False, error_message=f"Get WiFi status failed: {e}")

    
    def validate_api_mapping(self, at_command: str) -> Tuple[bool, str]:
        """Validate that an AT command has a proper API mapping"""
        mapping = at_to_api_mapper.get_api_mapping(at_command)
        
        return True, f"Valid mapping: {at_command} -> {mapping.api_function}"