"""
Settings Manager for ucxclient GUI

Handles persistent storage of user preferences including:
- Last used product and version
- COM port, baud rate, flow control
- WiFi SSID and password
- Bluetooth remote BD address
"""

import os
os.environ['PYTHONDONTWRITEBYTECODE'] = '1'

import json
import os
from typing import Optional, Dict, Any
from pathlib import Path


class SettingsManager:
    """Manage application settings with JSON persistence"""
    
    def __init__(self, settings_file: str = None):
        """
        Initialize settings manager
        
        Args:
            settings_file: Path to settings file (default: user's home directory)
        """
        if settings_file is None:
            # Store in user's home directory
            home = Path.home()
            settings_dir = home / '.ucxclient'
            settings_dir.mkdir(exist_ok=True)
            settings_file = settings_dir / 'settings.json'
        
        self.settings_file = Path(settings_file)
        self.settings: Dict[str, Any] = self._load_settings()
    
    def _load_settings(self) -> Dict[str, Any]:
        """Load settings from file"""
        if not self.settings_file.exists():
            return self._get_default_settings()
        
        try:
            with open(self.settings_file, 'r') as f:
                settings = json.load(f)
                # Merge with defaults to ensure all keys exist
                defaults = self._get_default_settings()
                defaults.update(settings)
                return defaults
        except Exception as e:
            print(f"Warning: Could not load settings: {e}")
            return self._get_default_settings()
    
    def _get_default_settings(self) -> Dict[str, Any]:
        """Get default settings"""
        return {
            # Product configuration
            'product': {
                'source': 'github',  # 'github' or 'local'
                'name': '',  # e.g., 'NORA-W36'
                'version': '',  # e.g., '3.1.0'
                'local_path': ''  # Path to local YAML file
            },
            
            # Serial port configuration
            'serial': {
                'port': '',  # e.g., 'COM11'
                'baudrate': 115200,
                'flow_control': False
            },
            
            # WiFi settings
            'wifi': {
                'ssid': '',
                'password': '',
                'last_connected_ssid': ''
            },
            
            # Bluetooth settings
            'bluetooth': {
                'remote_bd_addr': ''  # Format: 'AA:BB:CC:DD:EE:FF'
            },
            
            # GUI preferences
            'gui': {
                'window_width': 800,
                'window_height': 600,
                'window_x': None,
                'window_y': None
            }
        }
    
    def save(self) -> bool:
        """
        Save current settings to file
        
        Returns:
            True if successful, False otherwise
        """
        try:
            # Ensure directory exists
            self.settings_file.parent.mkdir(parents=True, exist_ok=True)
            
            with open(self.settings_file, 'w') as f:
                json.dump(self.settings, f, indent=2)
            return True
        except Exception as e:
            print(f"Error saving settings: {e}")
            return False
    
    def get(self, key: str, default: Any = None) -> Any:
        """
        Get a setting value using dot notation
        
        Args:
            key: Setting key (e.g., 'serial.port' or 'wifi.ssid')
            default: Default value if key not found
            
        Returns:
            Setting value or default
        """
        keys = key.split('.')
        value = self.settings
        
        for k in keys:
            if isinstance(value, dict) and k in value:
                value = value[k]
            else:
                return default
        
        return value
    
    def set(self, key: str, value: Any) -> None:
        """
        Set a setting value using dot notation
        
        Args:
            key: Setting key (e.g., 'serial.port' or 'wifi.ssid')
            value: Value to set
        """
        keys = key.split('.')
        current = self.settings
        
        # Navigate to the parent dictionary
        for k in keys[:-1]:
            if k not in current or not isinstance(current[k], dict):
                current[k] = {}
            current = current[k]
        
        # Set the value
        current[keys[-1]] = value
    
    def get_product_config(self) -> Dict[str, str]:
        """Get product configuration"""
        return self.settings.get('product', {})
    
    def set_product_config(self, source: str, name: str = '', version: str = '', local_path: str = ''):
        """Set product configuration"""
        self.settings['product'] = {
            'source': source,
            'name': name,
            'version': version,
            'local_path': local_path
        }
    
    def get_serial_config(self) -> Dict[str, Any]:
        """Get serial port configuration"""
        return self.settings.get('serial', {})
    
    def set_serial_config(self, port: str = None, baudrate: int = None, flow_control: bool = None):
        """Set serial port configuration"""
        if port is not None:
            self.settings['serial']['port'] = port
        if baudrate is not None:
            self.settings['serial']['baudrate'] = baudrate
        if flow_control is not None:
            self.settings['serial']['flow_control'] = flow_control
    
    def get_wifi_config(self) -> Dict[str, str]:
        """Get WiFi configuration"""
        return self.settings.get('wifi', {})
    
    def set_wifi_config(self, ssid: str = None, password: str = None, last_connected_ssid: str = None):
        """Set WiFi configuration"""
        if ssid is not None:
            self.settings['wifi']['ssid'] = ssid
        if password is not None:
            self.settings['wifi']['password'] = password
        if last_connected_ssid is not None:
            self.settings['wifi']['last_connected_ssid'] = last_connected_ssid
    
    def get_bluetooth_config(self) -> Dict[str, str]:
        """Get Bluetooth configuration"""
        return self.settings.get('bluetooth', {})
    
    def set_bluetooth_config(self, remote_bd_addr: str = None):
        """Set Bluetooth configuration"""
        if remote_bd_addr is not None:
            self.settings['bluetooth']['remote_bd_addr'] = remote_bd_addr
    
    def get_gui_config(self) -> Dict[str, Any]:
        """Get GUI configuration"""
        return self.settings.get('gui', {})
    
    def set_gui_config(self, window_width: int = None, window_height: int = None, 
                       window_x: int = None, window_y: int = None):
        """Set GUI configuration"""
        if window_width is not None:
            self.settings['gui']['window_width'] = window_width
        if window_height is not None:
            self.settings['gui']['window_height'] = window_height
        if window_x is not None:
            self.settings['gui']['window_x'] = window_x
        if window_y is not None:
            self.settings['gui']['window_y'] = window_y
    
    def clear(self):
        """Clear all settings and reset to defaults"""
        self.settings = self._get_default_settings()
        self.save()
    
    def __repr__(self):
        """String representation"""
        return f"SettingsManager(file={self.settings_file})"


# Global singleton instance
_settings_manager: Optional[SettingsManager] = None

def get_settings_manager() -> SettingsManager:
    """Get global settings manager instance"""
    global _settings_manager
    if _settings_manager is None:
        _settings_manager = SettingsManager()
    return _settings_manager


if __name__ == '__main__':
    # Test the settings manager
    settings = SettingsManager('test_settings.json')
    
    # Set some values
    settings.set_product_config('github', 'NORA-W36', '3.1.0')
    settings.set_serial_config(port='COM11', baudrate=115200, flow_control=False)
    settings.set_wifi_config(ssid='MyNetwork', password='secret123')
    settings.set_bluetooth_config(remote_bd_addr='AA:BB:CC:DD:EE:FF')
    
    # Save
    settings.save()
    print(f"Settings saved to {settings.settings_file}")
    
    # Load fresh instance
    settings2 = SettingsManager('test_settings.json')
    print(f"Product: {settings2.get_product_config()}")
    print(f"Serial: {settings2.get_serial_config()}")
    print(f"WiFi: {settings2.get_wifi_config()}")
    print(f"Bluetooth: {settings2.get_bluetooth_config()}")
    
    # Cleanup
    import os
    os.remove('test_settings.json')
    print("Test complete!")
