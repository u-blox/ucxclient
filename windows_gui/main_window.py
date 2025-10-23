"""
Main window for ucxclient GUI application

This module implements the main window with COM port selection,
connection management, and tabbed interface for different features.
"""

# GUI Version (not the ucxclient API version)
UCXCLIENT_GUI_VERSION = "1.0.0"

# Prevent Python from creating __pycache__ directories
import os
os.environ['PYTHONDONTWRITEBYTECODE'] = '1'

import tkinter as tk
from tkinter import ttk, messagebox, scrolledtext, filedialog
import threading
import time
from typing import Optional, List, Tuple, Dict
import sys
from datetime import datetime
import serial.tools.list_ports
import subprocess

# Add parent directory to path to import ucx_wrapper
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

# Import u-blox portenum for proper EVK detection
try:
    import portenum
    PORTENUM_AVAILABLE = True
except ImportError:
    PORTENUM_AVAILABLE = False
    portenum = None

from ucx_wrapper import UcxClientWrapper, UcxClientError
from at_to_api_mapper import at_to_api_mapper
from ucx_api_executor import UCXAPIExecutor
from yaml_parser import YAMLCommandParser, get_yaml_parser
from product_selector_dialog import ProductSelectorDialog
from dynamic_product_gui import DynamicProductGUI
from settings_manager import get_settings_manager
from test_tab import SimpleTestTab
from mapper_tab import MapperTab
from firmware_tab import FirmwareUpdateTab


