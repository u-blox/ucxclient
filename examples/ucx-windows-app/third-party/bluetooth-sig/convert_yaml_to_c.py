#!/usr/bin/env python3
"""
Convert Bluetooth SIG YAML files to C header files with lookup tables.
"""

import yaml
import sys
from pathlib import Path

def convert_company_identifiers(yaml_file, output_file):
    """Convert company_identifiers.yaml to C header."""
    
    with open(yaml_file, 'r', encoding='utf-8') as f:
        data = yaml.safe_load(f)
    
    companies = data['company_identifiers']
    
    with open(output_file, 'w', encoding='utf-8') as f:
        f.write("""/*
 * Copyright 2025 u-blox
 *
 * Auto-generated from Bluetooth SIG company_identifiers.yaml
 * Do not edit manually - regenerate using convert_yaml_to_c.py
 */

#ifndef BT_COMPANY_IDENTIFIERS_H
#define BT_COMPANY_IDENTIFIERS_H

#include <stdint.h>

typedef struct {
    uint16_t id;
    const char *name;
} BtCompany_t;

static const BtCompany_t BT_COMPANIES[] = {
""")
        
        for company in companies:
            company_id = company['value']
            company_name = company['name'].replace('"', '\\"')  # Escape quotes
            f.write(f'    {{{company_id}, "{company_name}"}},\n')
        
        f.write(f"""}};\n
#define BT_COMPANIES_COUNT {len(companies)}

static inline const char* btGetCompanyName(uint16_t companyId) {{
    for (int i = 0; i < BT_COMPANIES_COUNT; i++) {{
        if (BT_COMPANIES[i].id == companyId) {{
            return BT_COMPANIES[i].name;
        }}
    }}
    return NULL;
}}

#endif /* BT_COMPANY_IDENTIFIERS_H */
""")
    
    print(f"Generated {output_file} with {len(companies)} companies")

def convert_service_uuids(yaml_file, output_file):
    """Convert service_uuids.yaml to C header."""
    
    with open(yaml_file, 'r', encoding='utf-8') as f:
        data = yaml.safe_load(f)
    
    services = data['uuids']
    
    with open(output_file, 'w', encoding='utf-8') as f:
        f.write("""/*
 * Copyright 2025 u-blox
 *
 * Auto-generated from Bluetooth SIG service_uuids.yaml
 * Do not edit manually - regenerate using convert_yaml_to_c.py
 */

#ifndef BT_SERVICE_UUIDS_H
#define BT_SERVICE_UUIDS_H

#include <stdint.h>

typedef struct {
    uint16_t uuid;
    const char *name;
} BtService_t;

static const BtService_t BT_SERVICES[] = {
""")
        
        for service in services:
            uuid = service['uuid']
            name = service['name'].replace('"', '\\"')  # Escape quotes
            f.write(f'    {{{uuid}, "{name}"}},\n')
        
        f.write(f"""}};\n
#define BT_SERVICES_COUNT {len(services)}

static inline const char* btGetServiceName(uint16_t uuid) {{
    for (int i = 0; i < BT_SERVICES_COUNT; i++) {{
        if (BT_SERVICES[i].uuid == uuid) {{
            return BT_SERVICES[i].name;
        }}
    }}
    return NULL;
}}

#endif /* BT_SERVICE_UUIDS_H */
""")
    
    print(f"Generated {output_file} with {len(services)} services")

def convert_appearance_values(yaml_file, output_file):
    """Convert appearance_values.yaml to C header."""
    
    with open(yaml_file, 'r', encoding='utf-8') as f:
        data = yaml.safe_load(f)
    
    appearances = data['appearance_values']
    
    # Build a flat list of all appearance values
    appearance_list = []
    
    for category_entry in appearances:
        category = category_entry['category']
        category_name = category_entry['name']
        
        # Add the main category (subcategory 0)
        appearance_value = (category << 6) | 0
        appearance_list.append((appearance_value, category_name))
        
        # Add subcategories if they exist
        if 'subcategory' in category_entry:
            for subcat in category_entry['subcategory']:
                subcat_value = subcat['value']
                subcat_name = subcat['name']
                appearance_value = (category << 6) | subcat_value
                appearance_list.append((appearance_value, subcat_name))
    
    with open(output_file, 'w', encoding='utf-8') as f:
        f.write("""/*
 * Copyright 2025 u-blox
 *
 * Auto-generated from Bluetooth SIG appearance_values.yaml
 * Do not edit manually - regenerate using convert_yaml_to_c.py
 */

#ifndef BT_APPEARANCE_VALUES_H
#define BT_APPEARANCE_VALUES_H

#include <stdint.h>

typedef struct {
    uint16_t value;
    const char *name;
} BtAppearance_t;

static const BtAppearance_t BT_APPEARANCES[] = {
""")
        
        for value, name in appearance_list:
            name_escaped = name.replace('"', '\\"')  # Escape quotes
            f.write(f'    {{{value}, "{name_escaped}"}},\n')
        
        f.write(f"""}};\n
#define BT_APPEARANCES_COUNT {len(appearance_list)}

static inline const char* btGetAppearanceName(uint16_t appearance) {{
    for (int i = 0; i < BT_APPEARANCES_COUNT; i++) {{
        if (BT_APPEARANCES[i].value == appearance) {{
            return BT_APPEARANCES[i].name;
        }}
    }}
    return NULL;
}}

#endif /* BT_APPEARANCE_VALUES_H */
""")
    
    print(f"Generated {output_file} with {len(appearance_list)} appearance values")

