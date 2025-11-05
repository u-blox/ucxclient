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

def main():
    script_dir = Path(__file__).parent
    
    # Convert company identifiers
    company_yaml = script_dir / 'company_identifiers.yaml'
    company_h = script_dir / 'bt_company_identifiers.h'
    convert_company_identifiers(company_yaml, company_h)
    
    # Convert service UUIDs
    service_yaml = script_dir / 'service_uuids.yaml'
    service_h = script_dir / 'bt_service_uuids.h'
    convert_service_uuids(service_yaml, service_h)
    
    # Convert appearance values
    appearance_yaml = script_dir / 'appearance_values.yaml'
    appearance_h = script_dir / 'bt_appearance_values.h'
    convert_appearance_values(appearance_yaml, appearance_h)
    
    print("\nDone! Include these headers in your C code:")
    print(f'  #include "{company_h.name}"')
    print(f'  #include "{service_h.name}"')
    print(f'  #include "{appearance_h.name}"')

if __name__ == '__main__':
    main()