class MainWindow:
    """Main application window"""
    
    def __init__(self):
        self.root = tk.Tk()
        self.root.title(f"ucxclient GUI v{UCXCLIENT_GUI_VERSION}")
        
        # Hide window initially to prevent flickering during setup
        self.root.withdraw()
        
        self.root.geometry("800x600")
        self.root.minsize(600, 400)
        
        # Apply dark theme
        self._apply_dark_theme()
        
        # Set application icon (Windows only)
        try:
            if sys.platform == "win32":
                icon_path = os.path.join(os.path.dirname(__file__), "shortrange.ico")
                if os.path.exists(icon_path):
                    self.root.iconbitmap(icon_path)
        except Exception:
            pass
        
        # Application state
        self.ucx_client: Optional[UcxClientWrapper] = None
        self.api_executor: Optional[UCXAPIExecutor] = None
        self.connected = False
        self.connection_thread: Optional[threading.Thread] = None
        self.device_name = "U-ConnectXpress Device"
        self.yaml_parser: Optional[YAMLCommandParser] = None
        self.product_loaded = False
        self.dynamic_gui: Optional[DynamicProductGUI] = None
        
        # Settings manager
        self.settings = get_settings_manager()
        
        # Initialize basic UCX wrapper
        try:
            self.ucx_client = UcxClientWrapper()
        except Exception as e:
            print(f"Warning: Could not initialize basic UCX wrapper: {e}")
            self.ucx_client = None
        
        # Load saved settings
        serial_config = self.settings.get_serial_config()
        
        # GUI components
        self.status_var = tk.StringVar(value="Disconnected")
        self.port_var = tk.StringVar(value=serial_config.get('port', ''))
        self.baud_var = tk.StringVar(value=str(serial_config.get('baudrate', 115200)))
        self.flow_control_var = tk.BooleanVar(value=serial_config.get('flow_control', False))
        
        self._create_widgets()
        self._setup_layout()
        self._load_window_position()
        self._refresh_ports()
        
        # Auto-detect EVK and show product selector on startup
        self.root.after(100, self._startup_sequence)
    
    def _apply_dark_theme(self):
        """Apply dark theme to the application"""
        # Define dark theme colors
        bg_dark = '#1e1e1e'
        bg_darker = '#252526'
        bg_lighter = '#2d2d30'
        fg_light = '#e0e0e0'
        fg_bright = '#ffffff'
        accent = '#007acc'
        accent_hover = '#0098ff'
        border = '#3e3e42'
        
        # Configure root window
        self.root.configure(bg=bg_dark)
        
        # Create custom ttk style
        style = ttk.Style()
        style.theme_use('clam')
        
        # Configure colors for all ttk widgets
        style.configure('.', 
                       background=bg_dark,
                       foreground=fg_light,
                       bordercolor=border,
                       darkcolor=bg_darker,
                       lightcolor=bg_lighter,
                       troughcolor=bg_darker,
                       fieldbackground=bg_darker,
                       selectbackground=accent,
                       selectforeground=fg_bright)
        
        # Frame
        style.configure('TFrame', background=bg_dark)
        style.configure('TLabelframe', background=bg_dark, foreground=fg_light, bordercolor=border)
        style.configure('TLabelframe.Label', background=bg_dark, foreground=fg_light)
        
        # Label
        style.configure('TLabel', background=bg_dark, foreground=fg_light)
        
        # Button
        style.configure('TButton', 
                       background=bg_lighter,
                       foreground=fg_light,
                       bordercolor=border,
                       focuscolor=accent,
                       highlightcolor=accent)
        style.map('TButton',
                 background=[('active', accent_hover), ('pressed', accent)],
                 foreground=[('active', fg_bright)])
        
        # Combobox
        style.configure('TCombobox',
                       fieldbackground=bg_darker,
                       background=bg_lighter,
                       foreground=fg_light,
                       arrowcolor=fg_light,
                       bordercolor=border)
        style.map('TCombobox',
                 fieldbackground=[('readonly', bg_darker)],
                 selectbackground=[('readonly', bg_darker)],
                 selectforeground=[('readonly', fg_light)])
        
        # Entry
        style.configure('TEntry',
                       fieldbackground=bg_darker,
                       foreground=fg_light,
                       bordercolor=border,
                       insertcolor=fg_bright)
        
        # Checkbutton
        style.configure('TCheckbutton',
                       background=bg_dark,
                       foreground=fg_light,
                       indicatorcolor=bg_darker,
                       bordercolor=border)
        style.map('TCheckbutton',
                 background=[('active', bg_dark)],
                 indicatorcolor=[('selected', accent)])
        
        # Notebook (tabs)
        style.configure('TNotebook', 
                       background=bg_dark,
                       bordercolor=border,
                       tabmargins=[2, 5, 2, 0])
        style.configure('TNotebook.Tab',
                       background=bg_darker,
                       foreground=fg_light,
                       bordercolor=border,
                       padding=[10, 5])
        style.map('TNotebook.Tab',
                 background=[('selected', bg_lighter), ('active', bg_lighter)],
                 foreground=[('selected', fg_bright)])
        
        # Treeview
        style.configure('Treeview',
                       background=bg_darker,
                       foreground=fg_light,
                       fieldbackground=bg_darker,
                       bordercolor=border,
                       rowheight=25)
        style.configure('Treeview.Heading',
                       background=bg_lighter,
                       foreground=fg_light,
                       bordercolor=border,
                       relief='flat')
        style.map('Treeview',
                 background=[('selected', accent)],
                 foreground=[('selected', fg_bright)])
        style.map('Treeview.Heading',
                 background=[('active', accent_hover)])
        
        # Scrollbar
        style.configure('Vertical.TScrollbar',
                       background=bg_darker,
                       troughcolor=bg_darker,
                       bordercolor=border,
                       arrowcolor=fg_light)
        style.configure('Horizontal.TScrollbar',
                       background=bg_darker,
                       troughcolor=bg_darker,
                       bordercolor=border,
                       arrowcolor=fg_light)
        
        # Store colors for use by other widgets
        self.theme_colors = {
            'bg_dark': bg_dark,
            'bg_darker': bg_darker,
            'bg_lighter': bg_lighter,
            'fg_light': fg_light,
            'fg_bright': fg_bright,
            'accent': accent,
            'border': border
        }
    
    def _create_widgets(self):
        """Create all GUI widgets"""
        
        # Create menu bar
        self._create_menu()
        
        # Main frame
        main_frame = ttk.Frame(self.root)
        main_frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=10)
        
        # Connection frame
        conn_frame = ttk.LabelFrame(main_frame, text="Connection")
        conn_frame.pack(fill=tk.X, pady=(0, 10))
        
        # Port selection
        ttk.Label(conn_frame, text="COM Port:").grid(row=0, column=0, sticky=tk.W, padx=5, pady=5)
        self.port_combo = ttk.Combobox(conn_frame, textvariable=self.port_var, width=10, state="readonly")
        self.port_combo.grid(row=0, column=1, padx=5, pady=5)
        
        # Refresh ports button
        ttk.Button(conn_frame, text="Refresh", command=self._refresh_ports).grid(row=0, column=2, padx=5, pady=5)
        
        # EVK detect button
        button_text = "EVK" if PORTENUM_AVAILABLE else "FTDI"
        ttk.Button(conn_frame, text=button_text, command=self._detect_nora_w36_evk).grid(row=0, column=3, padx=2, pady=5)
        
        # Auto-detect button
        ttk.Button(conn_frame, text="Auto Detect", command=self._auto_detect_device).grid(row=0, column=4, padx=5, pady=5)
        
        # Baud rate
        ttk.Label(conn_frame, text="Baud Rate:").grid(row=0, column=5, sticky=tk.W, padx=5, pady=5)
        baud_combo = ttk.Combobox(conn_frame, textvariable=self.baud_var, width=10, state="readonly")
        baud_combo['values'] = ('9600', '19200', '38400', '57600', '115200', '230400', '460800', '921600')
        baud_combo.grid(row=0, column=6, padx=5, pady=5)
        
        # Flow control
        ttk.Checkbutton(conn_frame, text="Flow Control", variable=self.flow_control_var).grid(row=0, column=7, padx=5, pady=5)
        
        # Connect button
        self.connect_btn = ttk.Button(conn_frame, text="Connect", command=self._toggle_connection)
        self.connect_btn.grid(row=0, column=8, padx=5, pady=5)
        
        # Status
        status_frame = ttk.Frame(conn_frame)
        status_frame.grid(row=1, column=0, columnspan=9, sticky=tk.EW, padx=5, pady=5)
        ttk.Label(status_frame, text="Status:").pack(side=tk.LEFT)
        self.status_label = ttk.Label(status_frame, textvariable=self.status_var, foreground="red")
        self.status_label.pack(side=tk.LEFT, padx=(5, 0))
        
        conn_frame.columnconfigure(6, weight=1)
        
        # Notebook for tabs
        self.notebook = ttk.Notebook(main_frame)
        self.notebook.pack(fill=tk.BOTH, expand=True)
        
        # Log tab (main output)
        self._create_log_tab()
        
        # Test and Mapper tabs (will be created after product is loaded)
        self.test_tab = None
        self.mapper_tab = None
        self.firmware_tab = None
    
    def _create_menu(self):
        """Create menu bar"""
        menubar = tk.Menu(self.root)
        self.root.config(menu=menubar)
        
        # File menu
        file_menu = tk.Menu(menubar, tearoff=0)
        menubar.add_cascade(label="File", menu=file_menu)
        file_menu.add_command(label="Change Product/Version...", command=self._change_product)
        file_menu.add_separator()
        file_menu.add_command(label="Save Log...", command=self._save_log)
        file_menu.add_separator()
        file_menu.add_command(label="Exit", command=self.root.quit)
        
        # Build menu
        build_menu = tk.Menu(menubar, tearoff=0)
        menubar.add_cascade(label="Build", menu=build_menu)
        build_menu.add_command(label="Build All (Release + Debug)", command=lambda: self._run_build("all"))
        build_menu.add_command(label="Build Release", command=lambda: self._run_build("release"))
        build_menu.add_command(label="Build Debug", command=lambda: self._run_build("debug"))
        build_menu.add_separator()
        build_menu.add_command(label="Rebuild All (Clean + Build)", command=lambda: self._run_build("rebuild"))
        build_menu.add_separator()
        build_menu.add_command(label="Clean Intermediate Files", command=lambda: self._run_build("clean"))
        build_menu.add_command(label="Clean All (Including Binaries)", command=lambda: self._run_build("cleanall"))
        
        # Tools menu
        tools_menu = tk.Menu(menubar, tearoff=0)
        menubar.add_cascade(label="Tools", menu=tools_menu)
        tools_menu.add_command(label="Compare YAML vs API...", command=self._show_api_comparison)
        tools_menu.add_separator()
        tools_menu.add_command(label="Clear Settings", command=self._clear_settings)
        
        # Help menu
        help_menu = tk.Menu(menubar, tearoff=0)
        menubar.add_cascade(label="Help", menu=help_menu)
        help_menu.add_command(label="About", command=self._show_about)
    
    def _create_log_tab(self):
        """Create logging tab"""
        log_frame = ttk.Frame(self.notebook)
        self.notebook.add(log_frame, text="Log")
        
        self.log_text = scrolledtext.ScrolledText(log_frame, height=20, state=tk.DISABLED,
                                                   bg=self.theme_colors['bg_darker'],
                                                   fg=self.theme_colors['fg_light'],
                                                   insertbackground=self.theme_colors['fg_bright'],
                                                   selectbackground=self.theme_colors['accent'],
                                                   selectforeground=self.theme_colors['fg_bright'],
                                                   borderwidth=0, highlightthickness=1,
                                                   highlightbackground=self.theme_colors['border'])
        self.log_text.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        # Configure color tags for log messages (bright colors for dark theme)
        # Using only foreground without font to avoid conflicts
        self.log_text.tag_config('success', foreground='#00FF00')  # Bright Green
        self.log_text.tag_config('error', foreground='#FF0000')    # Bright Red
        self.log_text.tag_config('warning', foreground='#FFA500')  # Orange
        self.log_text.tag_config('info', foreground='#00BFFF')     # Deep Sky Blue
        self.log_text.tag_config('list', foreground='#00FFFF')     # Cyan
        self.log_text.tag_config('search', foreground='#FF00FF')   # Magenta
        
        # Set tag priorities
        for tag in ['success', 'error', 'warning', 'info', 'list', 'search']:
            self.log_text.tag_raise(tag)
        
        # Controls frame
        controls_frame = ttk.Frame(log_frame)
        controls_frame.pack(fill=tk.X, padx=5, pady=5)
        
        # Log controls
        ttk.Button(controls_frame, text="Clear Log", command=self._clear_log).pack(side=tk.LEFT)
        ttk.Button(controls_frame, text="Save Log", command=self._save_log).pack(side=tk.LEFT, padx=5)
        
        # Separator
        ttk.Separator(controls_frame, orient=tk.VERTICAL).pack(side=tk.LEFT, fill=tk.Y, padx=10)
        
        # Quick commands
        ttk.Label(controls_frame, text="Quick Commands:").pack(side=tk.LEFT, padx=(5, 10))
        ttk.Button(controls_frame, text="AT (Test)", command=self._quick_at_test, width=12).pack(side=tk.LEFT, padx=2)
        ttk.Button(controls_frame, text="ATI9 (Info)", command=self._quick_ati9, width=12).pack(side=tk.LEFT, padx=2)
        ttk.Button(controls_frame, text="Store (AT&W)", command=self._quick_store_config, width=13).pack(side=tk.LEFT, padx=2)
        ttk.Button(controls_frame, text="Reboot", command=self._quick_reboot, width=12).pack(side=tk.LEFT, padx=2)
        ttk.Button(controls_frame, text="Factory Reset", command=self._quick_factory_reset, width=13).pack(side=tk.LEFT, padx=2)
    
    def _create_test_tab(self):
        """Create the automated test tab"""
        try:
            if self.yaml_parser and self.api_executor:
                self.test_tab = SimpleTestTab(
                    parent_notebook=self.notebook,
                    yaml_parser=self.yaml_parser,
                    api_executor=self.api_executor,
                    main_window=self,
                    theme_colors=self.theme_colors
                )
                self._log("✓ Test tab created successfully")
            else:
                self._log("⚠ Cannot create test tab: YAML parser or API executor not available")
        except Exception as e:
            self._log(f"✗ Error creating test tab: {e}")
            import traceback
            traceback.print_exc()
    
    def _create_mapper_tab(self):
        """Create the AT-to-API mapper tab"""
        try:
            if self.yaml_parser:
                # Import the mapper function
                from at_to_api_mapper import at_to_api_mapper
                
                # Get DLL reference for function availability checking
                ucx_dll = None
                if hasattr(self, 'ucx_client') and self.ucx_client and hasattr(self.ucx_client, '_dll'):
                    ucx_dll = self.ucx_client._dll
                
                self.mapper_tab = MapperTab(
                    parent_notebook=self.notebook,
                    yaml_parser=self.yaml_parser,
                    at_to_api_mapper=at_to_api_mapper,
                    theme_colors=self.theme_colors,
                    ucx_dll=ucx_dll,
                    main_window=self
                )
                self._log("✓ Mapper tab created successfully")
            else:
                self._log("⚠ Cannot create mapper tab: YAML parser not available")
        except Exception as e:
            self._log(f"✗ Error creating mapper tab: {e}")
            import traceback
            traceback.print_exc()
    
    def _create_firmware_tab(self):
        """Create the firmware update tab"""
        try:
            if hasattr(self, 'ucx_client') and self.ucx_client:
                self.firmware_tab = FirmwareUpdateTab(
                    parent=self.notebook,
                    ucx_wrapper=self.ucx_client,
                    settings_manager=self.settings
                )
                self.notebook.add(self.firmware_tab.get_frame(), text="Firmware")
                self._log("✓ Firmware tab created successfully")
            else:
                self._log("⚠ Cannot create firmware tab: Not connected")
        except Exception as e:
            self._log(f"✗ Error creating firmware tab: {e}")
            import traceback
            traceback.print_exc()
    
    def _setup_layout(self):
        """Setup window layout - just update sizes, positioning done in _load_window_position"""
        self.root.update_idletasks()
    
    def _load_window_position(self):
        """Load saved window position and size"""
        gui_config = self.settings.get_gui_config()
        width = gui_config.get('window_width', 800)
        height = gui_config.get('window_height', 600)
        x = gui_config.get('window_x')
        y = gui_config.get('window_y')
        
        # Update geometry to calculate proper sizes
        self.root.update_idletasks()
        
        if x is not None and y is not None:
            # Use saved position
            self.root.geometry(f"{width}x{height}+{x}+{y}")
        else:
            # Center on screen for first run
            screen_width = self.root.winfo_screenwidth()
            screen_height = self.root.winfo_screenheight()
            x = (screen_width // 2) - (width // 2)
            y = (screen_height // 2) - (height // 2)
            self.root.geometry(f"{width}x{height}+{x}+{y}")
        
        # Now show the window after it's properly positioned
        self.root.deiconify()
    
    def _center_window(self, window, width=None, height=None):
        """Center a window on screen or relative to parent
        
        Args:
            window: The window to center (Toplevel or Tk)
            width: Optional width, if None uses current width
            height: Optional height, if None uses current height
        """
        window.update_idletasks()
        
        if width is None or height is None:
            # Get current size
            geometry = window.geometry()
            size = geometry.split('+')[0].split('x')
            width = int(size[0]) if width is None else width
            height = int(size[1]) if height is None else height
        
        # Try to center relative to parent if it's a Toplevel
        if isinstance(window, tk.Toplevel):
            parent = window.master
            if parent and parent.winfo_viewable():
                x = parent.winfo_x() + (parent.winfo_width() // 2) - (width // 2)
                y = parent.winfo_y() + (parent.winfo_height() // 2) - (height // 2)
            else:
                # Parent not visible, center on screen
                screen_width = window.winfo_screenwidth()
                screen_height = window.winfo_screenheight()
                x = (screen_width // 2) - (width // 2)
                y = (screen_height // 2) - (height // 2)
        else:
            # Center on screen for main window
            screen_width = window.winfo_screenwidth()
            screen_height = window.winfo_screenheight()
            x = (screen_width // 2) - (width // 2)
            y = (screen_height // 2) - (height // 2)
        
        window.geometry(f"{width}x{height}+{x}+{y}")
    
    def _save_window_position(self):
        """Save current window position and size"""
        try:
            geometry = self.root.geometry()
            # Parse geometry string: WIDTHxHEIGHT+X+Y
            size_pos = geometry.split('+')
            if len(size_pos) >= 3:
                size = size_pos[0].split('x')
                width = int(size[0])
                height = int(size[1])
                x = int(size_pos[1])
                y = int(size_pos[2])
                
                self.settings.set_gui_config(
                    window_width=width,
                    window_height=height,
                    window_x=x,
                    window_y=y
                )
        except Exception as e:
            print(f"Error saving window position: {e}")
    
    def _refresh_ports(self):
        """Refresh COM port list"""
        if not self.ucx_client:
            return
        
        try:
            ports_info = serial.tools.list_ports.comports()
            available_ports = [port.device for port in ports_info]
            
            self.port_combo['values'] = available_ports
            if available_ports and not self.port_var.get():
                self.port_var.set(available_ports[0])
            
            self._log(f"Found {len(available_ports)} COM ports")
        except Exception as e:
            self._log(f"Error refreshing ports: {e}")
    
    def _detect_nora_w36_evk(self):
        """Detect NORA EVK"""
        self._log("🔍 Detecting u-blox EVK...")
        detected = self._auto_detect_evk_silent()
        
        if detected:
            self._log(f"✓ Found: {detected['desc']} on {detected['port']}")
            if detected['status'] == 'busy':
                messagebox.showwarning("Port Busy", f"EVK found on {detected['port']} but port is busy.\nPlease close other applications using this port.")
            elif detected['status'] == 'available' and not self.connected and self.product_loaded:
                if messagebox.askyesno("Connect?", f"Connect to {detected['desc']}?"):
                    self._connect()
        else:
            self._log("ℹ No EVK detected")
    
    def _test_port_availability(self, port_name):
        """Test if port is available"""
        try:
            import serial
            test_serial = serial.Serial(port=port_name, baudrate=115200, timeout=0.1, write_timeout=0.1)
            test_serial.close()
            return "available"
        except serial.SerialException as e:
            if "access is denied" in str(e).lower() or "in use" in str(e).lower():
                return "busy"
            return "unknown"
        except Exception:
            return "unknown"
    
    def _auto_detect_device(self):
        """Auto-detect device"""
        if not self.ucx_client or self.connected:
            return
        
        self._log("Starting auto-detection...")
        
        def detect_device():
            try:
                device_info = self.ucx_client.auto_detect_ucx_device()
                self.root.after(0, self._auto_detect_result, device_info)
            except Exception as e:
                self.root.after(0, self._auto_detect_error, str(e))
        
        threading.Thread(target=detect_device, daemon=True).start()
    
    def _auto_detect_result(self, device_info: Optional[Dict]):
        """Handle auto-detect result"""
        if device_info:
            self.port_var.set(device_info['port'])
            self.baud_var.set(str(device_info['baud_rate']))
            self.flow_control_var.set(device_info['flow_control'])
            
            info_msg = f"Device detected on {device_info['port']}:\n\n"
            if 'manufacturer' in device_info:
                info_msg += f"Manufacturer: {device_info['manufacturer']}\n"
            if 'model' in device_info:
                info_msg += f"Model: {device_info['model']}\n"
            
            messagebox.showinfo("Device Detected", info_msg)
            self._log(f"Auto-detected: {device_info.get('model', 'Unknown')} on {device_info['port']}")
        else:
            messagebox.showwarning("No Device Found", "No u-connectXpress device found.")
            self._log("Auto-detection: no device found")
    
    def _auto_detect_error(self, error: str):
        """Handle auto-detect error"""
        messagebox.showerror("Auto-Detection Error", f"Error: {error}")
        self._log(f"Auto-detection error: {error}")
    
    def _toggle_connection(self):
        """Toggle connection"""
        if self.connected:
            self._disconnect()
        else:
            self._connect()
    
    def _connect(self):
        """Connect to COM port"""
        if not self.ucx_client:
            messagebox.showerror("Error", "ucxclient not initialized")
            return
        
        port = self.port_var.get()
        if not port:
            messagebox.showerror("Error", "Please select a COM port")
            return
        
        baud_rate = int(self.baud_var.get())
        flow_control = self.flow_control_var.get()
        
        # Save serial settings
        self.settings.set_serial_config(port=port, baudrate=baud_rate, flow_control=flow_control)
        self.settings.save()
        
        self._log(f"Connecting to {port} at {baud_rate} baud (flow control: {flow_control})...")
        self.connect_btn.config(state=tk.DISABLED)
        self.status_var.set("Connecting...")
        self.status_label.config(foreground="orange")
        
        def connect_worker():
            try:
                self.ucx_client.initialize_client()
                success = self.ucx_client.connect(port, baud_rate, flow_control)
                self.root.after(0, self._connection_result, success, port)
            except Exception as e:
                self.root.after(0, self._connection_error, str(e))
        
        self.connection_thread = threading.Thread(target=connect_worker, daemon=True)
        self.connection_thread.start()
    
    def _connection_result(self, success: bool, port: str):
        """Handle connection result"""
        if success:
            self.connected = True
            self.status_var.set(f"Connected to {port}")
            self.status_label.config(foreground="green")
            self.connect_btn.config(text="Disconnect", state=tk.NORMAL)
            self._log(f"Successfully connected to {port}")
            
            if not self.api_executor:
                self.api_executor = UCXAPIExecutor(self.ucx_client)
                self._log("UCX API executor enabled")
            
            # Update dynamic GUI's api_executor reference if it exists
            if self.dynamic_gui:
                self.dynamic_gui.api_executor = self.api_executor
                self._log("Dynamic GUI executor reference updated")
            
            # Create test tab now that api_executor exists (schedule in main thread)
            if not self.test_tab:
                self.root.after(100, self._create_test_tab)
                self._log("Test tab will be created...")
            
            # Create mapper tab
            if not self.mapper_tab:
                self.root.after(200, self._create_mapper_tab)
                self._log("Mapper tab will be created...")
            
            # Create firmware tab
            if not self.firmware_tab:
                self.root.after(300, self._create_firmware_tab)
                self._log("Firmware tab will be created...")
        else:
            self.status_var.set("Connection failed")
            self.status_label.config(foreground="red")
            self.connect_btn.config(state=tk.NORMAL)
            self._log(f"Failed to connect to {port}")
    
    def _connection_error(self, error: str):
        """Handle connection error"""
        self.status_var.set("Connection error")
        self.status_label.config(foreground="red")
        self.connect_btn.config(state=tk.NORMAL)
        self._log(f"Connection error: {error}")
        messagebox.showerror("Connection Error", f"Failed to connect:\n{error}")
    
    def _disconnect(self):
        """Disconnect from COM port"""
        if not self.ucx_client:
            return
        
        self._log("Disconnecting...")
        try:
            self.ucx_client.disconnect()
            self.connected = False
            self.api_executor = None
            
            # Clear dynamic GUI's api_executor reference if it exists
            if self.dynamic_gui:
                self.dynamic_gui.api_executor = None
            
            self.status_var.set("Disconnected")
            self.status_label.config(foreground="red")
            self.connect_btn.config(text="Connect")
            self._log("Disconnected successfully")
        except Exception as e:
            self._log(f"Error during disconnect: {e}")
    
    def is_connected(self):
        """Check if connected"""
        return self.connected
    
    # Quick command handlers
    
    def _quick_at_test(self):
        """Execute AT test command"""
        if not self.is_connected():
            messagebox.showwarning("Not Connected", "Please connect to the device first")
            return
        
        if not self.api_executor:
            messagebox.showerror("Error", "API executor not initialized")
            return
        
        self._log("\n--- AT Test Command ---")
        result = self.api_executor.at_test()
        
        if result.success:
            self._log(f"✓ {result.result_data}")
        else:
            self._log(f"✗ {result.error_message}")
    
    def _quick_ati9(self):
        """Execute ATI9 device info command"""
        if not self.is_connected():
            messagebox.showwarning("Not Connected", "Please connect to the device first")
            return
        
        if not self.api_executor:
            messagebox.showerror("Error", "API executor not initialized")
            return
        
        self._log("\n--- ATI9 Device Information ---")
        result = self.api_executor.ati9_info()
        
        if result.success:
            self._log(result.result_data)
        else:
            self._log(f"✗ {result.error_message}")
    
    def _quick_store_config(self):
        """Execute AT&W store configuration command"""
        if not self.is_connected():
            messagebox.showwarning("Not Connected", "Please connect to the device first")
            return
        
        if not self.api_executor:
            messagebox.showerror("Error", "API executor not initialized")
            return
        
        if messagebox.askyesno("Store Configuration", 
                               "Save current configuration to flash memory?\n\n"
                               "This will store all settings permanently.",
                               parent=self.root):
            self._log("\n--- Store Configuration (AT&W) ---")
            result = self.api_executor.store_configuration()
            
            if result.success:
                self._log(f"✓ {result.result_data}")
                messagebox.showinfo("Success", "Configuration saved successfully!", parent=self.root)
            else:
                self._log(f"✗ {result.error_message}")
                messagebox.showerror("Error", f"Failed to store configuration:\n{result.error_message}", parent=self.root)
    
    def _quick_reboot(self):
        """Execute reboot command (AT+CPWROFF)"""
        if not self.is_connected():
            messagebox.showwarning("Not Connected", "Please connect to the device first")
            return
        
        if not self.api_executor:
            messagebox.showerror("Error", "API executor not initialized")
            return
        
        self._log("\n--- Reboot Device (AT+CPWROFF) ---")
        
        def reboot_thread():
            result = self.api_executor.reboot_device()
            
            if result.success:
                self._log("✓ Reboot command sent successfully")
            else:
                self._log(f"✗ Reboot failed: {result.error_message}")
        
        threading.Thread(target=reboot_thread, daemon=True).start()
    
    def _quick_factory_reset(self):
        """Execute factory reset command (AT&F + AT+CPWROFF)"""
        if not self.is_connected():
            messagebox.showwarning("Not Connected", "Please connect to the device first")
            return
        
        if not self.api_executor:
            messagebox.showerror("Error", "API executor not initialized")
            return
        
        if messagebox.askyesno("Factory Reset", 
                               "⚠️ WARNING: Factory Reset ⚠️\n\n"
                               "This will reset ALL settings to factory defaults (AT&F)\n"
                               "followed by a reboot (AT+CPWROFF).\n\n"
                               "ALL custom settings will be lost!\n\n"
                               "Continue with factory reset?",
                               icon='warning',
                               parent=self.root):
            self._log("\n--- Factory Reset (AT&F + AT+CPWROFF) ---")
            
            # First, reset to defaults
            result1 = self.api_executor.reset_to_defaults()
            if not result1.success:
                self._log(f"✗ Reset to defaults failed: {result1.error_message}")
                messagebox.showerror("Error", f"Failed to reset to defaults:\n{result1.error_message}", parent=self.root)
                return
            
            self._log(f"✓ {result1.result_data}")
            
            # Then reboot
            result2 = self.api_executor.reboot_device()
            if result2.success:
                self._log(f"✓ {result2.result_data}")
                messagebox.showinfo("Success", 
                                  "Factory reset complete!\n\n"
                                  "Device is rebooting with factory defaults.",
                                  parent=self.root)
            else:
                self._log(f"✗ {result2.error_message}")
                messagebox.showerror("Error", f"Reboot failed:\n{result2.error_message}", parent=self.root)
    
    def _log(self, message: str):
        """Add color-coded message to log
        
        Supports color prefixes:
        - ✓ or SUCCESS: Green
        - ✗ or ERROR: Red
        - ⚠ or WARNING: Yellow
        - ℹ or INFO: Blue
        - 📋 LIST: Cyan
        - 🔍 SEARCH: Magenta
        """
        if not hasattr(self, 'log_text') or self.log_text is None:
            print(f"[LOG] {message}")
            return
        
        timestamp = time.strftime("%H:%M:%S")
        log_entry = f"[{timestamp}] {message}\n"
        
        # Determine color based on message content
        color_tag = None
        msg_upper = message.upper()
        
        # Check for success indicators
        if '✓' in message or '✅' in message or 'SUCCESS' in msg_upper or message.startswith('Successfully'):
            color_tag = 'success'
        # Check for error indicators  
        elif '✗' in message or '❌' in message or message.startswith('ERROR') or 'ERROR:' in message or 'FAIL' in msg_upper:
            color_tag = 'error'
        # Check for warning indicators
        elif '⚠' in message or message.startswith('WARNING') or message.startswith('WARN') or 'warning' in message.lower():
            color_tag = 'warning'
        # Check for info indicators
        elif 'ℹ' in message or '📘' in message or message.startswith('INFO') or message.startswith('ℹ️'):
            color_tag = 'info'
        # Check for list/found indicators
        elif '📋' in message or 'Found' in message or 'Creating' in message or 'Building' in message or 'Loading' in message:
            color_tag = 'list'
        # Check for search/scan indicators
        elif '🔍' in message or 'Searching' in message or 'Scanning' in message or 'Auto-detecting' in message or 'Fetching' in message:
            color_tag = 'search'
        
        self.log_text.config(state=tk.NORMAL)
        
        # Insert with color tag using the tags parameter directly
        if color_tag:
            self.log_text.insert(tk.END, log_entry, color_tag)
        else:
            self.log_text.insert(tk.END, log_entry)
        
        self.log_text.config(state=tk.DISABLED)
        self.log_text.see(tk.END)
    
    def _clear_log(self):
        """Clear log"""
        self.log_text.config(state=tk.NORMAL)
        self.log_text.delete(1.0, tk.END)
        self.log_text.config(state=tk.DISABLED)
    
    def _save_log(self):
        """Save log to file"""
        filename = filedialog.asksaveasfilename(
            defaultextension=".txt",
            filetypes=[("Text files", "*.txt"), ("All files", "*.*")]
        )
        
        if filename:
            try:
                with open(filename, 'w') as f:
                    f.write(self.log_text.get(1.0, tk.END))
                self._log(f"Log saved to {filename}")
            except Exception as e:
                messagebox.showerror("Error", f"Failed to save log:\n{e}")
    
    def _startup_sequence(self):
        """Startup sequence"""
        self._log(f"ucxclient GUI v{UCXCLIENT_GUI_VERSION} - Dynamic GUI for u-connectXpress modules")
        self._log("")
        self._log("🔍 Auto-detecting u-blox EVK...")
        
        detected = self._auto_detect_evk_silent()
        
        if detected:
            self._log(f"✓ EVK detected: {detected['desc']}")
            self._log(f"✓ AT Port: {detected['port']}")
        else:
            self._log("ℹ No EVK auto-detected - please select COM port manually")
        
        self._show_product_selector()
    
    def _auto_detect_evk_silent(self):
        """Silent EVK detection"""
        if not PORTENUM_AVAILABLE:
            return None
        
        try:
            devices = portenum.enumerate_devices()
            nora_models = ['NORA-W36', 'NORA-B26', 'NORA-W46', 'NORA-W56']
            
            for dev in devices:
                if any(model in dev.desc.upper() for model in nora_models):
                    at_port = dev.get_at_port()
                    if at_port:
                        port_name = at_port.port
                        self.port_var.set(port_name)
                        status = self._test_port_availability(port_name)
                        return {'desc': dev.desc, 'port': port_name, 'status': status}
        except Exception as e:
            self._log(f"EVK detection error: {e}")
        
        return None
    
    def _auto_connect_if_ready(self):
        """Auto-connect if ready"""
        if not self.connected and self.product_loaded and self.port_var.get():
            self._log("🔗 Auto-connecting to EVK...")
            self._connect()
    
    def _show_product_selector(self):
        """Show product selector"""
        # Get saved product configuration
        product_config = self.settings.get_product_config()
        
        # Check if we should auto-load saved configuration
        if product_config.get('name') or product_config.get('local_path'):
            # Auto-load saved configuration
            source = product_config.get('source', 'github')
            if source == 'github':
                product = product_config.get('name', '')
                version = product_config.get('version', '')
                if product and version:
                    self._log(f"Auto-loading saved configuration: {product} v{version}")
                    self._load_product_configuration(source, product, version)
                    if not self.connected and self.port_var.get():
                        self.root.after(100, self._auto_connect_if_ready)
                    return
            else:
                local_path = product_config.get('local_path', '')
                if local_path and os.path.exists(local_path):
                    self._log(f"Auto-loading saved configuration from: {local_path}")
                    self._load_product_configuration(source, '', local_path)
                    if not self.connected and self.port_var.get():
                        self.root.after(100, self._auto_connect_if_ready)
                    return
        
        # Show dialog with saved defaults
        dialog = ProductSelectorDialog(
            self.root,
            default_source=product_config.get('source', 'github'),
            default_product=product_config.get('name', ''),
            default_version=product_config.get('version', ''),
            default_path=product_config.get('local_path', '')
        )
        result = dialog.show()
        
        if result:
            source, product, path = result
            self._load_product_configuration(source, product, path)
            
            if not self.connected and self.port_var.get():
                self.root.after(100, self._auto_connect_if_ready)
        else:
            messagebox.showinfo("No Configuration Loaded", "No product configuration was loaded. You can load one later from File > Load Product Configuration.")
    
    def _load_product_configuration(self, source: str, product: str, path: str):
        """Load product configuration"""
        self.yaml_parser = get_yaml_parser()
        self._log(f"Loading product configuration...")
        # Use update_idletasks instead of update to avoid event processing issues
        self.root.update_idletasks()
        
        success = False
        if source == "github":
            self._log(f"Fetching {product} v{path} from GitHub...")
            success = self.yaml_parser.load_from_github(product, path)
            if success:
                self.device_name = product
                # Save to settings
                self.settings.set_product_config(source='github', name=product, version=path)
                self.settings.save()
            else:
                # YAML not found - this is expected for some products
                self._log(f"⚠ Product configuration not available for {product} v{path}")
                self._log("  Note: YAML files may not exist for all products/versions")
                messagebox.showinfo(
                    "Configuration Not Available", 
                    f"Product configuration is not available for {product} v{path}.\n\n"
                    "This is expected for some products. You can:\n"
                    "• Try a different product/version\n"
                    "• Load a local YAML file if available\n"
                    "• Use the generic AT command interface"
                )
                return
        else:
            self._log(f"Loading from file: {path}")
            success = self.yaml_parser.load_from_file(path)
            if success:
                self.device_name = self.yaml_parser.product or "U-ConnectXpress Device"
                # Save to settings
                self.settings.set_product_config(source='local', local_path=path)
                self.settings.save()
            else:
                # Actual error loading local file
                self._log(f"✗ Failed to load product configuration from file")
                messagebox.showerror("Load Failed", "Failed to load product configuration.\nCheck the log for details.")
                return
        
        if success:
            self.product_loaded = True
            self._log(f"✓ Successfully loaded {self.yaml_parser.product} v{self.yaml_parser.version}")
            self._log(f"  Found {len(self.yaml_parser.commands)} AT commands in {len(self.yaml_parser.command_groups)} groups")
            
            self.root.title(f"ucxclient - {self.yaml_parser.product} v{self.yaml_parser.version}")
            
            at_to_api_mapper.set_yaml_parser(self.yaml_parser)
            api_mapping_count = len(at_to_api_mapper.get_all_mappings())
            self._log(f"  Built {api_mapping_count} AT-to-API mappings dynamically from YAML")
            
            self._build_dynamic_product_gui()
    
    def _build_dynamic_product_gui(self):
        """Build dynamic GUI"""
        if not self.yaml_parser or not self.product_loaded:
            return
        
        self._log("Building dynamic product GUI...")
        
        # Reset tab references so they will be recreated
        # Note: Tab clearing is handled by _change_product() before calling this
        self.test_tab = None
        self.mapper_tab = None
        self.firmware_tab = None
        
        self.dynamic_gui = DynamicProductGUI(
            parent_notebook=self.notebook,
            yaml_parser=self.yaml_parser,
            api_executor=self.api_executor,
            log_callback=self._log,
            main_window=self,
            theme_colors=self.theme_colors
        )
        
        self.dynamic_gui.build_gui()
        self._log("✓ Dynamic product GUI ready")
        
        # Recreate Test and Mapper tabs with new YAML
        if self.api_executor and self.connected:
            self._log("Recreating Test, Mapper and Firmware tabs for new product...")
            self._create_test_tab()
            self._create_mapper_tab()
            self._create_firmware_tab()
        else:
            self._log("Test, Mapper and Firmware tabs will be created after device connection")
    
    def _change_product(self):
        """Allow user to change product/version"""
        try:
            print("[DEBUG] _change_product called")
            # Get current product configuration
            product_config = self.settings.get_product_config()
            print(f"[DEBUG] product_config: {product_config}")
            
            # Show product selector dialog
            print("[DEBUG] Creating ProductSelectorDialog...")
            dialog = ProductSelectorDialog(
                self.root,
                default_source=product_config.get('source', 'github'),
                default_product=product_config.get('name', ''),
                default_version=product_config.get('version', ''),
                default_path=product_config.get('local_path', '')
            )
            print("[DEBUG] ProductSelectorDialog created, calling show()...")
            result = dialog.show()
            print(f"[DEBUG] dialog.show() returned: {result}")
        except Exception as e:
            print(f"[ERROR] Exception in _change_product: {e}")
            import traceback
            traceback.print_exc()
            self._log(f"✗ Error changing product: {e}")
            messagebox.showerror(
                "Error",
                f"Failed to change product:\n{str(e)}",
                parent=self.root
            )
            return
        
        if result:
            source, product, path = result
            self._log(f"\n{'='*60}")
            self._log("Changing product configuration...")
            self._log(f"{'='*60}\n")
            
            # Clear existing dynamic tabs manually (don't use old instance)
            if hasattr(self, 'dynamic_gui') and self.dynamic_gui:
                self._log("Cleaning up previous product GUI...")
                # Remove all tabs except Log (index 0)
                num_tabs = self.notebook.index("end")
                for i in range(num_tabs - 1, 0, -1):
                    self.notebook.forget(i)
                # Clear the old instance
                self.dynamic_gui = None
            
            # Load new configuration
            self._load_product_configuration(source, product, path)
    
    def _show_api_comparison(self):
        """Show comparison between YAML commands and available API functions"""
        if not self.yaml_parser or not self.product_loaded:
            messagebox.showwarning(
                "No Product Loaded",
                "Please load a product configuration first.",
                parent=self.root
            )
            return
        
        # Create comparison dialog
        dialog = tk.Toplevel(self.root)
        dialog.title(f"API Comparison - {self.yaml_parser.product} v{self.yaml_parser.version}")
        dialog.transient(self.root)
        
        # Hide initially to prevent flickering
        dialog.withdraw()
        
        # Main frame
        main_frame = ttk.Frame(dialog, padding=10)
        main_frame.pack(fill=tk.BOTH, expand=True)
        
        # Title
        title_label = ttk.Label(
            main_frame,
            text=f"YAML Commands vs UCX API Functions",
            font=('TkDefaultFont', 12, 'bold')
        )
        title_label.pack(pady=(0, 10))
        
        # Summary frame
        summary_frame = ttk.LabelFrame(main_frame, text="Summary", padding=10)
        summary_frame.pack(fill=tk.X, pady=(0, 10))
        
        # Run comparison
        comparison_results = self._compare_yaml_vs_api()
        
        # Display summary
        summary_text = (
            f"Product: {self.yaml_parser.product} v{self.yaml_parser.version}\n"
            f"Total YAML Commands: {comparison_results['total_yaml']}\n"
            f"Commands with API Mapping: {comparison_results['mapped']}\n"
            f"Commands without API: {comparison_results['unmapped']}\n"
            f"Coverage: {comparison_results['coverage_percent']:.1f}%"
        )
        
        summary_label = ttk.Label(summary_frame, text=summary_text, justify=tk.LEFT)
        summary_label.pack(anchor=tk.W)
        
        # Details frame
        details_frame = ttk.LabelFrame(main_frame, text="Details", padding=10)
        details_frame.pack(fill=tk.BOTH, expand=True)
        
        # Create text widget with scrollbar
        text_frame = ttk.Frame(details_frame)
        text_frame.pack(fill=tk.BOTH, expand=True)
        
        scrollbar = ttk.Scrollbar(text_frame)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        
        text_widget = tk.Text(
            text_frame,
            wrap=tk.WORD,
            yscrollcommand=scrollbar.set,
            font=('Consolas', 9),
            bg=self.theme_colors.get('bg_darker', '#252526'),
            fg=self.theme_colors.get('fg_light', '#e0e0e0'),
            insertbackground=self.theme_colors.get('fg_bright', '#ffffff'),
            selectbackground=self.theme_colors.get('accent', '#007acc'),
            selectforeground=self.theme_colors.get('fg_bright', '#ffffff')
        )
        text_widget.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar.config(command=text_widget.yview)
        
        # Populate details
        details = comparison_results['details']
        
        # Mapped commands
        if details['mapped']:
            text_widget.insert(tk.END, "✓ COMMANDS WITH API MAPPING:\n", 'header')
            text_widget.insert(tk.END, "=" * 80 + "\n\n")
            for cmd in sorted(details['mapped'], key=lambda x: x['command']):
                text_widget.insert(tk.END, f"  {cmd['command']}\n", 'success')
                text_widget.insert(tk.END, f"    → {cmd['api_function']}\n", 'api')
                text_widget.insert(tk.END, f"    Group: {cmd['group']}\n\n", 'info')
        
        # Unmapped commands
        if details['unmapped']:
            text_widget.insert(tk.END, "\n✗ COMMANDS WITHOUT API MAPPING:\n", 'header')
            text_widget.insert(tk.END, "=" * 80 + "\n\n")
            for cmd in sorted(details['unmapped'], key=lambda x: x['group']):
                text_widget.insert(tk.END, f"  {cmd['command']}\n", 'warning')
                text_widget.insert(tk.END, f"    Group: {cmd['group']}\n", 'info')
                if cmd.get('api_name'):
                    text_widget.insert(tk.END, f"    Expected API: {cmd['api_name']}\n", 'info')
                text_widget.insert(tk.END, "\n")
        
        # Configure tags
        text_widget.tag_config('header', foreground='#007acc', font=('Consolas', 10, 'bold'))
        text_widget.tag_config('success', foreground='#4ec9b0')
        text_widget.tag_config('warning', foreground='#dcdcaa')
        text_widget.tag_config('api', foreground='#9cdcfe')
        text_widget.tag_config('info', foreground='#808080')
        
        text_widget.config(state=tk.DISABLED)
        
        # Button frame
        btn_frame = ttk.Frame(main_frame)
        btn_frame.pack(fill=tk.X, pady=(10, 0))
        
        ttk.Button(btn_frame, text="Close", command=dialog.destroy, width=15).pack(side=tk.RIGHT)
        ttk.Button(btn_frame, text="Copy to Clipboard", command=lambda: self._copy_comparison_to_clipboard(comparison_results), width=20).pack(side=tk.RIGHT, padx=(0, 5))
        
        # Center and show dialog
        self._center_window(dialog, 900, 600)
        dialog.deiconify()
    
    def _compare_yaml_vs_api(self) -> Dict:
        """Compare YAML commands against available API functions"""
        if not self.yaml_parser:
            return {}
        
        mapped_commands = []
        unmapped_commands = []
        
        # Get all mappings
        mappings = at_to_api_mapper.get_all_mappings()
        
        # Check each YAML command
        for cmd_name in self.yaml_parser.commands.keys():
            cmd_info = self.yaml_parser.get_command_info(cmd_name)
            if not cmd_info:
                continue
            
            group = cmd_info.get('group', 'Unknown')
            api_name = cmd_info.get('api_name', '')
            
            if cmd_name in mappings:
                mapping = mappings[cmd_name]
                mapped_commands.append({
                    'command': cmd_name,
                    'api_function': mapping.api_function,
                    'group': group,
                    'api_name': api_name
                })
            else:
                unmapped_commands.append({
                    'command': cmd_name,
                    'group': group,
                    'api_name': api_name
                })
        
        total = len(mapped_commands) + len(unmapped_commands)
        coverage = (len(mapped_commands) / total * 100) if total > 0 else 0
        
        return {
            'total_yaml': total,
            'mapped': len(mapped_commands),
            'unmapped': len(unmapped_commands),
            'coverage_percent': coverage,
            'details': {
                'mapped': mapped_commands,
                'unmapped': unmapped_commands
            }
        }
    
    def _copy_comparison_to_clipboard(self, results: Dict):
        """Copy comparison results to clipboard"""
        lines = []
        lines.append(f"YAML vs API Comparison - {self.yaml_parser.product} v{self.yaml_parser.version}")
        lines.append("=" * 80)
        lines.append(f"Total Commands: {results['total_yaml']}")
        lines.append(f"Mapped: {results['mapped']}")
        lines.append(f"Unmapped: {results['unmapped']}")
        lines.append(f"Coverage: {results['coverage_percent']:.1f}%")
        lines.append("")
        
        if results['details']['mapped']:
            lines.append("COMMANDS WITH API MAPPING:")
            lines.append("-" * 80)
            for cmd in sorted(results['details']['mapped'], key=lambda x: x['command']):
                lines.append(f"  {cmd['command']} → {cmd['api_function']}")
            lines.append("")
        
        if results['details']['unmapped']:
            lines.append("COMMANDS WITHOUT API MAPPING:")
            lines.append("-" * 80)
            for cmd in sorted(results['details']['unmapped'], key=lambda x: x['group']):
                lines.append(f"  {cmd['command']} (Group: {cmd['group']})")
        
        text = "\n".join(lines)
        self.root.clipboard_clear()
        self.root.clipboard_append(text)
        messagebox.showinfo("Copied", "Comparison report copied to clipboard!")
    
    def _clear_settings(self):
        """Clear all saved settings"""
        if messagebox.askyesno(
            "Clear Settings",
            "This will clear all saved settings including:\n"
            "• Product configuration\n"
            "• Serial port settings\n"
            "• WiFi credentials\n"
            "• Bluetooth addresses\n"
            "• Window position\n\n"
            "Continue?",
            parent=self.root
        ):
            self.settings.clear()
            messagebox.showinfo("Settings Cleared", "All settings have been cleared.\nRestart the application to reset to defaults.")
    
    def _run_build(self, build_type: str):
        """Run build command using the log window"""
        # Get project root (parent of windows_gui)
        project_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
        
        # Determine build script and arguments based on platform
        if sys.platform == "win32":
            build_script = os.path.join(project_root, "build_scripts", "build_windows.cmd")
            shell = True
        else:
            # Linux/macOS
            build_script = os.path.join(project_root, "build_scripts", "build.sh")
            shell = False
        
        # Check if build script exists
        if not os.path.exists(build_script):
            messagebox.showerror(
                "Build Error",
                f"Build script not found:\n{build_script}",
                parent=self.root
            )
            return
        
        # Determine command arguments
        if build_type == "all":
            cmd_args = []
            build_name = "All (Release + Debug)"
        elif build_type == "release":
            cmd_args = ["release"]
            build_name = "Release"
        elif build_type == "debug":
            cmd_args = ["debug"]
            build_name = "Debug"
        elif build_type == "rebuild":
            cmd_args = ["rebuild"]
            build_name = "Rebuild All"
        elif build_type == "clean":
            cmd_args = ["clean"]
            build_name = "Clean"
        elif build_type == "cleanall":
            cmd_args = ["cleanall"]
            build_name = "Clean All"
        else:
            cmd_args = []
            build_name = "All"
        
        # Build command
        if sys.platform == "win32":
            command = [build_script] + cmd_args
        else:
            command = ["/bin/bash", build_script] + cmd_args
        
        # Switch to Log tab to show build output
        for i in range(self.notebook.index("end")):
            if self.notebook.tab(i, "text") == "Log":
                self.notebook.select(i)
                break
        
        # Clear log and add build header
        self._clear_log()
        self._log(f"{'='*60}")
        self._log(f"BUILD: {build_name}")
        self._log(f"{'='*60}")
        self._log(f"Command: {' '.join(command)}")
        self._log(f"Working directory: {project_root}")
        self._log(f"{'='*60}\n")
        
        def run_build_process():
            """Run build process in background thread"""
            try:
                # Start build process
                process = subprocess.Popen(
                    command,
                    cwd=project_root,
                    stdout=subprocess.PIPE,
                    stderr=subprocess.STDOUT,
                    universal_newlines=True,
                    shell=shell,
                    bufsize=1
                )
                
                # Read output line by line
                for line in process.stdout:
                    self._log(line.rstrip())
                
                # Wait for process to complete
                return_code = process.wait()
                
                # Log completion status
                self._log(f"\n{'='*60}")
                if return_code == 0:
                    self._log(f"✓ BUILD SUCCESSFUL: {build_name}")
                else:
                    self._log(f"✗ BUILD FAILED: {build_name} (exit code {return_code})")
                self._log(f"{'='*60}\n")
                
            except Exception as e:
                self._log(f"\n{'='*60}")
                self._log(f"✗ BUILD ERROR: {e}")
                self._log(f"{'='*60}\n")
        
        # Start build in background thread
        build_thread = threading.Thread(target=run_build_process, daemon=True)
        build_thread.start()
    
    def _show_about(self):
        """Show about dialog"""
        about_text = (
            f"ucxclient GUI v{UCXCLIENT_GUI_VERSION}\n\n"
            f"Dynamic GUI for u-connectXpress modules\n\n"
            f"Current Product: {self.yaml_parser.product if self.yaml_parser else 'None'}\n"
            f"Version: {self.yaml_parser.version if self.yaml_parser else 'None'}\n\n"
            f"© 2025 u-blox"
        )
        messagebox.showinfo("About", about_text, parent=self.root)
    
    def run(self):
        """Run application"""
        def on_closing():
            # Save window position
            self._save_window_position()
            
            # Disconnect if connected
            if self.connected:
                self._disconnect()
            
            # Save settings one final time
            self.settings.save()
            
            self.root.destroy()
        
        self.root.protocol("WM_DELETE_WINDOW", on_closing)
        self.root.mainloop()


if __name__ == "__main__":
    app = MainWindow()
    app.run()