def convert_characteristic_uuids(yaml_file, output_file):
    """Convert characteristic_uuids.yaml to C header."""
    
    with open(yaml_file, 'r', encoding='utf-8') as f:
        data = yaml.safe_load(f)
    
    characteristics = data['uuids']
    
    with open(output_file, 'w', encoding='utf-8') as f:
        f.write("""/*
 * Copyright 2025 u-blox
 *
 * Auto-generated from Bluetooth SIG characteristic_uuids.yaml
 * Do not edit manually - regenerate using convert_yaml_to_c.py
 */

#ifndef BT_CHARACTERISTIC_UUIDS_H
#define BT_CHARACTERISTIC_UUIDS_H

#include <stdint.h>

typedef struct {
    uint16_t uuid;
    const char *name;
} BtCharacteristic_t;

static const BtCharacteristic_t BT_CHARACTERISTICS[] = {
""")
        
        for char in characteristics:
            uuid = char['uuid']
            name = char['name'].replace('"', '\\"')  # Escape quotes
            f.write(f'    {{0x{uuid:04X}, "{name}"}},\n')
        
        f.write(f"""}};\n
#define BT_CHARACTERISTICS_COUNT {len(characteristics)}

static inline const char* btGetCharacteristicName(uint16_t uuid) {{
    for (int i = 0; i < BT_CHARACTERISTICS_COUNT; i++) {{
        if (BT_CHARACTERISTICS[i].uuid == uuid) {{
            return BT_CHARACTERISTICS[i].name;
        }}
    }}
    return NULL;
}}

#endif /* BT_CHARACTERISTIC_UUIDS_H */
""")
    
    print(f"Generated {output_file} with {len(characteristics)} characteristics")

def convert_descriptor_uuids(yaml_file, output_file):
    """Convert descriptor_uuids.yaml to C header."""
    
    with open(yaml_file, 'r', encoding='utf-8') as f:
        data = yaml.safe_load(f)
    
    descriptors = data['uuids']
    
    with open(output_file, 'w', encoding='utf-8') as f:
        f.write("""/*
 * Copyright 2025 u-blox
 *
 * Auto-generated from Bluetooth SIG descriptor_uuids.yaml
 * Do not edit manually - regenerate using convert_yaml_to_c.py
 */

#ifndef BT_DESCRIPTOR_UUIDS_H
#define BT_DESCRIPTOR_UUIDS_H

#include <stdint.h>

typedef struct {
    uint16_t uuid;
    const char *name;
} BtDescriptor_t;

static const BtDescriptor_t BT_DESCRIPTORS[] = {
""")
        
        for desc in descriptors:
            uuid = desc['uuid']
            name = desc['name'].replace('"', '\\"')  # Escape quotes
            f.write(f'    {{0x{uuid:04X}, "{name}"}},\n')
        
        f.write(f"""}};\n
#define BT_DESCRIPTORS_COUNT {len(descriptors)}

static inline const char* btGetDescriptorName(uint16_t uuid) {{
    for (int i = 0; i < BT_DESCRIPTORS_COUNT; i++) {{
        if (BT_DESCRIPTORS[i].uuid == uuid) {{
            return BT_DESCRIPTORS[i].name;
        }}
    }}
    return NULL;
}}

#endif /* BT_DESCRIPTOR_UUIDS_H */
""")
    
    print(f"Generated {output_file} with {len(descriptors)} descriptors")

