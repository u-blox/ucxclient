"""
Dynamic Product GUI Builder

Creates a fully dynamic GUI based on loaded product YAML configuration.
Inspired by s-center classic interface with:
- Automatic tab generation per command group
- Read/Write forms for each command
- WiFi scan panel
- Bluetooth scan panel  
- Parameter validation and defaults
"""

import os
os.environ['PYTHONDONTWRITEBYTECODE'] = '1'

import tkinter as tk
from tkinter import ttk, messagebox, scrolledtext
from typing import Optional, Dict, List, Tuple
import threading
from datetime import datetime


class DynamicProductGUI:
    """
    Dynamic GUI builder that creates interface based on YAML product configuration
    """
    
    def __init__(self, parent_notebook: ttk.Notebook, yaml_parser, api_executor, log_callback, main_window=None, theme_colors=None):
        """
        Initialize dynamic GUI builder
        
        Args:
            parent_notebook: Parent notebook widget to add tabs to
            yaml_parser: YAMLCommandParser instance with loaded product
            api_executor: UCXAPIExecutor instance for command execution
            log_callback: Function to call for logging messages
            main_window: Reference to MainWindow instance for connection status checks
            theme_colors: Dictionary of theme colors for dark mode
        """
        self.parent_notebook = parent_notebook
        self.yaml_parser = yaml_parser
        self.api_executor = api_executor
        self.log = log_callback
        self.main_window = main_window
        self.theme_colors = theme_colors or {}
        
        # Get settings manager from main_window
        self.settings = main_window.settings if main_window else None
        
        # Track created widgets for cleanup
        self.group_frames: Dict[str, ttk.Frame] = {}
        self.command_widgets: Dict[str, Dict] = {}
        
        # Special feature panels
        self.wifi_scan_panel: Optional[ttk.Frame] = None
        self.bluetooth_scan_panel: Optional[ttk.Frame] = None
        
    def build_gui(self):
        """Build complete dynamic GUI from loaded product configuration"""
        if not self.yaml_parser or not self.yaml_parser.commands:
            self.log("ERROR: No product configuration loaded")
            return
        
        self.log(f"Building dynamic GUI for {self.yaml_parser.product} v{self.yaml_parser.version}...")
        
        # Clear any existing dynamic tabs
        self._clear_dynamic_tabs()
        
        # Get command groups
        groups = self.yaml_parser.get_command_groups()
        self.log(f"Found {len(groups)} command groups: {', '.join(groups)}")
        
        # Define group categories for organization
        group_categories = {
            'WiFi': ['Wi-Fi', 'WiFi Station', 'WiFi Access Point', 'WiFi', 'Station', 'Access Point'],
            'Bluetooth': ['Bluetooth', 'GATT client', 'GATT Server', 'SPS'],
            'Network': ['Socket', 'MQTT', 'HTTP'],
            'System': ['General', 'System', 'Device Information', 'Diagnostics'],
            'Security': ['Security', 'Network Time', 'Transparency', 'Power']
        }
        
        # Track which groups have been added
        processed_groups = set()
        tabs_created = 0
        
        # Create WiFi category tab with organized sub-tabs
        wifi_groups = [g for g in groups if g in group_categories['WiFi']]
        if wifi_groups:
            self.log(f"Creating WiFi tab with {len(wifi_groups)} groups: {', '.join(wifi_groups)}")
            self._create_wifi_organized_tab(wifi_groups)
            processed_groups.update(wifi_groups)
            tabs_created += 1
        
        # Create Bluetooth category tab with sub-tabs
        bluetooth_groups = [g for g in groups if g in group_categories['Bluetooth']]
        if bluetooth_groups:
            self.log(f"Creating Bluetooth tab with {len(bluetooth_groups)} groups: {', '.join(bluetooth_groups)}")
            self._create_category_tab('Bluetooth', bluetooth_groups, has_special_scan='Bluetooth' in bluetooth_groups)
            processed_groups.update(bluetooth_groups)
            tabs_created += 1
        
        # Create Network category tab if any network groups exist
        network_groups = [g for g in groups if g in group_categories['Network']]
        if network_groups:
            self.log(f"Creating Network tab with {len(network_groups)} groups: {', '.join(network_groups)}")
            self._create_category_tab('Network', network_groups)
            processed_groups.update(network_groups)
            tabs_created += 1
        
        # Create System category tab
        system_groups = [g for g in groups if g in group_categories['System']]
        if system_groups:
            self.log(f"Creating System tab with {len(system_groups)} groups: {', '.join(system_groups)}")
            self._create_category_tab('System', system_groups)
            processed_groups.update(system_groups)
            tabs_created += 1
        
        # Create Security category tab
        security_groups = [g for g in groups if g in group_categories['Security']]
        if security_groups:
            self.log(f"Creating Security tab with {len(security_groups)} groups: {', '.join(security_groups)}")
            self._create_category_tab('Security', security_groups)
            processed_groups.update(security_groups)
            tabs_created += 1
        
        # Create tabs for any remaining uncategorized groups
        remaining_groups = [g for g in groups if g not in processed_groups]
        for group_name in remaining_groups:
            self._create_group_tab(group_name)
            tabs_created += 1
        
        self.log(f"✓ Created {tabs_created} tabs with {len(self.yaml_parser.commands)} commands")
    
    def _clear_dynamic_tabs(self):
        """Clear all dynamically created tabs"""
        # Remove all tabs except the first static one (Log tab)
        num_tabs = self.parent_notebook.index("end")
        for i in range(num_tabs - 1, 0, -1):  # Keep only first tab (Log at index 0)
            self.parent_notebook.forget(i)
        
        self.group_frames.clear()
        self.command_widgets.clear()
    
    def _create_category_tab(self, category_name: str, group_names: List[str], has_special_scan: bool = False):
        """Create a category tab with sub-tabs for each group
        
        Args:
            category_name: Name of the category (e.g., 'Bluetooth', 'Network', 'WiFi')
            group_names: List of group names to include as sub-tabs
            has_special_scan: If True, create special scan interface for first matching group
        """
        # Emoji icons for categories
        icons = {
            'WiFi': '📶',
            'WiFi Station': '📶',
            'WiFi AP': '📡',
            'WiFi Security': '🔐',
            'Bluetooth': '🔵',
            'Network': '🌐',
            'System': '⚙️',
            'Security': '🔒'
        }
        icon = icons.get(category_name, '📁')
        
        # Create main category frame
        category_frame = ttk.Frame(self.parent_notebook)
        self.parent_notebook.add(category_frame, text=f"{icon} {category_name}")
        
        # Create sub-notebook for groups within this category
        sub_notebook = ttk.Notebook(category_frame)
        sub_notebook.pack(fill=tk.BOTH, expand=True)
        
        # Add special scan tab first if applicable
        if has_special_scan:
            if category_name == 'Bluetooth' and 'Bluetooth' in group_names:
                self._create_bluetooth_tab_in_notebook(sub_notebook)
            elif category_name == 'WiFi' and any(g in group_names for g in ['Wi-Fi', 'WiFi']):
                self._create_wifi_tab_in_notebook(sub_notebook)
        
        # Add sub-tabs for each group (all AT commands)
        for group_name in group_names:
            # Create regular command group tab for all groups
            self._create_group_tab_in_notebook(sub_notebook, group_name)
    
    def _create_wifi_organized_tab(self, wifi_groups: List[str]):
        """Create organized WiFi tab with sub-tabs for Scan/Connect, Station, AP, and Enterprise
        
        Args:
            wifi_groups: List of WiFi-related group names (typically just one: 'Station')
        """
        # Create main WiFi frame
        wifi_frame = ttk.Frame(self.parent_notebook)
        self.parent_notebook.add(wifi_frame, text="📶 Wi-Fi")
        
        # Create sub-notebook for WiFi organization
        wifi_notebook = ttk.Notebook(wifi_frame)
        wifi_notebook.pack(fill=tk.BOTH, expand=True)
        
        # 1. WiFi Scan and Connect tab (special interface) - always first
        self._create_wifi_tab_in_notebook(wifi_notebook)
        
        # Get all WiFi commands from all groups
        all_wifi_commands = []
        for group_name in wifi_groups:
            cmd_names = self.yaml_parser.get_commands_in_group(group_name)
            if cmd_names:
                all_wifi_commands.extend(cmd_names)
        
        # Categorize commands by prefix
        station_commands = []
        ap_commands = []
        enterprise_commands = []
        
        for cmd_name in all_wifi_commands:
            # Check AT command name and API name
            cmd_info = self.yaml_parser.get_command_info(cmd_name)
            if not cmd_info:
                station_commands.append(cmd_name)
                continue
                
            api_name = (cmd_info.get('api_name') or '').lower()
            cmd_lower = cmd_name.lower()
            
            # Enterprise security commands (check api_name for "enterprise" or "peap")
            if 'enterprise' in api_name or 'peap' in api_name:
                enterprise_commands.append(cmd_name)
            # Access Point commands (AT+UWAP...)
            elif 'uwap' in cmd_lower:
                ap_commands.append(cmd_name)
            # Station commands (AT+UWS..., AT+UWHN, etc.) - everything else
            else:
                station_commands.append(cmd_name)
        
        self.log(f"WiFi commands: {len(station_commands)} station, {len(ap_commands)} AP, {len(enterprise_commands)} enterprise")
        
        # 2. Station commands tab
        if station_commands:
            station_frame = ttk.Frame(wifi_notebook)
            wifi_notebook.add(station_frame, text="� Station")
            
            # Create canvas with scrollbar
            canvas = tk.Canvas(station_frame, bg=self.theme_colors.get('bg', '#1e1e1e'), 
                             highlightthickness=0)
            scrollbar = ttk.Scrollbar(station_frame, orient=tk.VERTICAL, command=canvas.yview)
            scrollable_frame = ttk.Frame(canvas)
            
            scrollable_frame.bind("<Configure>", lambda e, c=canvas: c.configure(scrollregion=c.bbox("all")))
            canvas.create_window((0, 0), window=scrollable_frame, anchor=tk.NW)
            canvas.configure(yscrollcommand=scrollbar.set)
            
            canvas.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, padx=5, pady=5)
            scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
            
            # Add station commands
            for cmd_name in station_commands:
                self._create_command_widget(scrollable_frame, cmd_name)
        
        # 3. Access Point commands tab
        if ap_commands:
            ap_frame = ttk.Frame(wifi_notebook)
            wifi_notebook.add(ap_frame, text="📡 Access Point")
            
            # Create canvas with scrollbar
            canvas = tk.Canvas(ap_frame, bg=self.theme_colors.get('bg', '#1e1e1e'), 
                             highlightthickness=0)
            scrollbar = ttk.Scrollbar(ap_frame, orient=tk.VERTICAL, command=canvas.yview)
            scrollable_frame = ttk.Frame(canvas)
            
            scrollable_frame.bind("<Configure>", lambda e, c=canvas: c.configure(scrollregion=c.bbox("all")))
            canvas.create_window((0, 0), window=scrollable_frame, anchor=tk.NW)
            canvas.configure(yscrollcommand=scrollbar.set)
            
            canvas.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, padx=5, pady=5)
            scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
            
            # Add AP commands
            for cmd_name in ap_commands:
                self._create_command_widget(scrollable_frame, cmd_name)
        
        # 4. Enterprise Security tab
        if enterprise_commands:
            enterprise_frame = ttk.Frame(wifi_notebook)
            wifi_notebook.add(enterprise_frame, text="🔐 Enterprise Security")
            
            # Create canvas with scrollbar
            canvas = tk.Canvas(enterprise_frame, bg=self.theme_colors.get('bg', '#1e1e1e'), 
                             highlightthickness=0)
            scrollbar = ttk.Scrollbar(enterprise_frame, orient=tk.VERTICAL, command=canvas.yview)
            scrollable_frame = ttk.Frame(canvas)
            
            scrollable_frame.bind("<Configure>", lambda e: canvas.configure(scrollregion=canvas.bbox("all")))
            canvas.create_window((0, 0), window=scrollable_frame, anchor=tk.NW)
            canvas.configure(yscrollcommand=scrollbar.set)
            
            canvas.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, padx=5, pady=5)
            scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
            
            # Add enterprise security commands
            for cmd_name in enterprise_commands:
                self._create_command_widget(scrollable_frame, cmd_name)
    
    def _create_wifi_tab(self):
        """Create dedicated WiFi control tab with scan and connect features in parent notebook"""
        self._create_wifi_tab_in_notebook(self.parent_notebook)
    
    def _create_wifi_tab_in_notebook(self, notebook: ttk.Notebook):
        """Create dedicated WiFi control tab with scan and connect features"""
        wifi_frame = ttk.Frame(notebook)
        notebook.add(wifi_frame, text="📶 WiFi Scan and Connect")
        
        # Create paned window for scan results and controls
        paned = ttk.PanedWindow(wifi_frame, orient=tk.HORIZONTAL)
        paned.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        # Left: Scan results
        left_frame = ttk.LabelFrame(paned, text="Available Networks", padding=10)
        paned.add(left_frame, weight=3)
        
        # Scan button at the top
        scan_btn_frame = ttk.Frame(left_frame)
        scan_btn_frame.pack(fill=tk.X, pady=(0, 5))
        
        ttk.Button(scan_btn_frame, text="🔍 Scan Networks", 
                  command=self._wifi_scan).pack(side=tk.LEFT, padx=5)
        
        self.wifi_scan_status = ttk.Label(scan_btn_frame, text="Ready")
        self.wifi_scan_status.pack(side=tk.LEFT, padx=10)
        
        # WiFi scan tree
        tree_frame = ttk.Frame(left_frame)
        tree_frame.pack(fill=tk.BOTH, expand=True)
        
        columns = ('SSID', 'BSSID', 'Channel', 'RSSI', 'Security')
        self.wifi_tree = ttk.Treeview(tree_frame, columns=columns, show='headings', height=15)
        
        for col in columns:
            self.wifi_tree.heading(col, text=col)
            self.wifi_tree.column(col, width=100)
        
        # Scrollbars
        vsb = ttk.Scrollbar(tree_frame, orient=tk.VERTICAL, command=self.wifi_tree.yview)
        self.wifi_tree.configure(yscrollcommand=vsb.set)
        
        self.wifi_tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        vsb.pack(side=tk.RIGHT, fill=tk.Y)
        
        # Right: Connection controls
        right_frame = ttk.LabelFrame(paned, text="Network Connection", padding=10)
        paned.add(right_frame, weight=2)
        
        # Load saved WiFi settings
        saved_wifi = self.settings.get_wifi_config() if self.settings else {}
        
        # SSID
        ttk.Label(right_frame, text="SSID:").grid(row=0, column=0, sticky=tk.W, pady=5)
        self.wifi_ssid_var = tk.StringVar(value=saved_wifi.get('ssid', ''))
        self.wifi_ssid_entry = ttk.Entry(right_frame, textvariable=self.wifi_ssid_var, width=30)
        self.wifi_ssid_entry.grid(row=0, column=1, pady=5, padx=5, sticky=tk.EW)
        
        # Password
        ttk.Label(right_frame, text="Password:").grid(row=1, column=0, sticky=tk.W, pady=5)
        self.wifi_pass_var = tk.StringVar(value=saved_wifi.get('password', ''))
        self.wifi_pass_entry = ttk.Entry(right_frame, textvariable=self.wifi_pass_var, 
                                        width=30, show="●")
        self.wifi_pass_entry.grid(row=1, column=1, pady=5, padx=5, sticky=tk.EW)
        
        # Show password checkbox
        self.wifi_show_pass = tk.BooleanVar()
        ttk.Checkbutton(right_frame, text="Show password", 
                       variable=self.wifi_show_pass,
                       command=self._toggle_wifi_password).grid(row=2, column=1, sticky=tk.W, pady=2)
        
        # Security type
        ttk.Label(right_frame, text="Security:").grid(row=3, column=0, sticky=tk.W, pady=5)
        self.wifi_security_var = tk.StringVar(value="WPA2")
        security_combo = ttk.Combobox(right_frame, textvariable=self.wifi_security_var,
                                     values=["Open", "WEP", "WPA", "WPA2", "WPA3"], 
                                     state="readonly", width=27)
        security_combo.grid(row=3, column=1, pady=5, padx=5, sticky=tk.EW)
        
        # Buttons
        btn_frame = ttk.Frame(right_frame)
        btn_frame.grid(row=4, column=0, columnspan=2, pady=15)
        
        ttk.Button(btn_frame, text="🔗 Connect", 
                  command=self._wifi_connect).pack(side=tk.LEFT, padx=5)
        ttk.Button(btn_frame, text="🔌 Disconnect", 
                  command=self._wifi_disconnect).pack(side=tk.LEFT, padx=5)
        
        # Status display
        status_frame = ttk.LabelFrame(right_frame, text="Connection Status", padding=10)
        status_frame.grid(row=5, column=0, columnspan=2, sticky=tk.EW, pady=10)
        
        self.wifi_status_text = scrolledtext.ScrolledText(status_frame, height=8, width=40,
                                                          wrap=tk.WORD, state=tk.DISABLED,
                                                          bg=self.theme_colors.get('bg_darker', '#252526'),
                                                          fg=self.theme_colors.get('fg_light', '#e0e0e0'),
                                                          insertbackground=self.theme_colors.get('fg_bright', '#ffffff'),
                                                          selectbackground=self.theme_colors.get('accent', '#007acc'),
                                                          selectforeground=self.theme_colors.get('fg_bright', '#ffffff'),
                                                          borderwidth=0, highlightthickness=1,
                                                          highlightbackground=self.theme_colors.get('border', '#3e3e42'))
        self.wifi_status_text.pack(fill=tk.BOTH, expand=True)
        
        # Bind selection to auto-fill SSID
        self.wifi_tree.bind('<<TreeviewSelect>>', self._on_wifi_network_select)
        
        right_frame.columnconfigure(1, weight=1)
        
        self.wifi_scan_panel = wifi_frame
    
    def _create_bluetooth_tab(self):
        """Create dedicated Bluetooth control tab with scan and connect features in parent notebook"""
        self._create_bluetooth_tab_in_notebook(self.parent_notebook)
    
    def _create_bluetooth_tab_in_notebook(self, notebook: ttk.Notebook):
        """Create dedicated Bluetooth control tab with scan and connect features"""
        bt_frame = ttk.Frame(notebook)
        notebook.add(bt_frame, text="🔵 Bluetooth Scan and Connect")
        
        # Create paned window
        paned = ttk.PanedWindow(bt_frame, orient=tk.HORIZONTAL)
        paned.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        # Left: Scan results
        left_frame = ttk.LabelFrame(paned, text="Discovered Devices", padding=10)
        paned.add(left_frame, weight=3)
        
        # Scan button at top
        scan_btn_frame = ttk.Frame(left_frame)
        scan_btn_frame.pack(fill=tk.X, pady=(0, 5))
        
        ttk.Button(scan_btn_frame, text="🔍 Scan Devices", 
                  command=self._bluetooth_scan).pack(side=tk.LEFT, padx=5)
        
        self.bt_scan_status = ttk.Label(scan_btn_frame, text="Ready")
        self.bt_scan_status.pack(side=tk.LEFT, padx=10)
        
        # Bluetooth device tree
        tree_frame = ttk.Frame(left_frame)
        tree_frame.pack(fill=tk.BOTH, expand=True)
        
        columns = ('Name', 'Address', 'RSSI', 'Type')
        self.bt_tree = ttk.Treeview(tree_frame, columns=columns, show='headings', height=15)
        
        for col in columns:
            self.bt_tree.heading(col, text=col)
            self.bt_tree.column(col, width=120)
        
        # Scrollbars
        vsb = ttk.Scrollbar(tree_frame, orient=tk.VERTICAL, command=self.bt_tree.yview)
        self.bt_tree.configure(yscrollcommand=vsb.set)
        
        self.bt_tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        vsb.pack(side=tk.RIGHT, fill=tk.Y)
        
        # Right: Connection controls
        right_frame = ttk.LabelFrame(paned, text="Device Connection", padding=10)
        paned.add(right_frame, weight=2)
        
        # Load saved Bluetooth settings
        saved_bt = self.settings.get_bluetooth_config() if self.settings else {}
        
        # Device address
        ttk.Label(right_frame, text="Device Address:").grid(row=0, column=0, sticky=tk.W, pady=5)
        self.bt_addr_var = tk.StringVar(value=saved_bt.get('remote_bd_addr', ''))
        self.bt_addr_entry = ttk.Entry(right_frame, textvariable=self.bt_addr_var, width=30)
        self.bt_addr_entry.grid(row=0, column=1, pady=5, padx=5, sticky=tk.EW)
        
        # Connection buttons
        btn_frame = ttk.Frame(right_frame)
        btn_frame.grid(row=1, column=0, columnspan=2, pady=15)
        
        ttk.Button(btn_frame, text="🔗 Connect", 
                  command=self._bluetooth_connect).pack(side=tk.LEFT, padx=5)
        ttk.Button(btn_frame, text="🔌 Disconnect", 
                  command=self._bluetooth_disconnect).pack(side=tk.LEFT, padx=5)
        
        # Status display
        status_frame = ttk.LabelFrame(right_frame, text="Connection Status", padding=10)
        status_frame.grid(row=2, column=0, columnspan=2, sticky=tk.EW, pady=10)
        
        self.bt_status_text = scrolledtext.ScrolledText(status_frame, height=10, width=40,
                                                        wrap=tk.WORD, state=tk.DISABLED,
                                                        bg=self.theme_colors.get('bg_darker', '#252526'),
                                                        fg=self.theme_colors.get('fg_light', '#e0e0e0'),
                                                        insertbackground=self.theme_colors.get('fg_bright', '#ffffff'),
                                                        selectbackground=self.theme_colors.get('accent', '#007acc'),
                                                        selectforeground=self.theme_colors.get('fg_bright', '#ffffff'),
                                                        borderwidth=0, highlightthickness=1,
                                                        highlightbackground=self.theme_colors.get('border', '#3e3e42'))
        self.bt_status_text.pack(fill=tk.BOTH, expand=True)
        
        # Bind selection
        self.bt_tree.bind('<<TreeviewSelect>>', self._on_bluetooth_device_select)
        
        right_frame.columnconfigure(1, weight=1)
        
        self.bluetooth_scan_panel = bt_frame
    
    def _create_group_tab(self, group_name: str):
        """Create a tab for a command group in parent notebook"""
        self._create_group_tab_in_notebook(self.parent_notebook, group_name)
    
    def _create_group_tab_in_notebook(self, notebook: ttk.Notebook, group_name: str):
        """
        Create a tab for a command group with read/write forms for each command
        
        Args:
            notebook: The notebook widget to add the tab to
            group_name: Name of the command group (e.g., "System", "Wi-Fi")
        """
        # Skip WiFi and Bluetooth - they have dedicated tabs (when in parent notebook)
        if notebook == self.parent_notebook and group_name in ['Wi-Fi', 'Bluetooth']:
            return
        
        # Create tab
        group_frame = ttk.Frame(notebook)
        notebook.add(group_frame, text=group_name)
        self.group_frames[group_name] = group_frame
        
        # Create scrollable canvas
        canvas = tk.Canvas(group_frame)
        scrollbar = ttk.Scrollbar(group_frame, orient=tk.VERTICAL, command=canvas.yview)
        scrollable_frame = ttk.Frame(canvas)
        
        scrollable_frame.bind(
            "<Configure>",
            lambda e: canvas.configure(scrollregion=canvas.bbox("all"))
        )
        
        canvas.create_window((0, 0), window=scrollable_frame, anchor=tk.NW)
        canvas.configure(yscrollcommand=scrollbar.set)
        
        canvas.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        
        # Get commands in this group
        commands = self.yaml_parser.get_commands_in_group(group_name)
        
        # Create a labeled frame for each command
        for cmd_name in commands:
            self._create_command_widget(scrollable_frame, cmd_name)
    
    def _create_command_widget(self, parent, cmd_name: str):
        """
        Create a widget for a single command with read/write capabilities
        
        Args:
            parent: Parent widget
            cmd_name: AT command name (e.g., "AT+USYLA")
        """
        cmd_info = self.yaml_parser.get_command_info(cmd_name)
        if not cmd_info:
            return
        
        # Create labeled frame for this command
        brief = cmd_info.get('brief', cmd_name)
        cmd_frame = ttk.LabelFrame(parent, text=f"{cmd_name} - {brief}", padding=10)
        cmd_frame.pack(fill=tk.X, padx=10, pady=5)
        
        # Description
        desc = cmd_info.get('desc', '')
        if desc:
            desc_label = ttk.Label(cmd_frame, text=desc, wraplength=600, 
                                  foreground='gray', font=('TkDefaultFont', 9))
            desc_label.pack(anchor=tk.W, pady=(0, 5))
        
        # Get syntax variants
        variants = self.yaml_parser.get_command_syntax_variants(cmd_name)
        
        # Determine if command has read and/or write operations
        # Read: syntax with '?' OR syntax type='Read'
        # Write: syntax with '=' and type='Write' or not type='Read'
        has_read = False
        has_write = False
        
        for syntax_str, syntax_data in variants:
            if isinstance(syntax_data, dict):
                syntax_type = syntax_data.get('type', '')
                
                # Check for read operation
                if '?' in syntax_str or syntax_type == 'Read':
                    has_read = True
                
                # Check for write operation
                if '=' in syntax_str and syntax_type != 'Read':
                    has_write = True
        
        # Create notebook for read/write tabs if both exist
        if has_read and has_write:
            sub_notebook = ttk.Notebook(cmd_frame)
            sub_notebook.pack(fill=tk.BOTH, expand=True)
            
            read_frame = ttk.Frame(sub_notebook, padding=5)
            write_frame = ttk.Frame(sub_notebook, padding=5)
            
            sub_notebook.add(read_frame, text="Read")
            sub_notebook.add(write_frame, text="Write")
            
            if has_read:
                self._create_read_form(read_frame, cmd_name, cmd_info)
            if has_write:
                self._create_write_form(write_frame, cmd_name, cmd_info)
        else:
            # Single operation
            if has_read:
                self._create_read_form(cmd_frame, cmd_name, cmd_info)
            elif has_write:
                self._create_write_form(cmd_frame, cmd_name, cmd_info)
            else:
                # Action command (no parameters)
                self._create_action_form(cmd_frame, cmd_name, cmd_info)
    
    def _create_read_form(self, parent, cmd_name: str, cmd_info: Dict):
        """Create form for reading command values"""
        # For Read operations, offer parameter inputs if command has parameters
        # This allows parameterized reads like AT+UWAPNST=<status_id>
        syntax = cmd_info.get('syntax', {})
        all_params = cmd_info.get('parameters', {})
        
        # Extract parameters from syntax strings (e.g., <status_id> from "AT+UWAPNST=<status_id>")
        syntax_params = set()
        for syntax_str in syntax.keys():
            # Find all <param_name> patterns in syntax
            import re
            params_in_syntax = re.findall(r'<(\w+)>', syntax_str)
            syntax_params.update(params_in_syntax)
        
        # Build combined parameter dict from both sources
        read_params = {}
        
        # Add parameters from parameters section
        if all_params:
            read_params.update(all_params)
        
        # Add parameters found in syntax but not in parameters section
        for param_name in syntax_params:
            if param_name not in read_params:
                # Create a basic parameter definition
                read_params[param_name] = {
                    'type': 'string',
                    'desc': f'Parameter from syntax: {param_name}'
                }
        
        # Show parameter inputs if we have any parameters
        show_params = bool(read_params and any('=' in s for s in syntax.keys()))
        
        # Create parameter input fields if needed
        param_vars = {}
        if show_params:
            param_frame = ttk.LabelFrame(parent, text="Read Parameters (optional - leave empty for simple read)", padding=5)
            param_frame.pack(fill=tk.X, pady=5)
            
            row = 0
            for param_name, param_info in read_params.items():
                # Label
                ttk.Label(param_frame, text=f"{param_name}:").grid(row=row, column=0, 
                                                                   sticky=tk.W, pady=3, padx=5)
                
                # Get parameter description
                param_desc_raw = param_info.get('desc')
                if isinstance(param_desc_raw, list):
                    param_desc = ' '.join(str(item) for item in param_desc_raw)
                else:
                    param_desc = str(param_desc_raw) if param_desc_raw else ''
                
                # Input field
                var = tk.StringVar()
                entry = ttk.Entry(param_frame, textvariable=var, width=30)
                entry.grid(row=row, column=1, sticky=tk.EW, pady=3, padx=5)
                param_vars[param_name] = var
                
                # Description/hint
                if param_desc:
                    hint = ttk.Label(param_frame, text=param_desc[:80], 
                                    font=("Segoe UI", 8), 
                                    foreground=self.theme_colors.get('fg_dim', '#808080'))
                    hint.grid(row=row, column=2, sticky=tk.W, pady=3, padx=5)
                
                row += 1
            
            param_frame.columnconfigure(1, weight=1)
        
        # Read button
        btn_frame = ttk.Frame(parent)
        btn_frame.pack(fill=tk.X, pady=5)
        
        ttk.Button(btn_frame, text=f"📖 Read {cmd_name}", 
                  command=lambda: self._execute_read(cmd_name, param_vars if show_params else None)).pack(side=tk.LEFT)
        
        # Result display
        result_frame = ttk.LabelFrame(parent, text="Response", padding=5)
        result_frame.pack(fill=tk.BOTH, expand=True, pady=5)
        
        result_text = scrolledtext.ScrolledText(result_frame, height=4, width=60,
                                               wrap=tk.WORD, state=tk.DISABLED,
                                               bg=self.theme_colors.get('bg_darker', '#252526'),
                                               fg=self.theme_colors.get('fg_light', '#e0e0e0'),
                                               insertbackground=self.theme_colors.get('fg_bright', '#ffffff'),
                                               selectbackground=self.theme_colors.get('accent', '#007acc'),
                                               selectforeground=self.theme_colors.get('fg_bright', '#ffffff'),
                                               borderwidth=0, highlightthickness=1,
                                               highlightbackground=self.theme_colors.get('border', '#3e3e42'))
        result_text.pack(fill=tk.BOTH, expand=True)
        
        # Store widget references
        if cmd_name not in self.command_widgets:
            self.command_widgets[cmd_name] = {}
        self.command_widgets[cmd_name]['read_result'] = result_text
        if show_params:
            self.command_widgets[cmd_name]['read_params'] = param_vars
    
    def _create_write_form(self, parent, cmd_name: str, cmd_info: Dict):
        """Create form for writing command values with parameter inputs"""
        params = cmd_info.get('parameters', {})
        
        # Create parameter input fields
        param_vars = {}
        param_frame = ttk.Frame(parent)
        param_frame.pack(fill=tk.X, pady=5)
        
        row = 0
        for param_name, param_info in params.items():
            # Label
            ttk.Label(param_frame, text=f"{param_name}:").grid(row=row, column=0, 
                                                               sticky=tk.W, pady=3, padx=5)
            
            # Get parameter type and default (safely handle None and lists)
            param_type = str(param_info.get('type') or 'string')
            param_desc_raw = param_info.get('desc')
            if isinstance(param_desc_raw, list):
                param_desc = ' '.join(str(item) for item in param_desc_raw)
            else:
                param_desc = str(param_desc_raw) if param_desc_raw else ''
            default_value = self.yaml_parser.get_parameter_default(cmd_name, param_name) or ''
            
            # Input widget based on type
            if param_type == 'integer' or 'integer' in param_type:
                var = tk.StringVar(value=default_value)
                entry = ttk.Entry(param_frame, textvariable=var, width=30)
                entry.grid(row=row, column=1, pady=3, padx=5, sticky=tk.EW)
            elif 'valid_values' in param_info:
                # Combobox for enumerated values
                var = tk.StringVar(value=default_value)
                values = param_info['valid_values'].split('..')
                if len(values) == 1:
                    # List of specific values
                    combo = ttk.Combobox(param_frame, textvariable=var, 
                                        values=values[0].split(','), width=27)
                else:
                    # Range - use entry
                    combo = ttk.Entry(param_frame, textvariable=var, width=30)
                combo.grid(row=row, column=1, pady=3, padx=5, sticky=tk.EW)
            else:
                # Text entry
                var = tk.StringVar(value=default_value)
                entry = ttk.Entry(param_frame, textvariable=var, width=30)
                entry.grid(row=row, column=1, pady=3, padx=5, sticky=tk.EW)
            
            # Description tooltip
            if param_desc:
                desc_label = ttk.Label(param_frame, text=param_desc[:80] + "..." if len(param_desc) > 80 else param_desc,
                                      foreground='gray', font=('TkDefaultFont', 8))
                desc_label.grid(row=row, column=2, sticky=tk.W, padx=5)
            
            param_vars[param_name] = var
            row += 1
        
        param_frame.columnconfigure(1, weight=1)
        
        # Write button
        btn_frame = ttk.Frame(parent)
        btn_frame.pack(fill=tk.X, pady=5)
        
        ttk.Button(btn_frame, text=f"✍️ Write {cmd_name}", 
                  command=lambda: self._execute_write(cmd_name, param_vars)).pack(side=tk.LEFT)
        
        # Result display
        result_frame = ttk.LabelFrame(parent, text="Response", padding=5)
        result_frame.pack(fill=tk.BOTH, expand=True, pady=5)
        
        result_text = scrolledtext.ScrolledText(result_frame, height=3, width=60,
                                               wrap=tk.WORD, state=tk.DISABLED,
                                               bg=self.theme_colors.get('bg_darker', '#252526'),
                                               fg=self.theme_colors.get('fg_light', '#e0e0e0'),
                                               insertbackground=self.theme_colors.get('fg_bright', '#ffffff'),
                                               selectbackground=self.theme_colors.get('accent', '#007acc'),
                                               selectforeground=self.theme_colors.get('fg_bright', '#ffffff'),
                                               borderwidth=0, highlightthickness=1,
                                               highlightbackground=self.theme_colors.get('border', '#3e3e42'))
        result_text.pack(fill=tk.BOTH, expand=True)
        
        # Store widget references
        if cmd_name not in self.command_widgets:
            self.command_widgets[cmd_name] = {}
        self.command_widgets[cmd_name]['write_params'] = param_vars
        self.command_widgets[cmd_name]['write_result'] = result_text
    
    def _create_action_form(self, parent, cmd_name: str, cmd_info: Dict):
        """Create form for action commands (no parameters)"""
        btn_frame = ttk.Frame(parent)
        btn_frame.pack(fill=tk.X, pady=5)
        
        ttk.Button(btn_frame, text=f"▶️ Execute {cmd_name}", 
                  command=lambda: self._execute_action(cmd_name)).pack(side=tk.LEFT)
        
        # Result display
        result_frame = ttk.LabelFrame(parent, text="Response", padding=5)
        result_frame.pack(fill=tk.BOTH, expand=True, pady=5)
        
        result_text = scrolledtext.ScrolledText(result_frame, height=3, width=60,
                                               wrap=tk.WORD, state=tk.DISABLED,
                                               bg=self.theme_colors.get('bg_darker', '#252526'),
                                               fg=self.theme_colors.get('fg_light', '#e0e0e0'),
                                               insertbackground=self.theme_colors.get('fg_bright', '#ffffff'),
                                               selectbackground=self.theme_colors.get('accent', '#007acc'),
                                               selectforeground=self.theme_colors.get('fg_bright', '#ffffff'),
                                               borderwidth=0, highlightthickness=1,
                                               highlightbackground=self.theme_colors.get('border', '#3e3e42'))
        result_text.pack(fill=tk.BOTH, expand=True)
        
        # Store widget reference
        if cmd_name not in self.command_widgets:
            self.command_widgets[cmd_name] = {}
        self.command_widgets[cmd_name]['action_result'] = result_text
    
    # Command execution methods
    
    def _execute_read(self, cmd_name: str, param_vars: Optional[Dict[str, tk.StringVar]] = None):
        """Execute read command with optional parameters"""
        # Check if connected to device first
        if not self.main_window.is_connected():
            messagebox.showwarning("Not Connected", 
                                 "Please connect to the device first",
                                 parent=self.parent)
            return
        
        self.log(f"Reading {cmd_name}...")
        
        # Build read command
        if param_vars:
            # Collect parameter values (only non-empty ones)
            params = []
            for param_name, var in param_vars.items():
                value = var.get().strip()
                if value:
                    params.append(value)
            
            # If any parameters provided, build command with them
            if params:
                # Build command with parameters: AT+CMD=param1,param2,...
                read_cmd = f"{cmd_name}={','.join(params)}"
            else:
                # No parameters provided, use simple read
                read_cmd = f"{cmd_name}?"
        else:
            # Simple read with ?
            read_cmd = f"{cmd_name}?"
        
        def execute_thread():
            try:
                result = self.api_executor.execute_at_command(read_cmd)
                
                # Update result display
                result_widget = self.command_widgets[cmd_name].get('read_result')
                if result_widget:
                    result_widget.config(state=tk.NORMAL)
                    result_widget.delete(1.0, tk.END)
                    if result.success:
                        result_widget.insert(tk.END, result.result_data or "OK")
                        result_widget.tag_add("success", "1.0", tk.END)
                        result_widget.tag_config("success", foreground="green")
                    else:
                        error_msg = result.error_message or "Command failed"
                        result_widget.insert(tk.END, error_msg)
                        result_widget.tag_add("error", "1.0", tk.END)
                        result_widget.tag_config("error", foreground="red")
                    result_widget.config(state=tk.DISABLED)
                
                self.log(f"✓ {cmd_name} read complete")
            except Exception as e:
                self.log(f"ERROR reading {cmd_name}: {e}")
        
        threading.Thread(target=execute_thread, daemon=True).start()
    
    def _execute_write(self, cmd_name: str, param_vars: Dict[str, tk.StringVar]):
        """Execute write command with parameters"""
        # Check if connected to device first
        if not self.main_window.is_connected():
            messagebox.showwarning("Not Connected", 
                                 "Please connect to the device first",
                                 parent=self.parent)
            return
        
        # Collect parameter values
        params = []
        for param_name, var in param_vars.items():
            value = var.get().strip()
            if value:
                params.append(value)
        
        # Build write command
        if params:
            write_cmd = f"{cmd_name}={','.join(params)}"
        else:
            write_cmd = f"{cmd_name}="
        
        self.log(f"Writing {write_cmd}...")
        
        def execute_thread():
            try:
                result = self.api_executor.execute_at_command(write_cmd)
                
                # Update result display
                result_widget = self.command_widgets[cmd_name].get('write_result')
                if result_widget:
                    result_widget.config(state=tk.NORMAL)
                    result_widget.delete(1.0, tk.END)
                    if result.success:
                        result_widget.insert(tk.END, "✓ Command executed successfully")
                        result_widget.tag_add("success", "1.0", tk.END)
                        result_widget.tag_config("success", foreground="green")
                    else:
                        error_msg = result.error_message or "Command failed"
                        result_widget.insert(tk.END, f"✗ {error_msg}")
                        result_widget.tag_add("error", "1.0", tk.END)
                        result_widget.tag_config("error", foreground="red")
                    result_widget.config(state=tk.DISABLED)
                
                self.log(f"✓ {write_cmd} complete")
            except Exception as e:
                self.log(f"ERROR writing {write_cmd}: {e}")
        
        threading.Thread(target=execute_thread, daemon=True).start()
    
    def _execute_action(self, cmd_name: str):
        """Execute action command"""
        # Check if connected to device first
        if not self.main_window.is_connected():
            messagebox.showwarning("Not Connected", 
                                 "Please connect to the device first",
                                 parent=self.parent)
            return
        
        self.log(f"Executing {cmd_name}...")
        
        def execute_thread():
            try:
                result = self.api_executor.execute_at_command(cmd_name)
                
                # Update result display
                result_widget = self.command_widgets[cmd_name].get('action_result')
                if result_widget:
                    result_widget.config(state=tk.NORMAL)
                    result_widget.delete(1.0, tk.END)
                    if result.success:
                        result_widget.insert(tk.END, result.result_data or "✓ OK")
                        result_widget.tag_add("success", "1.0", tk.END)
                        result_widget.tag_config("success", foreground="green")
                    else:
                        error_msg = result.error_message or "Command failed"
                        result_widget.insert(tk.END, f"✗ {error_msg}")
                        result_widget.tag_add("error", "1.0", tk.END)
                        result_widget.tag_config("error", foreground="red")
                    result_widget.config(state=tk.DISABLED)
                
                self.log(f"✓ {cmd_name} complete")
            except Exception as e:
                self.log(f"ERROR executing {cmd_name}: {e}")
        
        threading.Thread(target=execute_thread, daemon=True).start()
    
    # WiFi methods
    
    def _wifi_scan(self):
        """Execute WiFi scan and populate results"""
        # Check if connected first
        if not self.main_window.is_connected():
            messagebox.showwarning("Not Connected", 
                                 "Please connect to the device first",
                                 parent=self.parent)
            return
        
        self.log("Starting WiFi scan...")
        self.wifi_scan_status.config(text="Scanning...")
        
        # Clear existing results
        for item in self.wifi_tree.get_children():
            self.wifi_tree.delete(item)
        
        def scan_thread():
            try:
                # Find scan command dynamically
                scan_cmd = self._find_command_by_keyword('Wi-Fi', 'scan')
                if not scan_cmd:
                    self.log("ERROR: No WiFi scan command found")
                    self.wifi_scan_status.config(text="Error: No scan command")
                    return
                
                result = self.api_executor.execute_at_command(scan_cmd)
                
                if result.success and result.result_data:
                    networks, filtered_count = self._parse_wifi_results(result.result_data)
                    
                    # Update GUI
                    for net in networks:
                        self.wifi_tree.insert('', tk.END, values=(
                            net.get('ssid', ''),
                            net.get('bssid', ''),
                            net.get('channel', ''),
                            net.get('rssi', ''),
                            net.get('security', '')
                        ))
                    
                    # Log results with filter info
                    if filtered_count > 0:
                        self.log(f"✓ Found {len(networks)} networks ({filtered_count} without SSID filtered out)")
                        self.wifi_scan_status.config(text=f"Found {len(networks)} networks ({filtered_count} filtered)")
                    else:
                        self.log(f"✓ Found {len(networks)} networks")
                        self.wifi_scan_status.config(text=f"Found {len(networks)} networks")
                else:
                    error = result.error_message or "Scan failed"
                    self.log(f"WiFi scan error: {error}")
                    self.wifi_scan_status.config(text="Scan failed")
            except Exception as e:
                self.log(f"ERROR in WiFi scan: {e}")
                self.wifi_scan_status.config(text="Error")
        
        threading.Thread(target=scan_thread, daemon=True).start()
    
    def _wifi_connect(self):
        """Connect to WiFi network"""
        # Check if connected to device first
        if not self.main_window.is_connected():
            messagebox.showwarning("Not Connected", 
                                 "Please connect to the device first",
                                 parent=self.parent)
            return
        
        ssid = self.wifi_ssid_var.get().strip()
        password = self.wifi_pass_var.get().strip()
        
        if not ssid:
            messagebox.showwarning("Missing Information", "Please enter an SSID")
            return
        
        # Save WiFi settings
        if self.settings:
            self.settings.set_wifi_config(ssid=ssid, password=password)
            self.settings.save()
        
        self.log(f"Connecting to WiFi: {ssid}")
        self._update_wifi_status(f"Configuring WiFi parameters...\n")
        
        def connect_thread():
            try:
                # Step 1: Set connection parameters (SSID and password) using direct API call
                self.log("Setting WiFi parameters using direct API call...")
                result = self.api_executor._wifi_set_connection_params([ssid, password])
                if not result.success:
                    error = result.error_message or "Failed to set WiFi parameters"
                    self._update_wifi_status(f"✗ Configuration failed: {error}\n")
                    self.log(f"WiFi config failed: {error}")
                    return
                self._update_wifi_status(f"✓ Parameters configured\n")
                
                # Step 2: Connect to the network using direct API call
                self._update_wifi_status(f"Connecting to {ssid}...\n")
                self.log("Connecting using direct API call...")
                result = self.api_executor._wifi_connect([])
                
                if result.success:
                    # Get IP address from result data if available
                    ip_address = "unknown"
                    if result.result_data and isinstance(result.result_data, dict):
                        ip_address = result.result_data.get('ip', 'unknown')
                    
                    self._update_wifi_status(f"✓ Connected to {ssid}\nIP Address: {ip_address}\n")
                    self.log(f"✓ Successfully connected to {ssid} - IP: {ip_address}")
                else:
                    error = result.error_message or "Connection failed"
                    self._update_wifi_status(f"✗ Connection failed: {error}\n")
                    self.log(f"WiFi connection failed: {error}")
                    
            except Exception as e:
                error_msg = str(e)
                self._update_wifi_status(f"✗ Error: {error_msg}\n")
                self.log(f"ERROR in WiFi connect: {error_msg}")
        
        threading.Thread(target=connect_thread, daemon=True).start()
    
    def _wifi_disconnect(self):
        """Disconnect from WiFi network"""
        # Check if connected to device first
        if not self.main_window.is_connected():
            messagebox.showwarning("Not Connected", 
                                 "Please connect to the device first",
                                 parent=self.parent)
            return
        
        self.log("Disconnecting from WiFi...")
        self._update_wifi_status("Disconnecting...\n")
        
        def disconnect_thread():
            try:
                # Use direct API call
                result = self.api_executor._wifi_disconnect([])
                
                if result.success:
                    self._update_wifi_status("✓ Disconnected\n")
                    self.log("✓ WiFi disconnected")
                else:
                    error = result.error_message or "Disconnect failed"
                    self._update_wifi_status(f"✗ Disconnect failed: {error}\n")
                    self.log(f"WiFi disconnect failed: {error}")
            except Exception as e:
                self._update_wifi_status(f"✗ Error: {e}\n")
        
        threading.Thread(target=disconnect_thread, daemon=True).start()
    
    def _bluetooth_scan(self):
        """Execute Bluetooth scan and populate results"""
        self.log("Starting Bluetooth scan...")
        self.bt_scan_status.config(text="Scanning...")
        
        # Clear existing results
        for item in self.bt_tree.get_children():
            self.bt_tree.delete(item)
        
        def scan_thread():
            try:
                # Find discovery command (Bluetooth uses AT+UBTD for discovery)
                scan_cmd = self._find_command_by_keyword('Bluetooth', 'discovery')
                if not scan_cmd:
                    # Fallback to scan keyword
                    scan_cmd = self._find_command_by_keyword('Bluetooth', 'UBTD')
                
                if not scan_cmd:
                    self.log("No Bluetooth scan/discovery command found in YAML")
                    self.log("Using direct API call instead...")
                    # Call API directly
                    result = self.api_executor._bluetooth_scan_begin_end([])
                else:
                    self.log(f"Using YAML command: {scan_cmd}")
                    result = self.api_executor.execute_at_command(scan_cmd)
                
                self.log(f"DEBUG: result.success={result.success}, result.result_data={result.result_data}")
                
                if result.success and result.result_data:
                    devices, filtered_count = self._parse_bluetooth_results(result.result_data)
                    self.log(f"DEBUG: Parsed {len(devices)} devices from result_data")
                    
                    # Update GUI
                    for dev in devices:
                        self.bt_tree.insert('', tk.END, values=(
                            dev.get('name', 'Unknown'),
                            dev.get('address', ''),
                            dev.get('rssi', ''),
                            dev.get('type', '')
                        ))
                    
                    # Log results with filter info
                    if filtered_count > 0:
                        self.log(f"✓ Found {len(devices)} named devices ({filtered_count} without names filtered out)")
                        self.bt_scan_status.config(text=f"Found {len(devices)} devices ({filtered_count} filtered)")
                    else:
                        self.log(f"✓ Found {len(devices)} devices")
                        self.bt_scan_status.config(text=f"Found {len(devices)} devices")
                elif result.success and not result.result_data:
                    self.log("Bluetooth scan succeeded but returned empty list")
                    self.bt_scan_status.config(text="No devices found")
                else:
                    self.log(f"Bluetooth scan failed: {result.error_message}")
                    self.bt_scan_status.config(text="Scan failed")
            except Exception as e:
                self.log(f"ERROR in Bluetooth scan: {e}")
                self.bt_scan_status.config(text="Error")
        
        threading.Thread(target=scan_thread, daemon=True).start()
    
    def _bluetooth_connect(self):
        """Connect to Bluetooth device"""
        address = self.bt_addr_var.get().strip()
        
        if not address:
            messagebox.showwarning("Missing Information", "Please enter a device address")
            return
        
        # Save Bluetooth settings
        if self.settings:
            self.settings.set_bluetooth_config(remote_bd_addr=address)
            self.settings.save()
        
        self.log(f"Connecting to Bluetooth device: {address}")
        self._update_bt_status(f"Connecting to {address}...\n")
        
        def connect_thread():
            try:
                # Find connect command dynamically
                connect_cmd = self._find_command_by_keyword('Bluetooth', 'connect')
                if not connect_cmd:
                    self._update_bt_status("ERROR: No Bluetooth connect command found\n")
                    return
                
                # Execute with address as parameter
                result = self.api_executor.execute_at_command(connect_cmd, [address])
                
                if result.success:
                    conn_handle = result.result_data.get('conn_handle') if result.result_data else None
                    if conn_handle is not None:
                        self._update_bt_status(f"✓ Connected to {address} (handle: {conn_handle})\n")
                        self.log(f"✓ Bluetooth connected: {address} (handle: {conn_handle})")
                    else:
                        self._update_bt_status(f"✓ Connected to {address}\n")
                        self.log(f"✓ Bluetooth connected: {address}")
                else:
                    error = result.error_message or "Connection failed"
                    self._update_bt_status(f"✗ Connection failed: {error}\n")
            except Exception as e:
                self._update_bt_status(f"✗ Error: {e}\n")
        
        threading.Thread(target=connect_thread, daemon=True).start()
    
    def _bluetooth_disconnect(self):
        """Disconnect Bluetooth device"""
        self.log("Disconnecting Bluetooth...")
        self._update_bt_status("Disconnecting...\n")
        
        def disconnect_thread():
            try:
                # Find disconnect command dynamically
                disconnect_cmd = self._find_command_by_keyword('Bluetooth', 'disconnect')
                if not disconnect_cmd:
                    self._update_bt_status("ERROR: No Bluetooth disconnect command found\n")
                    return
                
                result = self.api_executor.execute_at_command(disconnect_cmd)
                
                if result.success:
                    self._update_bt_status("✓ Disconnected\n")
                    self.log("✓ Bluetooth disconnected")
                else:
                    error = result.error_message or "Disconnect failed"
                    self._update_bt_status(f"✗ Disconnect failed: {error}\n")
            except Exception as e:
                self._update_bt_status(f"✗ Error: {e}\n")
        
        threading.Thread(target=disconnect_thread, daemon=True).start()
    
    # Helper methods
    
    def _find_command_by_keyword(self, group: str, keyword: str) -> Optional[str]:
        """Find command in group by keyword in name or description"""
        commands = self.yaml_parser.get_commands_in_group(group)
        
        for cmd_name in commands:
            cmd_info = self.yaml_parser.get_command_info(cmd_name)
            if not cmd_info:
                continue
            
            # Safely get string values, handling None and lists
            brief = str(cmd_info.get('brief') or '').lower()
            desc_raw = cmd_info.get('desc')
            if isinstance(desc_raw, list):
                desc = ' '.join(str(item) for item in desc_raw).lower()
            else:
                desc = str(desc_raw or '').lower()
            api_name = str(cmd_info.get('api_name') or '').lower()
            
            if keyword.lower() in cmd_name.lower() or \
               keyword.lower() in brief or \
               keyword.lower() in desc or \
               keyword.lower() in api_name:
                return cmd_name
        
        return None
    
    def _parse_wifi_results(self, data) -> tuple[List[Dict], int]:
        """Parse WiFi scan results from UCX API
        
        Returns:
            tuple: (list of networks with valid SSIDs, count of filtered networks)
        """
        networks = []
        filtered_count = 0
        
        # UCX API returns structured data (list of dicts)
        if isinstance(data, list):
            for item in data:
                if isinstance(item, dict):
                    # Get the SSID
                    ssid = item.get('ssid', '')
                    
                    # Filter out networks with empty, whitespace-only, or hidden SSIDs
                    if not ssid or not ssid.strip():
                        filtered_count += 1
                        continue  # Skip empty or whitespace-only SSIDs
                    
                    # Skip networks with generic placeholder SSIDs
                    ssid_lower = ssid.strip().lower()
                    if ssid_lower in ['unknown', 'n/a', 'na', 'hidden', '']:
                        filtered_count += 1
                        continue
                    
                    networks.append({
                        'ssid': ssid.strip(),
                        'bssid': item.get('bssid', ''),
                        'channel': item.get('channel', ''),
                        'rssi': str(item.get('signal', item.get('rssi', ''))),
                        'security': item.get('security', 'Unknown')
                    })
        
        return networks, filtered_count
    
    def _parse_bluetooth_results(self, data) -> tuple[List[Dict], int]:
        """Parse Bluetooth scan results from UCX API
        
        Returns:
            tuple: (list of devices with valid names, count of filtered devices)
        """
        devices = []
        filtered_count = 0
        
        # UCX API returns structured data (list of dicts)
        if isinstance(data, list):
            for item in data:
                if isinstance(item, dict):
                    # Get the raw name value
                    raw_name = item.get('name', '')
                    
                    # Filter out devices with empty, whitespace-only, or "Unknown" names
                    if not raw_name or not raw_name.strip():
                        filtered_count += 1
                        continue  # Skip empty or whitespace-only names
                    
                    # Skip devices with generic "Unknown" or similar placeholder names
                    name_lower = raw_name.strip().lower()
                    if name_lower in ['unknown', 'n/a', 'na', 'unnamed', '']:
                        filtered_count += 1
                        continue
                    
                    # Only include devices with valid names
                    devices.append({
                        'name': raw_name.strip(),
                        'address': item.get('address', item.get('bd_addr', '')),
                        'rssi': str(item.get('rssi', item.get('signal', ''))),
                        'type': item.get('type', item.get('device_type', ''))
                    })
        
        return devices, filtered_count
    
    def _toggle_wifi_password(self):
        """Toggle WiFi password visibility"""
        if self.wifi_show_pass.get():
            self.wifi_pass_entry.config(show="")
        else:
            self.wifi_pass_entry.config(show="●")
    
    def _on_wifi_network_select(self, event):
        """Auto-fill SSID when network is selected"""
        selection = self.wifi_tree.selection()
        if selection:
            item = self.wifi_tree.item(selection[0])
            values = item['values']
            if values:
                self.wifi_ssid_var.set(values[0])  # SSID is first column
    
    def _on_bluetooth_device_select(self, event):
        """Auto-fill address when device is selected"""
        selection = self.bt_tree.selection()
        if selection:
            item = self.bt_tree.item(selection[0])
            values = item['values']
            if values and len(values) > 1:
                self.bt_addr_var.set(values[1])  # Address is second column
    
    def _update_wifi_status(self, message: str):
        """Update WiFi status text widget"""
        self.wifi_status_text.config(state=tk.NORMAL)
        self.wifi_status_text.insert(tk.END, f"[{datetime.now().strftime('%H:%M:%S')}] {message}")
        self.wifi_status_text.see(tk.END)
        self.wifi_status_text.config(state=tk.DISABLED)
    
    def _update_bt_status(self, message: str):
        """Update Bluetooth status text widget"""
        self.bt_status_text.config(state=tk.NORMAL)
        self.bt_status_text.insert(tk.END, f"[{datetime.now().strftime('%H:%M:%S')}] {message}")
        self.bt_status_text.see(tk.END)
        self.bt_status_text.config(state=tk.DISABLED)
