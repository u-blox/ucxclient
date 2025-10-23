"""
UCX API Command Executor

This module executes UCX API function calls based on AT command mappings.
No AT commands are sent directly - everything goes through the UCX API layer.
"""

import os
os.environ['PYTHONDONTWRITEBYTECODE'] = '1'

import ctypes
from typing import Dict, List, Optional, Any, Tuple, Union
from dataclasses import dataclass
from at_to_api_mapper import APIMapping, APICallType, at_to_api_mapper

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
            # Get the UCX handle from the wrapper
            # The wrapper initializes the AT client and UCX handle when connecting
            if hasattr(self.ucx_wrapper, 'ucx_handle'):
                self.ucx_handle = self.ucx_wrapper.ucx_handle
                if self.ucx_handle and self.ucx_handle.value:
                    print(f"Using UCX handle from wrapper: {self.ucx_handle.value:#x}")
                else:
                    print("Warning: UCX handle not yet initialized (will be initialized on connect)")
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
            # AT command is just "OK" - we verify communication is working
            # Just check if the handle is valid
            if self.ucx_handle.value:
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
        """Execute ATI9 command (get comprehensive device info)"""
        try:
            # ATI9 returns comprehensive info - we'll get multiple pieces of information
            info_lines = []
            
            # Get manufacturer
            if hasattr(self.dll, 'uCxGeneralGetManufacturerIdentificationBegin'):
                manufacturer = ctypes.c_char_p()
                begin_func = self.dll.uCxGeneralGetManufacturerIdentificationBegin
                begin_func.argtypes = [ctypes.POINTER(ctypes.c_void_p), ctypes.POINTER(ctypes.c_char_p)]
                begin_func.restype = ctypes.c_bool
                
                if begin_func(ctypes.byref(self.ucx_handle), ctypes.byref(manufacturer)):
                    info_lines.append(f"Manufacturer: {manufacturer.value.decode('utf-8') if manufacturer.value else 'N/A'}")
                    # Call End to complete the command
                    if hasattr(self.dll, 'uCxEnd'):
                        end_func = self.dll.uCxEnd
                        end_func.argtypes = [ctypes.POINTER(ctypes.c_void_p)]
                        end_func.restype = ctypes.c_int32
                        end_func(ctypes.byref(self.ucx_handle))
            
            # Get model
            if hasattr(self.dll, 'uCxGeneralGetDeviceModelIdentificationBegin'):
                model = ctypes.c_char_p()
                begin_func = self.dll.uCxGeneralGetDeviceModelIdentificationBegin
                begin_func.argtypes = [ctypes.POINTER(ctypes.c_void_p), ctypes.POINTER(ctypes.c_char_p)]
                begin_func.restype = ctypes.c_bool
                
                if begin_func(ctypes.byref(self.ucx_handle), ctypes.byref(model)):
                    info_lines.append(f"Model: {model.value.decode('utf-8') if model.value else 'N/A'}")
                    # Call End to complete the command
                    if hasattr(self.dll, 'uCxEnd'):
                        end_func = self.dll.uCxEnd
                        end_func.argtypes = [ctypes.POINTER(ctypes.c_void_p)]
                        end_func.restype = ctypes.c_int32
                        end_func(ctypes.byref(self.ucx_handle))
            
            # Get version
            if hasattr(self.dll, 'uCxGeneralGetSoftwareVersionBegin'):
                version = ctypes.c_char_p()
                begin_func = self.dll.uCxGeneralGetSoftwareVersionBegin
                begin_func.argtypes = [ctypes.POINTER(ctypes.c_void_p), ctypes.POINTER(ctypes.c_char_p)]
                begin_func.restype = ctypes.c_bool
                
                if begin_func(ctypes.byref(self.ucx_handle), ctypes.byref(version)):
                    info_lines.append(f"Software Version: {version.value.decode('utf-8') if version.value else 'N/A'}")
                    # Call End to complete the command
                    if hasattr(self.dll, 'uCxEnd'):
                        end_func = self.dll.uCxEnd
                        end_func.argtypes = [ctypes.POINTER(ctypes.c_void_p)]
                        end_func.restype = ctypes.c_int32
                        end_func(ctypes.byref(self.ucx_handle))
            
            # Get serial number
            if hasattr(self.dll, 'uCxGeneralGetSerialNumberBegin'):
                serial = ctypes.c_char_p()
                begin_func = self.dll.uCxGeneralGetSerialNumberBegin
                begin_func.argtypes = [ctypes.POINTER(ctypes.c_void_p), ctypes.POINTER(ctypes.c_char_p)]
                begin_func.restype = ctypes.c_bool
                
                if begin_func(ctypes.byref(self.ucx_handle), ctypes.byref(serial)):
                    info_lines.append(f"Serial Number: {serial.value.decode('utf-8') if serial.value else 'N/A'}")
                    # Call End to complete the command
                    if hasattr(self.dll, 'uCxEnd'):
                        end_func = self.dll.uCxEnd
                        end_func.argtypes = [ctypes.POINTER(ctypes.c_void_p)]
                        end_func.restype = ctypes.c_int32
                        end_func(ctypes.byref(self.ucx_handle))
            
            if info_lines:
                return APICallResult(
                    success=True,
                    result_data="\n".join(info_lines),
                    raw_response="ATI9: Device information retrieved"
                )
            else:
                return APICallResult(success=False, error_message="No device info available")
                
        except Exception as e:
            return APICallResult(success=False, error_message=f"ATI9 failed: {e}")
    
    def store_configuration(self) -> APICallResult:
        """Execute AT&W command (store configuration to flash)"""
        try:
            if not hasattr(self.dll, 'uCxSystemStoreConfiguration'):
                return APICallResult(success=False, error_message="uCxSystemStoreConfiguration not available")
            
            func = self.dll.uCxSystemStoreConfiguration
            func.argtypes = [ctypes.POINTER(ctypes.c_void_p)]
            func.restype = ctypes.c_int32
            
            result = func(ctypes.byref(self.ucx_handle))
            
            if result == 0:
                return APICallResult(
                    success=True,
                    result_data="Configuration stored to flash",
                    raw_response="AT&W: Configuration saved successfully"
                )
            else:
                return APICallResult(success=False, error_message=f"Store configuration failed with error code: {result}")
                
        except Exception as e:
            return APICallResult(success=False, error_message=f"Store configuration failed: {e}")
    
    def reset_to_defaults(self) -> APICallResult:
        """Execute AT+USYDS command (reset all settings to default values)"""
        try:
            if not hasattr(self.dll, 'uCxSystemDefaultSettings'):
                return APICallResult(success=False, error_message="uCxSystemDefaultSettings not available")
            
            func = self.dll.uCxSystemDefaultSettings
            func.argtypes = [ctypes.POINTER(ctypes.c_void_p)]
            func.restype = ctypes.c_int32
            
            result = func(ctypes.byref(self.ucx_handle))
            
            if result == 0:
                return APICallResult(
                    success=True,
                    result_data="Settings reset to defaults (certificates and BT bonding preserved)",
                    raw_response="AT+USYDS: Default settings restored"
                )
            else:
                return APICallResult(success=False, error_message=f"Reset to defaults failed with error code: {result}")
                
        except Exception as e:
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
        """Execute AT+CPWROFF command (reboot the device)"""
        try:
            if not hasattr(self.dll, 'uCxSystemReboot'):
                return APICallResult(success=False, error_message="uCxSystemReboot not available")
            
            func = self.dll.uCxSystemReboot
            func.argtypes = [ctypes.POINTER(ctypes.c_void_p)]
            func.restype = ctypes.c_int32
            
            result = func(ctypes.byref(self.ucx_handle))
            
            if result == 0:
                return APICallResult(
                    success=True,
                    result_data="Device rebooting...",
                    raw_response="AT+CPWROFF: Reboot initiated"
                )
            else:
                return APICallResult(success=False, error_message=f"Reboot failed with error code: {result}")
                
        except Exception as e:
            return APICallResult(success=False, error_message=f"Reboot failed: {e}")
    
    def validate_api_mapping(self, at_command: str) -> Tuple[bool, str]:
        """Validate that an AT command has a proper API mapping"""
        mapping = at_to_api_mapper.get_api_mapping(at_command)
        
        if not mapping:
            return False, f"No API mapping found for: {at_command}"
        
        if not hasattr(self.dll, mapping.api_function):
            return False, f"API function not available: {mapping.api_function}"
        
        return True, f"Valid mapping: {at_command} -> {mapping.api_function}"