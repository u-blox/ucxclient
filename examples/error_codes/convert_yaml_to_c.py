#!/usr/bin/env python3
"""
Convert u-connectXpress error codes YAML file to C header file.
"""

import yaml
import sys
from pathlib import Path
from datetime import datetime

def convert_error_codes(yaml_file, output_file):
    """Convert error_codes.yaml to C header."""
    
    with open(yaml_file, 'r', encoding='utf-8') as f:
        data = yaml.safe_load(f)
    
    modules = data.get('modules', {})
    title = data.get('title', 'Error Codes')
    description = data.get('description', '')
    
    # Collect all errors
    all_errors = []
    for module_name, module_data in modules.items():
        errors = module_data.get('errors', [])
        for error in errors:
            all_errors.append({
                'module': module_name,
                'name': error['name'],
                'value': error['value']
            })
    
    # Sort by value
    all_errors.sort(key=lambda x: x['value'])
    
    with open(output_file, 'w', encoding='utf-8') as f:
        # Write header comment
        f.write("/*\n")
        f.write(" * Copyright 2025 u-blox\n")
        f.write(" *\n")
        f.write(" * Auto-generated from error_codes.yaml\n")
        f.write(f" * {title}\n")
        f.write(f" * {description}\n")
        f.write(" * \n")
        f.write(f" * Generated on: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n")
        f.write(" * Do not edit manually - regenerate using convert_yaml_to_c.py\n")
        f.write(" */\n\n")
        f.write("#ifndef U_ERROR_CODES_H\n")
        f.write("#define U_ERROR_CODES_H\n\n")
        f.write("#include <stdint.h>\n")
        f.write("#include <stdio.h>\n\n")
        f.write("typedef struct {\n")
        f.write("    int32_t value;\n")
        f.write("    const char *name;\n")
        f.write("    const char *module;\n")
        f.write("} UcxErrorCode_t;\n\n")
        f.write("static const UcxErrorCode_t UCX_ERROR_CODES[] = {\n")
        
        for error in all_errors:
            value = error['value']
            name = error['name']
            module = error['module']
            f.write(f'    {{{value}, "{name}", "{module}"}},\n')
        
        f.write("};\n\n")
        f.write(f"#define UCX_ERROR_CODES_COUNT {len(all_errors)}\n\n")
        f.write("/**\n")
        f.write(" * Get the error name for a given error code\n")
        f.write(" * \n")
        f.write(" * @param errorCode The error code value\n")
        f.write(" * @return The error name string, or NULL if not found\n")
        f.write(" */\n")
        f.write("static inline const char* ucxGetErrorName(int32_t errorCode) {\n")
        f.write("    for (int i = 0; i < UCX_ERROR_CODES_COUNT; i++) {\n")
        f.write("        if (UCX_ERROR_CODES[i].value == errorCode) {\n")
        f.write("            return UCX_ERROR_CODES[i].name;\n")
        f.write("        }\n")
        f.write("    }\n")
        f.write("    return NULL;\n")
        f.write("}\n\n")
        f.write("/**\n")
        f.write(" * Get the module name for a given error code\n")
        f.write(" * \n")
        f.write(" * @param errorCode The error code value\n")
        f.write(" * @return The module name string, or NULL if not found\n")
        f.write(" */\n")
        f.write("static inline const char* ucxGetErrorModule(int32_t errorCode) {\n")
        f.write("    for (int i = 0; i < UCX_ERROR_CODES_COUNT; i++) {\n")
        f.write("        if (UCX_ERROR_CODES[i].value == errorCode) {\n")
        f.write("            return UCX_ERROR_CODES[i].module;\n")
        f.write("        }\n")
        f.write("    }\n")
        f.write("    return NULL;\n")
        f.write("}\n\n")
        f.write("/**\n")
        f.write(" * Get a user-friendly error description\n")
        f.write(" * \n")
        f.write(" * @param errorCode The error code value\n")
        f.write(" * @param buffer Buffer to write the description to\n")
        f.write(" * @param bufferSize Size of the buffer\n")
        f.write(" * @return Number of characters written (excluding null terminator), or -1 if error not found\n")
        f.write(" */\n")
        f.write("static inline int ucxGetErrorDescription(int32_t errorCode, char *buffer, size_t bufferSize) {\n")
        f.write("    const char *name = ucxGetErrorName(errorCode);\n")
        f.write("    const char *module = ucxGetErrorModule(errorCode);\n")
        f.write("    \n")
        f.write("    if (name == NULL || module == NULL) {\n")
        f.write("        return -1;\n")
        f.write("    }\n")
        f.write("    \n")
        f.write("    int written = snprintf(buffer, bufferSize, \"[%s] %s (code %d)\", module, name, errorCode);\n")
        f.write("    return (written < (int)bufferSize) ? written : -1;\n")
        f.write("}\n\n")
        f.write("#endif /* U_ERROR_CODES_H */\n")
    
    print(f"Generated {output_file} with {len(all_errors)} error codes from {len(modules)} modules")
    print(f"Modules: {', '.join(modules.keys())}")

def main():
    script_dir = Path(__file__).parent
    
    # Convert error codes
    error_yaml = script_dir / 'error_codes.yaml'
    error_h = script_dir / 'u_error_codes.h'
    
    if error_yaml.exists():
        convert_error_codes(error_yaml, error_h)
        print(f"\nSuccess! Include this header in your C code:")
        print(f'  #include "error_codes/u_error_codes.h"')
        print(f"\nUsage example:")
        print(f'  const char *errorName = ucxGetErrorName(5);')
        print(f'  // Returns: "U_ERROR_COMMON_INVALID_PARAMETER"')
        print(f'  ')
        print(f'  char description[128];')
        print(f'  ucxGetErrorDescription(5, description, sizeof(description));')
        print(f'  // Returns: "[Common] U_ERROR_COMMON_INVALID_PARAMETER (code 5)"')
    else:
        print(f"Error: {error_yaml} not found!")
        sys.exit(1)

if __name__ == '__main__':
    main()
