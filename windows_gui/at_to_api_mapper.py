"""
AT Command to UCX API Mapper

This module creates dynamic mappings between AT commands from device manuals
and the corresponding ucx_client API functions. No AT commands are sent directly -
everything goes through the proper API layer.
"""

import os
os.environ['PYTHONDONTWRITEBYTECODE'] = '1'

import re
from typing import Dict, List, Optional, Callable, Any, Tuple
from dataclasses import dataclass
from enum import Enum

class APICallType(Enum):
    """Types of API calls"""
    SIMPLE = "simple"           # Single function call
    BEGIN_END = "begin_end"     # Begin/End pattern
    ASYNC = "async"            # Asynchronous with callback
    COMPLEX = "complex"        # Multiple API calls required

@dataclass
class APIMapping:
    """Mapping between AT command and UCX API function"""
    at_command: str
    api_function: str
    api_type: APICallType
    parameters: List[str]
    response_parser: Optional[str] = None
    description: str = ""
    example_usage: str = ""

class ATToAPIMapper:
    """Maps AT commands to UCX API function calls - 100% dynamic from YAML"""
    
    def __init__(self, yaml_parser=None):
        self.mappings: Dict[str, APIMapping] = {}
        self.yaml_parser = yaml_parser
        if yaml_parser:
            self._build_mappings_from_yaml()
    
    def set_yaml_parser(self, yaml_parser):
        """Set YAML parser and rebuild mappings dynamically"""
        self.yaml_parser = yaml_parser
        self._build_mappings_from_yaml()
    
    def _build_mappings_from_yaml(self):
        """Build AT-to-API mappings dynamically from YAML parser"""
        if not self.yaml_parser:
            return
        
        self.mappings.clear()
        
        # Get all AT commands from YAML
        for cmd_name in self.yaml_parser.commands.keys():
            cmd_info = self.yaml_parser.get_command_info(cmd_name)
            if not cmd_info:
                continue
            
            api_name = cmd_info.get('api_name')
            if not api_name:
                continue
            
            # Convert YAML api_name to UCX API function name
            # Format: ApiName -> uCxModuleApiNameBegin
            api_function = self._convert_api_name_to_function(cmd_name, api_name, cmd_info)
            
            # Determine API call type based on command characteristics
            api_type = self._determine_api_call_type(cmd_info)
            
            # Extract parameters from syntax
            parameters = self._extract_parameters(cmd_info)
            
            # Create mapping
            self.mappings[cmd_name] = APIMapping(
                at_command=cmd_name,
                api_function=api_function,
                api_type=api_type,
                parameters=parameters,
                description=cmd_info.get('brief', ''),
                response_parser=None
            )
    
    def _convert_api_name_to_function(self, at_cmd: str, api_name: str, cmd_info: Dict) -> str:
        """
        Convert YAML api_name to UCX API function name
        
        Examples:
            GetManufacturerIdentification -> uCxGeneralGetManufacturerIdentificationBegin
            StationScanDefault -> uCxWifiStationScanDefaultBegin
            Connect (Bluetooth) -> uCxBluetoothConnectBegin
        """
        # Determine module prefix based on command group
        group = cmd_info.get('group', '')
        
        module_map = {
            'General': 'uCxGeneral',
            'System': 'uCxSystem',
            'Bluetooth': 'uCxBluetooth',
            'GATT client': 'uCxGattClient',
            'GATT Server': 'uCxGattServer',
            'SPS': 'uCxSps',
            'Wi-Fi': 'uCxWifi',
            'Socket': 'uCxSocket',
            'MQTT': 'uCxMqtt',
            'HTTP': 'uCxHttp',
            'Network Time': 'uCxNetworkTime',
            'Security': 'uCxSecurity',
            'Power': 'uCxPower',
            'Transparent': 'uCxTransparent',
            'Diagnostics': 'uCxDiagnostics'
        }
        
        # Map of group keywords to check if api_name already contains them
        # Maps group name to tuple of (keywords_to_check, replacement)
        group_keywords = {
            'Power': (['PowerManagement'], 'Power'),  # Strip PowerManagement, use Power prefix
            'Transparent': (['Transparent', 'TransparentMode'], None),
            'MQTT': (['Mqtt'], None),
            'Diagnostics': (['Diagnostics'], None),
            'Security': (['Security'], None),
            'System': (['System'], None),
            'General': (['General'], None),
            'Wi-Fi': (['Wi-fi', 'WiFi', 'Wifi'], 'Wifi'),  # Convert Wi-fi/WiFi/Wifi to Wifi
            'Socket': (['Socket'], 'Socket'),  # Strip Socket prefix
        }
        
        prefix = module_map.get(group, 'uCx')
        modified_api_name = api_name
        
        # Check if api_name already starts with a group-related keyword
        if group in group_keywords:
            keywords, replacement = group_keywords[group]
            for keyword in keywords:
                if api_name.startswith(keyword):
                    if replacement:
                        # Special case: Wi-fi/WiFi -> Wifi, keep the group prefix
                        # Strip the keyword and use replacement in the group prefix
                        modified_api_name = api_name[len(keyword):]
                        # prefix stays as uCxWifi (from module_map)
                    else:
                        # Standard case: api_name already has the keyword, use uCx prefix only
                        prefix = 'uCx'
                    break
        
        # Construct function name without "Begin" suffix
        # The executor will automatically try with "Begin" suffix if the simple version doesn't exist
        # This allows both SIMPLE (int32_t return) and BEGIN_END (bool return) patterns to work
        func_name = f"{prefix}{modified_api_name}"
        
        return func_name
    
    def _determine_api_call_type(self, cmd_info: Dict) -> APICallType:
        """Determine API call type from command info
        
        BEGIN_END pattern is used for:
        - Commands with multiline_response=true (multiple results)
        - Get* commands that return data via Begin/GetNext/End pattern
        - Scan/Discovery commands (return lists of results)
        
        SIMPLE pattern is used for:
        - Set* commands (write operations)
        - Action commands (Reboot, Connect, Disconnect, etc.)
        - Get* commands that return int32_t directly (no Begin suffix)
        - Commands that return just a status code (int32_t)
        """
        # Check if command has multiline response
        syntax = cmd_info.get('syntax', {})
        
        for variant in syntax.values():
            if isinstance(variant, dict):
                # Multiline responses always need BEGIN_END pattern
                if variant.get('multiline_response', False):
                    return APICallType.BEGIN_END
                
                api_name = variant.get('api_name', '')
                if api_name:
                    # Get* commands need BEGIN_END (they return data via output parameters)
                    if api_name.startswith('Get'):
                        return APICallType.BEGIN_END
                    
                    # Scan/Discovery commands need BEGIN_END (return multiple results)
                    if 'Scan' in api_name or 'Discovery' in api_name:
                        return APICallType.BEGIN_END
                    
                    # Everything else is SIMPLE (Set*, action commands, etc.)
                    # These typically return int32_t status code
                    return APICallType.SIMPLE
        
        # Default to SIMPLE
        return APICallType.SIMPLE
    
    def _extract_parameters(self, cmd_info: Dict) -> List[str]:
        """Extract parameter names from command info"""
        params = cmd_info.get('parameters', {})
        return list(params.keys()) if params else []
    
    def get_api_mapping(self, at_command: str) -> Optional[APIMapping]:
        """Get API mapping for an AT command"""
        # Normalize AT command (remove parameters, spaces, etc.)
        normalized_cmd = self._normalize_at_command(at_command)
        mapping = self.mappings.get(normalized_cmd)
        
        if mapping:
            # Check if this is a read (?) or write (=) command
            is_read = '?' in at_command
            is_write = '=' in at_command
            
            # If the API name starts with "Set" but this is a read command,
            # we need to use the "Get" version instead
            if is_read and mapping.api_function.startswith('uCx') and 'Set' in mapping.api_function:
                # Convert SetXxx to GetXxx
                api_func = mapping.api_function.replace('Set', 'Get')
                # Create a new mapping with the Get function
                return APIMapping(
                    at_command=mapping.at_command,
                    api_function=api_func,
                    api_type=APICallType.BEGIN_END,  # Get operations are typically BEGIN_END
                    parameters=mapping.parameters,
                    response_parser=mapping.response_parser,
                    description=mapping.description,
                    example_usage=mapping.example_usage
                )
        
        return mapping
    
    def _normalize_at_command(self, at_command: str) -> str:
        """Normalize AT command for mapping lookup"""
        # Remove parameters and whitespace
        cmd = at_command.strip().upper()
        
        # Remove everything after = or ?
        if '=' in cmd:
            cmd = cmd.split('=')[0]
        elif '?' in cmd:
            cmd = cmd.split('?')[0]
            
        return cmd
    
    def get_all_mappings(self) -> Dict[str, APIMapping]:
        """Get all AT to API mappings"""
        return self.mappings.copy()
    
    def add_custom_mapping(self, mapping: APIMapping):
        """Add a custom AT to API mapping"""
        self.mappings[mapping.at_command] = mapping


# Global mapper instance (will be initialized with YAML parser at runtime)
at_to_api_mapper = ATToAPIMapper()