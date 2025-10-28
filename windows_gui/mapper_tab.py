"""
AT Command to API Mapper Tab
Shows all command mappings with search functionality
"""

import os
os.environ['PYTHONDONTWRITEBYTECODE'] = '1'

import sys
import tkinter as tk
from tkinter import ttk


class MapperTab:
    """Tab showing AT command to API function mappings"""
    
    def __init__(self, parent_notebook: ttk.Notebook, yaml_parser, at_to_api_mapper, theme_colors=None, ucx_dll=None, main_window=None):
        """Initialize mapper tab"""
        self.parent_notebook = parent_notebook
        self.yaml_parser = yaml_parser
        self.mapper = at_to_api_mapper
        self.theme_colors = theme_colors or {}
        self.ucx_dll = ucx_dll  # DLL reference to check if functions exist
        self.main_window = main_window  # Reference to main window to get DLL later
        self.parent = parent_notebook  # For after() calls
        
        # Create the tab
        self.frame = ttk.Frame(parent_notebook)
        parent_notebook.add(self.frame, text="🗺️ Mapper")
        
        try:
            self._create_ui()
            # Load mappings with significant delay to allow GC to settle after DLL initialization
            print("[MAPPER] Scheduling mapping load with 500ms delay...")
            parent_notebook.after(500, self._safe_load_mappings)
        except Exception as e:
            print(f"[MAPPER] FATAL ERROR in __init__: {e}")
            import traceback
            traceback.print_exc()
            # Show error in tab
            error_label = ttk.Label(self.frame, text=f"Failed to initialize mapper tab:\n{e}", foreground="red")
            error_label.pack(expand=True, fill='both', padx=20, pady=20)
    
    def _safe_load_mappings(self):
        """Wrapper for _load_mappings with exception handling"""
        try:
            self._load_mappings()
        except Exception as e:
            print(f"[MAPPER] ERROR loading mappings: {e}")
            import traceback
            traceback.print_exc()
            # Try to show error in UI
            try:
                error_msg = f"Failed to load mappings: {e}"
                if hasattr(self, 'tree'):
                    self.tree.insert('', 'end', values=("ERROR", error_msg, "", "", ""))
            except:
                pass

    
    def _create_ui(self):
        """Create the mapper UI"""
        # Search bar at top
        search_frame = ttk.Frame(self.frame)
        search_frame.pack(fill=tk.X, padx=10, pady=10)
        
        ttk.Label(search_frame, text="Search:", 
                 font=('Segoe UI', 10, 'bold')).pack(side=tk.LEFT, padx=5)
        
        self.search_var = tk.StringVar()
        self.search_var.trace('w', self._on_search)
        
        search_entry = ttk.Entry(search_frame, textvariable=self.search_var, width=40)
        search_entry.pack(side=tk.LEFT, padx=5, fill=tk.X, expand=True)
        
        ttk.Label(search_frame, text="Search AT command or API function", 
                 foreground=self.theme_colors.get('fg_dim', '#808080'),
                 font=('Segoe UI', 8)).pack(side=tk.LEFT, padx=5)
        
        # Stats frame below search
        stats_frame = ttk.Frame(self.frame)
        stats_frame.pack(fill=tk.X, padx=10, pady=(0, 5))
        
        self.stats_label = ttk.Label(stats_frame, text="", 
                                     font=('Segoe UI', 9, 'bold'),
                                     foreground=self.theme_colors.get('fg', '#FFFFFF'))
        self.stats_label.pack(side=tk.LEFT)
        
        # Buttons at top (after search bar)
        btn_frame = ttk.Frame(self.frame)
        btn_frame.pack(fill=tk.X, padx=10, pady=(5, 10))
        
        ttk.Button(btn_frame, text="🔄 Refresh", 
                  command=self._load_mappings).pack(side=tk.LEFT, padx=5)
        
        ttk.Button(btn_frame, text="📋 Copy Selected", 
                  command=self._copy_selected).pack(side=tk.LEFT, padx=5)
        
        # Legend for color coding (on same line)
        legend_frame = ttk.Frame(btn_frame)
        legend_frame.pack(side=tk.RIGHT, padx=10)
        
        ttk.Label(legend_frame, text="Status: ", 
                 font=('Segoe UI', 8, 'bold')).pack(side=tk.LEFT, padx=2)
        ttk.Label(legend_frame, text="Found (green)", 
                 foreground='#228B22', font=('Segoe UI', 8)).pack(side=tk.LEFT, padx=5)
        ttk.Label(legend_frame, text="Not Found (orange)", 
                 foreground='#FF8C00', font=('Segoe UI', 8)).pack(side=tk.LEFT, padx=5)
        
        # Treeview for mappings
        tree_frame = ttk.Frame(self.frame)
        tree_frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=(0, 10))
        
        # Create treeview with columns
        columns = ('AT Command', 'AT API Name', 'UCX API Function', 'Status', 'Type')
        self.tree = ttk.Treeview(tree_frame, columns=columns, show='headings', height=25)
        
        # Configure columns
        self.tree.heading('AT Command', text='AT Command')
        self.tree.heading('AT API Name', text='AT API Name')
        self.tree.heading('UCX API Function', text='UCX API Function')
        self.tree.heading('Status', text='Status')
        self.tree.heading('Type', text='Type')
        
        self.tree.column('AT Command', width=140)
        self.tree.column('AT API Name', width=200)
        self.tree.column('UCX API Function', width=280)
        self.tree.column('Status', width=100)
        self.tree.column('Type', width=100)
        
        # Configure tags for color coding
        self.tree.tag_configure('pending', foreground='#FF8C00')  # Orange for pending
        self.tree.tag_configure('error', foreground='#DC143C')    # Red for errors
        self.tree.tag_configure('ok', foreground='#228B22')       # Green for implemented
        
        # Scrollbars
        vsb = ttk.Scrollbar(tree_frame, orient=tk.VERTICAL, command=self.tree.yview)
        hsb = ttk.Scrollbar(tree_frame, orient=tk.HORIZONTAL, command=self.tree.xview)
        self.tree.configure(yscrollcommand=vsb.set, xscrollcommand=hsb.set)
        
        # Pack tree and scrollbars
        self.tree.grid(row=0, column=0, sticky='nsew')
        vsb.grid(row=0, column=1, sticky='ns')
        hsb.grid(row=1, column=0, sticky='ew')
        
        tree_frame.grid_rowconfigure(0, weight=1)
        tree_frame.grid_columnconfigure(0, weight=1)
    
    def _get_dll_exported_functions(self):
        """Get set of all functions exported from the DLL"""
        import os
        
        script_dir = os.path.dirname(os.path.abspath(__file__))
        exports_file = os.path.join(script_dir, 'dll_exports.txt')
        
        # If exports file doesn't exist, try to generate it
        if not os.path.exists(exports_file):
            print(f"[MAPPER] Generating DLL exports list...")
            try:
                import sys
                sys.path.insert(0, script_dir)
                from extract_dll_functions import get_dll_exports_from_file
                from ucx_wrapper import UcxClientWrapper
                
                wrapper = UcxClientWrapper()
                dll_path = wrapper._dll._name
                exports = get_dll_exports_from_file(dll_path)
                
                with open(exports_file, 'w') as f:
                    for func in sorted(exports):
                        f.write(func + '\n')
                
                print(f"[MAPPER] Generated {len(exports)} function exports")
            except Exception as e:
                print(f"[MAPPER] Warning: Could not generate exports: {e}")
                return set()
        
        # Load exports from file
        exported_functions = set()
        try:
            with open(exports_file, 'r') as f:
                for line in f:
                    func_name = line.strip()
                    if func_name:
                        exported_functions.add(func_name)
            print(f"[MAPPER] Loaded {len(exported_functions)} exported functions from DLL")
        except Exception as e:
            print(f"[MAPPER] Warning: Could not load exports file: {e}")
        
        return exported_functions
    
    def _load_mappings(self):
        """Load all command mappings into the tree"""
        # Clear existing items
        for item in self.tree.get_children():
            self.tree.delete(item)
        
        if not self.yaml_parser or not self.yaml_parser.commands:
            return
        
        # Get set of all functions exported from the DLL
        self.dll_exports = self._get_dll_exported_functions()
        
        # Get all commands
        self.all_mappings = []
        pending_count = 0
        implemented_count = 0
        
        print(f"[MAPPER] Loading {len(self.yaml_parser.commands)} commands...")
        
        cmd_count = 0
        for cmd_name, cmd_info in self.yaml_parser.commands.items():
            try:
                cmd_count += 1
                if cmd_count % 5 == 0 or cmd_count > 180:  # More frequent reporting near the end
                    print(f"[MAPPER] Processing command {cmd_count}/{len(self.yaml_parser.commands)}: {cmd_name}")
                
                at_cmd = cmd_name  # The key is the AT command (e.g., "AT+UWSC")
                api_name = cmd_info.get('api_name', '')  # AT API name from YAML (e.g., "StationConnect")
                group = cmd_info.get('group', '')
                
                # Get the full API function name using the mapper
                try:
                    # Check if api_name is empty or None - these are not implemented
                    if not api_name or api_name.lower() in ['none', '']:
                        at_api_name = "(none)"
                        ucx_api_function = "(not implemented)"
                        status = "Not Found"
                        type_str = "-"
                        tag = 'pending'
                        pending_count += 1
                    else:
                        at_api_name = api_name
                        
                        # Use the mapper's internal method to convert API name to function
                        ucx_api_function = self.mapper._convert_api_name_to_function(cmd_name, api_name, cmd_info)
                        
                        # Check if this function actually exists in the DLL
                        # The DLL may have numbered variants (e.g., uCxFoo1, uCxFoo2)
                        # or Begin suffix (e.g., uCxFooBegin)
                        function_exists = False
                        found_variants = []
                        
                        # Check exact match first
                        if ucx_api_function in self.dll_exports:
                            function_exists = True
                            found_variants.append(ucx_api_function)
                        
                        # Check with "Begin" suffix for BEGIN_END pattern
                        if f"{ucx_api_function}Begin" in self.dll_exports:
                            function_exists = True
                            if f"{ucx_api_function}Begin" not in found_variants:
                                found_variants.append(f"{ucx_api_function}Begin")
                        
                        # Check for numbered variants (uCxFoo1, uCxFoo2, etc.)
                        for export in self.dll_exports:
                            # If export starts with our function name and ends with a digit
                            if export.startswith(ucx_api_function) and len(export) > len(ucx_api_function):
                                remainder = export[len(ucx_api_function):]
                                # Check for patterns: Foo1, Foo2 or FooBegin1, FooBegin2
                                if remainder.isdigit():
                                    function_exists = True
                                    found_variants.append(export)
                                elif remainder.startswith('Begin'):
                                    after_begin = remainder[5:]
                                    if not after_begin or after_begin.isdigit():
                                        function_exists = True
                                        found_variants.append(export)
                        
                        if not function_exists:
                            # Function has api_name in YAML but doesn't exist in DLL
                            status = "Not Found"
                            type_str = "-"
                            tag = 'pending'
                            pending_count += 1
                        else:
                            # Function exists in DLL - it's implemented!
                            status = "Found"
                            tag = 'ok'
                            
                            # Show variants if multiple exist
                            if len(found_variants) > 1:
                                ucx_api_function = f"{ucx_api_function} ({', '.join([v.replace(ucx_api_function, '') for v in found_variants])})"
                            elif len(found_variants) == 1 and found_variants[0] != ucx_api_function:
                                ucx_api_function = found_variants[0]
                            
                            # Determine type (SIMPLE or BEGIN_END)
                            from at_to_api_mapper import APICallType
                            call_type = self.mapper._determine_api_call_type(cmd_info)
                            type_str = "SIMPLE" if call_type == APICallType.SIMPLE else "BEGIN_END"
                            implemented_count += 1
                    
                except Exception as e:
                    print(f"[MAPPER ERROR] Processing {cmd_name}: {e}")
                    import traceback
                    traceback.print_exc()
                    at_api_name = api_name if api_name else "(error)"
                    ucx_api_function = f"Error: {str(e)[:50]}"
                    status = "Error"
                    type_str = "ERROR"
                    tag = 'error'
                
                mapping = {
                'at_cmd': at_cmd,
                'at_api_name': at_api_name,
                'ucx_api_function': ucx_api_function,
                'status': status,
                    'type': type_str,
                    'tag': tag
                }
                
                self.all_mappings.append(mapping)
                
            except Exception as outer_error:
                print(f"[MAPPER FATAL] Error processing command {cmd_name}: {outer_error}")
                import traceback
                traceback.print_exc()
        
        # Print summary
        total = len(self.all_mappings)
        print(f"[MAPPER] ✓ Loaded {total} commands: {implemented_count} implemented (green), {pending_count} pending (orange)")
        
        # DON'T populate tree automatically - too risky with GC after DLL operations
        # Tree will be populated on first view or manually
        print(f"[MAPPER] Mapping data ready. Tree will populate on demand to avoid crashes.")
        self.tree_populated = False
        
        # Update stats with data we have
        self._update_stats(len(self.all_mappings), len(self.all_mappings))
        
        # Add a button to manually populate if needed
        self._add_populate_button()
    
    def _populate_tree(self):
        """Populate the tree widget with all mappings (deferred to avoid GC crashes)"""
        try:
            import gc
            # Force GC to complete NOW before we touch Tkinter widgets
            # This prevents GC from running during Tkinter operations
            print(f"[MAPPER] Running GC collection before tree operations...")
            gc.collect()
            print(f"[MAPPER] GC complete, now populating tree with {len(self.all_mappings)} commands...")
            
            for mapping in self.all_mappings:
                try:
                    self.tree.insert('', 'end', values=(
                        mapping['at_cmd'],
                        mapping['at_api_name'],
                        mapping['ucx_api_function'],
                        mapping['status'],
                        mapping['type']
                    ), tags=(mapping['tag'],))
                except Exception as tree_error:
                    print(f"[MAPPER ERROR] Failed to insert command {mapping['at_cmd']} into tree: {tree_error}")
            
            print(f"[MAPPER] ✓ Tree populated successfully")
            
            # Update stats after tree is populated
            self._update_stats(len(self.all_mappings), len(self.all_mappings))
        except Exception as e:
            print(f"[MAPPER ERROR] Failed to populate tree: {e}")
            import traceback
            traceback.print_exc()
    
    def _on_search(self, *args):
        """Handle search input"""
        search_text = self.search_var.get().lower()
        
        # Clear tree
        for item in self.tree.get_children():
            self.tree.delete(item)
        
        # Filter and add matching items
        shown = 0
        for mapping in self.all_mappings:
            at_cmd = mapping['at_cmd'].lower()
            at_api_name = mapping['at_api_name'].lower()
            ucx_func = mapping['ucx_api_function'].lower()
            status = mapping.get('status', '').lower()
            
            # Check if search matches
            if (not search_text or 
                search_text in at_cmd or 
                search_text in at_api_name or
                search_text in ucx_func or
                search_text in status):
                
                self.tree.insert('', 'end', values=(
                    mapping['at_cmd'],
                    mapping['at_api_name'],
                    mapping['ucx_api_function'],
                    mapping['status'],
                    mapping['type']
                ), tags=(mapping.get('tag', 'ok'),))
                shown += 1
        
        # Update stats
        self._update_stats(shown, len(self.all_mappings))
    
    def _update_stats(self, shown, total):
        """Update statistics label with breakdown"""
        # Count pending vs implemented in all_mappings
        pending = sum(1 for m in self.all_mappings if m.get('tag') == 'pending')
        implemented = total - pending
        
        # Calculate percentage
        percentage = (implemented / total * 100) if total > 0 else 0
        
        if shown == total:
            self.stats_label.config(text=f"Total: {total} commands  |  ✓ Implemented: {implemented} ({percentage:.1f}%)  |  ⚠ Pending: {pending}")
        else:
            self.stats_label.config(text=f"Showing: {shown}/{total} commands  |  ✓ Implemented: {implemented} ({percentage:.1f}%)  |  ⚠ Pending: {pending}")
    
    def _copy_selected(self):
        """Copy complete selected line to clipboard (all columns tab-separated)"""
        selection = self.tree.selection()
        if not selection:
            return
        
        item = self.tree.item(selection[0])
        values = item['values']
        if values:
            # Copy all columns as tab-separated values for easy pasting into Excel/docs
            # Format: AT Command \t AT API Name \t UCX API Function \t Status \t Type
            complete_line = '\t'.join(str(v) for v in values)
            
            self.frame.clipboard_clear()
            self.frame.clipboard_append(complete_line)
            
            # Visual feedback - show first part
            at_cmd = values[0]
            status = values[3]
            self.stats_label.config(text=f"✓ Copied: {at_cmd} [{status}]")
            self.frame.after(2000, lambda: self._update_stats(len(self.tree.get_children()), len(self.all_mappings)))
    
    def _update_dll_reference(self):
        """Update DLL reference from main window and reload mappings"""
        print(f"[MAPPER] Refresh button clicked - reloading command mappings...")
        self.stats_label.config(text="🔄 Refreshing...")
        self.frame.update()
        
        self._load_mappings()
        
        # Show completion message briefly
        self.stats_label.config(text="✓ Refresh complete!")
        self.frame.after(2000, lambda: self._update_stats(len(self.tree.get_children()), len(self.all_mappings)))