def convert_units(yaml_file, output_file):
    """Convert units.yaml to C header."""
    
    with open(yaml_file, 'r', encoding='utf-8') as f:
        data = yaml.safe_load(f)
    
    units = data['uuids']
    
    with open(output_file, 'w', encoding='utf-8') as f:
        f.write("""/*
 * Copyright 2025 u-blox
 *
 * Auto-generated from Bluetooth SIG units.yaml
 * Do not edit manually - regenerate using convert_yaml_to_c.py
 */

#ifndef BT_UNITS_H
#define BT_UNITS_H

#include <stdint.h>

typedef struct {
    uint16_t uuid;
    const char *name;
    const char *symbol;  // Common symbol (e.g., "m", "kg", "°C")
} BtUnit_t;

static const BtUnit_t BT_UNITS[] = {
""")
        
        for unit in units:
            uuid = unit['uuid']
            name = unit['name'].replace('"', '\\"')  # Escape quotes
            
            # Extract a short symbol from the name if possible
            symbol = ""
            if "metre" in name:
                symbol = "m"
            elif "kilogram" in name:
                symbol = "kg"
            elif "second" in name:
                symbol = "s"
            elif "ampere" in name:
                symbol = "A"
            elif "kelvin" in name:
                symbol = "K"
            elif "celsius" in name.lower():
                symbol = "°C"
            elif "fahrenheit" in name.lower():
                symbol = "°F"
            elif "pascal" in name:
                symbol = "Pa"
            elif "joule" in name:
                symbol = "J"
            elif "watt" in name:
                symbol = "W"
            elif "volt" in name:
                symbol = "V"
            elif "ohm" in name:
                symbol = "Ω"
            elif "coulomb" in name:
                symbol = "C"
            elif "farad" in name:
                symbol = "F"
            elif "henry" in name:
                symbol = "H"
            elif "hertz" in name:
                symbol = "Hz"
            elif "lumen" in name:
                symbol = "lm"
            elif "lux" in name:
                symbol = "lx"
            elif "weber" in name:
                symbol = "Wb"
            elif "tesla" in name:
                symbol = "T"
            elif "degree" in name and "angle" in name:
                symbol = "°"
            elif "radian" in name:
                symbol = "rad"
            elif "litre" in name or "liter" in name:
                symbol = "L"
            elif "gram" in name and "kilogram" not in name:
                symbol = "g"
            elif "percentage" in name or "percent" in name:
                symbol = "%"
            
            f.write(f'    {{0x{uuid:04X}, "{name}", "{symbol}"}},\n')
        
        f.write(f"""}};\n
#define BT_UNITS_COUNT {len(units)}

static inline const char* btGetUnitName(uint16_t uuid) {{
    for (int i = 0; i < BT_UNITS_COUNT; i++) {{
        if (BT_UNITS[i].uuid == uuid) {{
            return BT_UNITS[i].name;
        }}
    }}
    return NULL;
}}

static inline const char* btGetUnitSymbol(uint16_t uuid) {{
    for (int i = 0; i < BT_UNITS_COUNT; i++) {{
        if (BT_UNITS[i].uuid == uuid) {{
            return BT_UNITS[i].symbol;
        }}
    }}
    return NULL;
}}

#endif /* BT_UNITS_H */
""")
    
    print(f"Generated {output_file} with {len(units)} units")

def main():
    script_dir = Path(__file__).parent
    
    # Convert company identifiers
    company_yaml = script_dir / 'company_identifiers.yaml'
    company_h = script_dir / 'bt_company_identifiers.h'
    if company_yaml.exists():
        convert_company_identifiers(company_yaml, company_h)
    
    # Convert service UUIDs
    service_yaml = script_dir / 'service_uuids.yaml'
    service_h = script_dir / 'bt_service_uuids.h'
    if service_yaml.exists():
        convert_service_uuids(service_yaml, service_h)
    
    # Convert appearance values
    appearance_yaml = script_dir / 'appearance_values.yaml'
    appearance_h = script_dir / 'bt_appearance_values.h'
    if appearance_yaml.exists():
        convert_appearance_values(appearance_yaml, appearance_h)
    
    # Convert characteristic UUIDs
    char_yaml = script_dir / 'characteristic_uuids.yaml'
    char_h = script_dir / 'bt_characteristic_uuids.h'
    if char_yaml.exists():
        convert_characteristic_uuids(char_yaml, char_h)
    
    # Convert descriptor UUIDs
    desc_yaml = script_dir / 'descriptor_uuids.yaml'
    desc_h = script_dir / 'bt_descriptor_uuids.h'
    if desc_yaml.exists():
        convert_descriptor_uuids(desc_yaml, desc_h)
    
    # Convert units
    units_yaml = script_dir / 'units.yaml'
    units_h = script_dir / 'bt_units.h'
    if units_yaml.exists():
        convert_units(units_yaml, units_h)
    
    print("\nDone! Include these headers in your C code:")
    print(f'  #include "{company_h.name}"')
    print(f'  #include "{service_h.name}"')
    print(f'  #include "{appearance_h.name}"')
    if char_yaml.exists():
        print(f'  #include "{char_h.name}"')
    if desc_yaml.exists():
        print(f'  #include "{desc_h.name}"')
    if units_yaml.exists():
        print(f'  #include "{units_h.name}"')

if __name__ == '__main__':
    main()
