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
    
    print("\nDone! Include these headers in your C code:")
    print(f'  #include "{company_h.name}"')
    print(f'  #include "{service_h.name}"')

if __name__ == '__main__':
    main()
