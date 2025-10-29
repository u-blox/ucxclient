"""
YAML-based AT command parser for u-connectXpress

Replaces MD file parsing with structured YAML from GitHub or local files.
Supports product and version selection.
"""

import os
os.environ['PYTHONDONTWRITEBYTECODE'] = '1'

import yaml
# Force pure Python implementation to avoid C extension crashes in GUI context
import sys
if hasattr(yaml, 'CSafeLoader'):
    # Disable C loaders that might crash in Tkinter environment
    delattr(yaml, 'CSafeLoader')
if hasattr(yaml, 'CLoader'):
    delattr(yaml, 'CLoader')

import urllib.request
import urllib.error
import os
import json
import re
from typing import Dict, List, Optional, Tuple
from dataclasses import dataclass


@dataclass
class ProductVersion:
    """Product version information"""
    product: str
    version: str
    date: str


class YAMLCommandParser:
    """Parser for compiled_product.yaml files from u-connectXpress"""
    
    GITHUB_BASE_URL = 'https://raw.githubusercontent.com/u-blox/u-connectXpress/main'
    GITHUB_API_URL = 'https://api.github.com/repos/u-blox/u-connectXpress/contents'
    
    # Cache for GitHub directory listings
    _github_cache: Dict[str, List[str]] = {}
    
    def __init__(self):
        self.yaml_data: Optional[Dict] = None
        self.product: Optional[str] = None
        self.version: Optional[str] = None
        self.commands: Dict[str, Dict] = {}
        self.command_groups: Dict[str, List[str]] = {}
        
    @classmethod
    def get_available_products(cls) -> List[str]:
        """
        Get list of available products from GitHub
        
        Returns:
            List of product names (e.g., ['NORA-W36', 'NORA-B26'])
        """
        if 'products' in cls._github_cache:
            return cls._github_cache['products']
        
        try:
            print("Fetching available products from GitHub...")
            req = urllib.request.Request(cls.GITHUB_API_URL)
            req.add_header('Accept', 'application/vnd.github.v3+json')
            
            with urllib.request.urlopen(req, timeout=10) as response:
                data = json.loads(response.read().decode('utf-8'))
                
                # Filter for directories that look like product names (NORA-*)
                products = []
                for item in data:
                    if item['type'] == 'dir' and item['name'].startswith('NORA-'):
                        products.append(item['name'])
                
                products.sort()
                cls._github_cache['products'] = products
                print(f"Found {len(products)} products: {', '.join(products)}")
                return products
                
        except Exception as e:
            print(f"Failed to fetch products from GitHub: {e}")
            # Fallback to known products
            fallback = ['NORA-W36', 'NORA-B26', 'NORA-W46', 'NORA-W56']
            cls._github_cache['products'] = fallback
            return fallback
    
    @classmethod
    def get_versions_for_product(cls, product: str) -> List[str]:
        """
        Get available versions for a product from GitHub
        
        Args:
            product: Product name (e.g., 'NORA-W36')
            
        Returns:
            List of version strings (e.g., ['3.1.0', '3.0.0'])
        """
        cache_key = f'versions_{product}'
        if cache_key in cls._github_cache:
            return cls._github_cache[cache_key]
        
        try:
            print(f"Fetching versions for {product} from GitHub...")
            url = f"{cls.GITHUB_API_URL}/{product}"
            req = urllib.request.Request(url)
            req.add_header('Accept', 'application/vnd.github.v3+json')
            
            with urllib.request.urlopen(req, timeout=10) as response:
                data = json.loads(response.read().decode('utf-8'))
                
                # Filter for directories that look like versions (X.Y.Z format)
                versions = []
                version_pattern = re.compile(r'^\d+\.\d+\.\d+$')
                
                for item in data:
                    if item['type'] == 'dir' and version_pattern.match(item['name']):
                        versions.append(item['name'])
                
                # Sort versions (newest first)
                versions.sort(key=lambda v: [int(x) for x in v.split('.')], reverse=True)
                
                cls._github_cache[cache_key] = versions
                print(f"Found {len(versions)} versions for {product}: {', '.join(versions)}")
                return versions
                
        except Exception as e:
            print(f"Failed to fetch versions for {product}: {e}")
            # Return empty list if we can't fetch
            cls._github_cache[cache_key] = []
            return []
    
    def load_from_github(self, product: str, version: str) -> bool:
        """
        Load YAML from GitHub
        
        Args:
            product: Product name (e.g., 'NORA-W36')
            version: Version string (e.g., '3.1.0')
            
        Returns:
            True if successful, False otherwise
        """
        url = f"{self.GITHUB_BASE_URL}/{product}/{version}/compiled_product.yaml"
        
        try:
            print(f"Fetching YAML from: {url}")
            with urllib.request.urlopen(url, timeout=10) as response:
                yaml_content = response.read().decode('utf-8')
                self.yaml_data = yaml.safe_load(yaml_content)
                self.product = product
                self.version = version
                self._parse_commands()
                print(f"Successfully loaded {product} v{version}")
                return True
                
        except urllib.error.HTTPError as e:
            if e.code == 404:
                print(f"Note: YAML not available for {product} v{version} (404 - this is expected for some products)")
                return False
            print(f"Failed to fetch YAML from GitHub: HTTP {e.code} - {e.reason}")
            return False
        except urllib.error.URLError as e:
            print(f"Failed to fetch YAML from GitHub: {e}")
            return False
        except yaml.YAMLError as e:
            print(f"Failed to parse YAML: {e}")
            return False
    
    def load_from_file(self, filepath: str) -> bool:
        """
        Load YAML from local file
        
        Args:
            filepath: Path to compiled_product.yaml file
            
        Returns:
            True if successful, False otherwise
        """
        try:
            print(f"Loading YAML from: {filepath}")
            print("Step 1: Opening file...")
            
            # Check file size first
            import os
            file_size = os.path.getsize(filepath)
            print(f"File size: {file_size / 1024 / 1024:.2f} MB")
            
            # WORKAROUND: YAML parsing crashes in GUI context
            # Load YAML in a separate process and convert to JSON
            print("Step 2a: Loading YAML via subprocess (workaround for GUI crash)...")
            import subprocess
            import tempfile
            import json
            
            # Create temp file for JSON output
            temp_json = tempfile.NamedTemporaryFile(mode='w', suffix='.json', delete=False)
            temp_json_path = temp_json.name
            temp_json.close()
            
            try:
                # Get path to subprocess script
                script_dir = os.path.dirname(os.path.abspath(__file__))
                subprocess_script = os.path.join(script_dir, 'yaml_loader_subprocess.py')
                
                print(f"Step 2b: Running: python {subprocess_script}")
                sys.stdout.flush()
                
                # Run subprocess to load YAML
                result = subprocess.run(
                    [sys.executable, subprocess_script, filepath, temp_json_path],
                    capture_output=True,
                    text=True,
                    timeout=30
                )
                
                print(f"Step 2c: Subprocess output:\n{result.stdout}")
                if result.stderr:
                    print(f"Subprocess stderr:\n{result.stderr}")
                
                if result.returncode != 0:
                    print(f"Subprocess failed with code {result.returncode}")
                    return False
                
                # Load the JSON
                print("Step 2d: Loading JSON result...")
                with open(temp_json_path, 'r', encoding='utf-8') as f:
                    self.yaml_data = json.load(f)
                
                print("Step 2e: YAML loaded successfully via subprocess")
                
            finally:
                # Clean up temp file
                try:
                    os.unlink(temp_json_path)
                except:
                    pass
            
            print("Step 3: Extracting product info...")
            # Extract product info from YAML
            if self.yaml_data and 'product' in self.yaml_data:
                print("Step 4: Found product section...")
                self.product = self.yaml_data['product'].get('model_name', 'Unknown')
                version_info = self.yaml_data['product'].get('software', {}).get('version', {})
                self.version = f"{version_info.get('major', 0)}.{version_info.get('minor', 0)}.{version_info.get('patch', 0)}"
                print(f"Step 5: Extracted product={self.product}, version={self.version}")
            else:
                # Fallback if YAML doesn't have expected structure
                print("Step 4: No product section found, using defaults")
                self.product = 'Unknown Product'
                self.version = '0.0.0'
            
            print("Step 6: Parsing commands...")
            self._parse_commands()
            print(f"Step 7: Successfully loaded {self.product} v{self.version}")
            return True
            
        except FileNotFoundError:
            print(f"File not found: {filepath}")
            return False
        except yaml.YAMLError as e:
            print(f"Failed to parse YAML: {e}")
            import traceback
            traceback.print_exc()
            return False
        except MemoryError:
            print(f"Out of memory loading YAML file (file too large)")
            return False
        except Exception as e:
            print(f"Failed to load YAML from file: {e}")
            import traceback
            traceback.print_exc()
            return False
    
    def _parse_commands(self):
        """Parse command structure from YAML"""
        if not self.yaml_data or 'command_groups' not in self.yaml_data:
            print("Warning: No command_groups found in YAML data")
            return
        
        self.commands.clear()
        self.command_groups.clear()
        
        try:
            for group_name, group_data in self.yaml_data['command_groups'].items():
                commands_in_group = []
                
                if 'commands' in group_data:
                    for cmd_name, cmd_data in group_data['commands'].items():
                        try:
                            # Extract api_name, fallback to brief if not found
                            api_name = self._extract_api_name(cmd_data)
                            if not api_name:
                                # Use brief as fallback (e.g., "Transparent Mode" -> "TransparentMode")
                                brief = cmd_data.get('brief', '')
                                if brief:
                                    # Remove spaces and special chars to create api_name
                                    api_name = ''.join(word.capitalize() for word in brief.split())
                            
                            self.commands[cmd_name] = {
                                'group': group_name,
                                'brief': cmd_data.get('brief', ''),
                                'desc': cmd_data.get('desc', ''),
                                'syntax': cmd_data.get('syntax', {}),
                                'parameters': cmd_data.get('parameters', {}),
                                'api_name': api_name,
                                'storable': self._is_storable(cmd_data),
                                'requires_reboot': self._requires_reboot(cmd_data),
                            }
                            commands_in_group.append(cmd_name)
                        except Exception as e:
                            print(f"Warning: Failed to parse command '{cmd_name}': {e}")
                            continue
                
                self.command_groups[group_name] = commands_in_group
        except Exception as e:
            print(f"Error parsing commands: {e}")
            import traceback
            traceback.print_exc()
    
    def _extract_api_name(self, cmd_data: Dict) -> Optional[str]:
        """Extract API name from command syntax"""
        if 'syntax' in cmd_data:
            for syntax_variant in cmd_data['syntax'].values():
                if isinstance(syntax_variant, dict) and 'api_name' in syntax_variant:
                    return syntax_variant['api_name']
        return None
    
    def _is_storable(self, cmd_data: Dict) -> bool:
        """Check if command is storable (AT&W compatible)"""
        if 'syntax' in cmd_data:
            for syntax_variant in cmd_data['syntax'].values():
                if isinstance(syntax_variant, dict) and syntax_variant.get('storable', False):
                    return True
        return False
    
    def _requires_reboot(self, cmd_data: Dict) -> bool:
        """Check if command requires reboot"""
        if 'syntax' in cmd_data:
            for syntax_variant in cmd_data['syntax'].values():
                if isinstance(syntax_variant, dict) and syntax_variant.get('requires_reboot', False):
                    return True
        return False
    
    def get_command_groups(self) -> List[str]:
        """Get list of command group names"""
        return list(self.command_groups.keys())
    
    def get_commands_in_group(self, group_name: str) -> List[str]:
        """Get list of command names in a group"""
        return self.command_groups.get(group_name, [])
    
    def get_command_info(self, cmd_name: str) -> Optional[Dict]:
        """Get detailed command information"""
        return self.commands.get(cmd_name)
    
    def get_command_syntax_variants(self, cmd_name: str) -> List[Tuple[str, Dict]]:
        """
        Get all syntax variants for a command
        
        Returns:
            List of (syntax_string, syntax_data) tuples
        """
        cmd_info = self.commands.get(cmd_name)
        if not cmd_info or 'syntax' not in cmd_info:
            return []
        
        variants = []
        for syntax_str, syntax_data in cmd_info['syntax'].items():
            if isinstance(syntax_data, dict):
                variants.append((syntax_str, syntax_data))
        
        return variants
    
    def get_api_mapping(self, cmd_name: str) -> Optional[str]:
        """Get UCX API function name for AT command"""
        cmd_info = self.commands.get(cmd_name)
        if cmd_info:
            return cmd_info.get('api_name')
        return None
    
    def get_parameter_info(self, cmd_name: str, param_name: str) -> Optional[Dict]:
        """Get parameter information"""
        cmd_info = self.commands.get(cmd_name)
        if not cmd_info or 'parameters' not in cmd_info:
            return None
        
        return cmd_info['parameters'].get(param_name)
    
    def get_parameter_default(self, cmd_name: str, param_name: str) -> Optional[str]:
        """
        Get default value for a parameter based on its type and description
        
        Returns default value as string, or None if no default applicable
        """
        param_info = self.get_parameter_info(cmd_name, param_name)
        if not param_info or not isinstance(param_info, dict):
            return None
        
        # Safely get type and desc, converting to string if needed
        param_type_raw = param_info.get('type')
        param_type = str(param_type_raw).lower() if param_type_raw else ''
        
        param_desc_raw = param_info.get('desc')
        if isinstance(param_desc_raw, list):
            param_desc = ' '.join(str(item) for item in param_desc_raw).lower()
        elif param_desc_raw:
            param_desc = str(param_desc_raw).lower()
        else:
            param_desc = ''
        
        # Special case: MAC address type - default to 000000000000 (factory reset value)
        if param_type == 'mac_addr' or 'mac address' in param_desc:
            if '000000000000' in param_desc or 'factory' in param_desc:
                return '000000000000'
        
        # For address parameters, check if description mentions a zero/null default
        if 'address' in param_name.lower():
            if '000000000000' in param_desc:
                return '000000000000'
            if param_desc and ('0' in param_desc or 'default' in param_desc):
                # Check if type suggests it's a MAC address
                if param_type == 'mac_addr':
                    return '000000000000'
        
        # Integer types default to 0 if mentioned in description
        if param_type == 'integer':
            if 'default' in param_desc and '0' in param_desc:
                return '0'
        
        return None
    
    def get_product_info(self) -> Dict:
        """Get product information"""
        if not self.yaml_data or 'product' not in self.yaml_data:
            return {}
        
        return self.yaml_data.get('product', {})
    
    def get_config_capabilities(self) -> Dict:
        """Get configuration capabilities from YAML"""
        if not self.yaml_data or 'config' not in self.yaml_data:
            return {}
        
        return self.yaml_data.get('config', {})
    
    def export_at_to_api_mapping(self) -> Dict[str, str]:
        """
        Export AT command to API name mapping
        
        Returns:
            Dictionary mapping AT commands to UCX API function names
        """
        mapping = {}
        for cmd_name, cmd_info in self.commands.items():
            api_name = cmd_info.get('api_name')
            if api_name:
                mapping[cmd_name] = api_name
        
        return mapping
    
    def search_commands(self, search_term: str) -> List[str]:
        """
        Search for commands by name, description, or API name
        
        Args:
            search_term: Search string (case-insensitive)
            
        Returns:
            List of matching command names
        """
        search_lower = search_term.lower()
        matches = []
        
        for cmd_name, cmd_info in self.commands.items():
            if search_lower in cmd_name.lower():
                matches.append(cmd_name)
            elif search_lower in cmd_info.get('brief', '').lower():
                matches.append(cmd_name)
            elif search_lower in cmd_info.get('desc', '').lower():
                matches.append(cmd_name)
            elif search_lower in str(cmd_info.get('api_name', '')).lower():
                matches.append(cmd_name)
        
        return matches


# Singleton instance
_parser_instance: Optional[YAMLCommandParser] = None


def get_yaml_parser() -> YAMLCommandParser:
    """Get singleton YAML parser instance"""
    global _parser_instance
    if _parser_instance is None:
        _parser_instance = YAMLCommandParser()
    return _parser_instance


if __name__ == '__main__':
    # Test the parser
    parser = YAMLCommandParser()
    
    # Test GitHub loading
    print("Testing GitHub load for NORA-W36 3.1.0...")
    if parser.load_from_github('NORA-W36', '3.1.0'):
        print(f"Loaded {len(parser.commands)} commands")
        print(f"Command groups: {parser.get_command_groups()}")
        
        # Test API mapping export
        api_mapping = parser.export_at_to_api_mapping()
        print(f"\nFound {len(api_mapping)} AT-to-API mappings")
        
        # Show some examples
        print("\nExample mappings:")
        for i, (at_cmd, api_name) in enumerate(list(api_mapping.items())[:5]):
            print(f"  {at_cmd} -> {api_name}")
        
        # Test search
        print("\nSearching for 'bluetooth' commands:")
        matches = parser.search_commands('bluetooth')
        for cmd in matches[:5]:
            print(f"  {cmd}")
