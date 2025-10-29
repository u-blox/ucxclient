"""
Product and version selector dialog
"""

import os
os.environ['PYTHONDONTWRITEBYTECODE'] = '1'

import tkinter as tk
from tkinter import ttk, messagebox, filedialog
from typing import Optional, Tuple

from yaml_parser import YAMLCommandParser


class ProductSelectorDialog:
    """Dialog for selecting product and version"""
    
    def __init__(self, parent, default_source='github', default_product='', default_version='', default_path=''):
        try:
            self.result: Optional[Tuple[str, str, str]] = None
            self.default_source = default_source
            self.default_product = default_product
            self.default_version = default_version
            self.default_path = default_path
            
            # Create dialog
            self.dialog = tk.Toplevel(parent)
            self.dialog.title("Select Product Configuration")
            self.dialog.geometry("500x350")
            
            # Create widgets
            self._create_widgets()
            
            # Setup dialog
            self.dialog.transient(parent)
            self.dialog.protocol("WM_DELETE_WINDOW", self._on_cancel)
            self._center_dialog()
        except Exception as e:
            print(f"[ERROR] ProductSelectorDialog __init__ failed: {e}")
            import traceback
            traceback.print_exc()
            raise
    
    def _center_dialog(self):
        """Center dialog on parent"""
        self.dialog.update_idletasks()
        parent = self.dialog.master
        
        if parent and parent.winfo_viewable():
            x = parent.winfo_x() + (parent.winfo_width() // 2) - (self.dialog.winfo_width() // 2)
            y = parent.winfo_y() + (parent.winfo_height() // 2) - (self.dialog.winfo_height() // 2)
            self.dialog.geometry(f"+{x}+{y}")
    
    def _create_widgets(self):
        """Create dialog widgets"""
        # Main frame
        main_frame = ttk.Frame(self.dialog, padding="20")
        main_frame.pack(fill=tk.BOTH, expand=True)
        
        # Title
        ttk.Label(
            main_frame,
            text="Load AT Command Configuration",
            font=('TkDefaultFont', 12, 'bold')
        ).pack(pady=(0, 20))
        
        # Source selection
        source_frame = ttk.LabelFrame(main_frame, text="Configuration Source", padding="10")
        source_frame.pack(fill=tk.X, pady=(0, 15))
        
        self.source_var = tk.StringVar(value=self.default_source)
        
        ttk.Radiobutton(
            source_frame,
            text="Load from GitHub (u-connectXpress repository)",
            variable=self.source_var,
            value="github",
            command=self._on_source_changed
        ).pack(anchor=tk.W, pady=2)
        
        ttk.Radiobutton(
            source_frame,
            text="Load from local file",
            variable=self.source_var,
            value="local",
            command=self._on_source_changed
        ).pack(anchor=tk.W, pady=2)
        
        # GitHub frame
        self.github_frame = ttk.Frame(main_frame)
        self.github_frame.pack(fill=tk.X, pady=(0, 10))
        
        ttk.Label(self.github_frame, text="Product:").grid(row=0, column=0, sticky=tk.W, pady=5)
        self.product_var = tk.StringVar()
        fallback_products = ['NORA-W36', 'NORA-B26', 'NORA-W46', 'NORA-W56']
        self.product_combo = ttk.Combobox(
            self.github_frame,
            textvariable=self.product_var,
            values=fallback_products,
            state="readonly",
            width=30
        )
        self.product_combo.grid(row=0, column=1, sticky=tk.W, padx=(10, 0), pady=5)
        self.product_combo.bind('<<ComboboxSelected>>', self._on_product_changed)
        
        ttk.Label(self.github_frame, text="Version:").grid(row=1, column=0, sticky=tk.W, pady=5)
        self.version_var = tk.StringVar()
        self.version_combo = ttk.Combobox(
            self.github_frame,
            textvariable=self.version_var,
            values=[],
            state="readonly",
            width=30
        )
        self.version_combo.grid(row=1, column=1, sticky=tk.W, padx=(10, 0), pady=5)
        
        # Set default product
        if self.default_product and self.default_product in fallback_products:
            self.product_var.set(self.default_product)
        elif fallback_products:
            self.product_var.set(fallback_products[0])
        
        self._initial_version_set = False
        self._products_loaded = False
        
        # Local file frame
        self.local_frame = ttk.Frame(main_frame)
        
        ttk.Label(self.local_frame, text="YAML File:").grid(row=0, column=0, sticky=tk.W, pady=5)
        self.filepath_var = tk.StringVar(value=self.default_path)
        ttk.Entry(
            self.local_frame,
            textvariable=self.filepath_var,
            width=35
        ).grid(row=0, column=1, sticky=tk.W, padx=(10, 5), pady=5)
        
        ttk.Button(
            self.local_frame,
            text="Browse...",
            command=self._browse_file
        ).grid(row=0, column=2, pady=5)
        
        # Info label
        self.info_label = ttk.Label(
            main_frame,
            text="Select a product and version to load AT command definitions from GitHub.",
            wraplength=450,
            foreground="gray"
        )
        self.info_label.pack(pady=15)
        
        # Buttons
        button_frame = ttk.Frame(main_frame)
        button_frame.pack(fill=tk.X, pady=(10, 0))
        
        ttk.Button(button_frame, text="Load", command=self._on_load, width=15).pack(side=tk.RIGHT, padx=(0, 5))
        ttk.Button(button_frame, text="Cancel", command=self._on_cancel, width=15).pack(side=tk.RIGHT)
        
        # Initialize visibility
        self._on_source_changed()
        
        # Keyboard shortcuts
        self.dialog.bind('<Return>', lambda e: self._on_load())
        self.dialog.bind('<Escape>', lambda e: self._on_cancel())
        
        # Load versions for the default product after everything is set up
        if self.product_var.get():
            self._on_product_changed()
    
    def _on_source_changed(self):
        """Handle source selection change"""
        source = self.source_var.get()
        
        if source == "github":
            self.github_frame.pack(fill=tk.X, pady=(0, 10))
            self.local_frame.pack_forget()
            self.info_label.config(text="Select a product and version to load AT command definitions from GitHub.")
        else:
            self.github_frame.pack_forget()
            self.local_frame.pack(fill=tk.X, pady=(0, 10))
            self.info_label.config(text="Select a local YAML file containing AT command definitions.")
    
    def _on_product_changed(self, event=None):
        """Handle product selection change"""
        product = self.product_var.get()
        if not product:
            return
        
        # Load versions for selected product
        try:
            versions = YAMLCommandParser.get_versions_for_product(product)
        except Exception as e:
            print(f"[ERROR] Failed to get versions for {product}: {e}")
            versions = []
        
        if versions:
            self.version_combo['values'] = versions
            if self.default_version and self.default_version in versions and not self._initial_version_set:
                self.version_var.set(self.default_version)
                self._initial_version_set = True
            elif versions:
                self.version_var.set(versions[0])
        else:
            self.version_combo['values'] = []
            self.version_var.set('')
            # Only show warning if this was triggered by user interaction (event is not None)
            # Don't show warning during initial dialog setup
            if event is not None:
                try:
                    messagebox.showwarning(
                        "No Versions",
                        f"Could not fetch versions for {product}.\nCheck your internet connection or try a local file."
                    )
                except Exception as e:
                    print(f"[ERROR] Failed to show warning dialog: {e}")
    
    def _browse_file(self):
        """Browse for local YAML file"""
        filename = filedialog.askopenfilename(
            title="Select YAML Configuration File",
            filetypes=[("YAML files", "*.yaml *.yml"), ("All files", "*.*")],
            initialdir=os.path.dirname(self.filepath_var.get()) if self.filepath_var.get() else None
        )
        if filename:
            self.filepath_var.set(filename)
    
    def _on_load(self):
        """Handle Load button"""
        source = self.source_var.get()
        
        if source == "github":
            product = self.product_var.get()
            version = self.version_var.get()
            
            if not product or not version:
                messagebox.showerror("Error", "Please select both product and version.")
                return
            
            self.result = ('github', product, version)
        else:
            filepath = self.filepath_var.get()
            
            if not filepath or not os.path.exists(filepath):
                messagebox.showerror("Error", "Please select a valid YAML file.")
                return
            
            self.result = ('local', '', filepath)
        
        self.dialog.destroy()
    
    def _on_cancel(self):
        """Handle Cancel button"""
        self.result = None
        self.dialog.destroy()
    
    def show(self):
        """Show dialog and wait for result"""
        self.dialog.wait_window()
        return self.result
