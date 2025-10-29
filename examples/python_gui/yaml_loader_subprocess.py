"""
Standalone YAML loader process - loads YAML and saves as JSON
This avoids crashes in the GUI process
"""
import sys
import yaml
import json

if len(sys.argv) != 3:
    print("Usage: yaml_loader_subprocess.py <input_yaml> <output_json>")
    sys.exit(1)

input_file = sys.argv[1]
output_file = sys.argv[2]

try:
    print(f"Loading YAML from: {input_file}")
    sys.setrecursionlimit(5000)
    
    with open(input_file, 'r', encoding='utf-8') as f:
        content = f.read()
    
    print(f"Read {len(content)} characters")
    print("Parsing YAML...")
    
    data = yaml.load(content, Loader=yaml.SafeLoader)
    
    print(f"Parsed successfully, writing to JSON...")
    
    with open(output_file, 'w', encoding='utf-8') as f:
        json.dump(data, f, indent=2)
    
    print(f"Success! JSON written to: {output_file}")
    sys.exit(0)
    
except Exception as e:
    print(f"ERROR: {e}")
    import traceback
    traceback.print_exc()
    sys.exit(1)
