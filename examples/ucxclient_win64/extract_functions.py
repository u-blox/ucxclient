#!/usr/bin/env python3
"""
Script to automatically extract functions from ucxclient_win64.c and split them
into modular files according to the refactoring plan.
"""

import re
import os

# Define the mapping of functions to their target files
FUNCTION_MAPPING = {
    'socket.c': [
        'socketCreateTcp', 'socketCreateUdp', 'socketConnect',
        'socketSendData', 'socketReadData', 'socketClose', 'socketListStatus'
    ],
    'sps.c': [
        'spsEnableService', 'spsConnect', 'spsSendData', 'spsReadData'
    ],
    'gatt.c': [
        'gattClientDiscoverServices', 'gattClientReadCharacteristic',
        'gattClientWriteCharacteristic', 'gattServerAddService',
        'gattServerSetCharacteristic'
    ],
    'bluetooth.c': [
        'showBluetoothStatus', 'bluetoothScan', 'bluetoothConnect',
        'bluetoothFunctionsMenu'
    ],
    'wifi.c': [
        'showWifiStatus', 'wifiScan', 'wifiConnect', 'wifiDisconnect',
        'testConnectivity', 'wifiFunctionsMenu'
    ],
    'device_connection.c': [
        'connectDevice', 'quickConnectToLastDevice', 'disconnectDevice',
        'executeAtTest', 'executeAti9', 'executeModuleReboot',
        'getExecutableDirectory', 'initFtd2xxLibrary'
    ],
    'com_port_detect.c': [
        'listAvailableComPorts', 'selectComPortFromList'
    ],
    'api_commands.c': [
        'fetchApiCommandsFromGitHub', 'parseYamlCommands', 'freeApiCommands',
        'listAllApiCommands', 'fetchLatestVersion', 'httpGetRequest',
        'httpGetBinaryRequest', 'extractProductFromFilename'
    ],
    'firmware_update.c': [
        'firmwareUpdateProgress', 'downloadFirmwareFromGitHub',
        'downloadFirmwareFromGitHubInteractive', 'extractZipFile',
        'saveBinaryFile'
    ],
    'ui_menus.c': [
        'printHeader', 'printWelcomeGuide', 'printHelp', 'printMenu',
        'handleUserInput', 'bluetoothMenu', 'wifiMenu', 'socketMenu',
        'gattClientMenu', 'gattServerMenu', 'securityTlsMenu'
    ]
}

def extract_function(content, func_name):
    """Extract a function from the source content."""
    # Pattern to match: static void funcName(...) or static bool funcName(...)
    pattern = rf'(static\s+\w+\s+{re.escape(func_name)}\s*\([^)]*\)(?:\s*\{{)?)'
    
    match = re.search(pattern, content)
    if not match:
        return None
    
    start = match.start()
    
    # Find the opening brace
    brace_start = content.find('{', start)
    if brace_start == -1:
        return None
    
    # Count braces to find the end
    brace_count = 1
    pos = brace_start + 1
    
    while pos < len(content) and brace_count > 0:
        if content[pos] == '{':
            brace_count += 1
        elif content[pos] == '}':
            brace_count -= 1
        pos += 1
    
    if brace_count == 0:
        return content[start:pos]
    
    return None

def main():
    input_file = '../ucxclient_win64.c'
    output_dir = '.'
    
    print(f"Reading {input_file}...")
    with open(input_file, 'r', encoding='utf-8') as f:
        content = f.read()
    
    print(f"Extracting functions...")
    
    for target_file, functions in FUNCTION_MAPPING.items():
        print(f"\nProcessing {target_file}...")
        output_path = os.path.join(output_dir, target_file)
        
        # Read existing file if it exists (to preserve copyright header)
        if os.path.exists(output_path):
            with open(output_path, 'r', encoding='utf-8') as f:
                existing = f.read()
                # Extract copyright header
                header_end = existing.find('#include')
                if header_end > 0:
                    header = existing[:header_end]
                else:
                    header = ""
        else:
            header = """/*
 * Copyright 2025 u-blox
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

"""
        
        # Start building the new file
        module_name = target_file.replace('.c', '')
        new_content = header
        new_content += f'#include "{module_name}.h"\n\n'
        
        # Extract each function
        extracted_count = 0
        for func_name in functions:
            func_code = extract_function(content, func_name)
            if func_code:
                new_content += func_code + '\n\n'
                extracted_count += 1
                print(f"  ✓ Extracted {func_name}")
            else:
                print(f"  ✗ Could not find {func_name}")
        
        # Write the file
        with open(output_path, 'w', encoding='utf-8') as f:
            f.write(new_content)
        
        print(f"  Wrote {extracted_count}/{len(functions)} functions to {target_file}")
    
    print("\n✓ Extraction complete!")
    print("\nNote: You may need to:")
    print("  1. Remove 'static' keywords from function definitions")
    print("  2. Add any helper functions that were missed")
    print("  3. Check for compilation errors and fix includes")

if __name__ == '__main__':
    main()
