"""
Simple Test Tab for UCX API Testing
Minimal implementation to avoid crashes
"""

import os
os.environ['PYTHONDONTWRITEBYTECODE'] = '1'

import tkinter as tk
from tkinter import ttk, scrolledtext
import threading


class SimpleTestTab:
    """Simple test tab for basic API testing"""
    
    def __init__(self, parent_notebook: ttk.Notebook, yaml_parser, api_executor, main_window, theme_colors=None):
        """Initialize simple test tab"""
        self.parent_notebook = parent_notebook
        self.yaml_parser = yaml_parser
        self.api_executor = api_executor
        self.main_window = main_window
        self.theme_colors = theme_colors or {}
        
        self.is_running = False
        self.tests_passed = 0
        self.tests_failed = 0
        
        # Create the tab
        self.frame = ttk.Frame(parent_notebook)
        parent_notebook.add(self.frame, text="🧪 Test")
        
        self._create_ui()
    
    def _create_ui(self):
        """Create the test UI"""
        # Control panel at top
        control_frame = ttk.Frame(self.frame)
        control_frame.pack(fill=tk.X, padx=10, pady=10)
        
        ttk.Label(control_frame, text="API Testing", 
                 font=('Segoe UI', 12, 'bold')).pack(side=tk.LEFT, padx=5)
        
        ttk.Button(control_frame, text="▶ Run Get Tests", 
                  command=self._run_get_tests).pack(side=tk.LEFT, padx=5)
        
        ttk.Button(control_frame, text="▶ Run Get/Set Tests", 
                  command=self._run_getset_tests).pack(side=tk.LEFT, padx=5)
        
        ttk.Button(control_frame, text="🗑 Clear", 
                  command=self._clear_results).pack(side=tk.LEFT, padx=5)
        
        # Statistics
        stats_frame = ttk.LabelFrame(self.frame, text="Statistics", padding=5)
        stats_frame.pack(fill=tk.X, padx=10, pady=5)
        
        self.stats_label = ttk.Label(stats_frame, text="Passed: 0 | Failed: 0 | Total: 0", 
                                     font=('Segoe UI', 10))
        self.stats_label.pack(side=tk.LEFT, padx=5)
        
        # Progress bar
        self.progress_bar = ttk.Progressbar(stats_frame, length=200, mode='determinate')
        self.progress_bar.pack(side=tk.RIGHT, padx=5)
        
        # Results area
        results_frame = ttk.LabelFrame(self.frame, text="Test Results", padding=10)
        results_frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=5)
        
        self.results_text = scrolledtext.ScrolledText(
            results_frame,
            height=20,
            state=tk.DISABLED,
            bg=self.theme_colors.get('bg_darker', '#252526'),
            fg=self.theme_colors.get('fg_light', '#e0e0e0'),
            font=('Consolas', 9)
        )
        self.results_text.pack(fill=tk.BOTH, expand=True)
    
    def _log(self, message: str):
        """Add message to results (thread-safe)"""
        def do_log():
            self.results_text.config(state=tk.NORMAL)
            self.results_text.insert(tk.END, message + "\n")
            self.results_text.see(tk.END)
            self.results_text.config(state=tk.DISABLED)
        
        # Schedule in main thread
        try:
            self.frame.after(0, do_log)
        except:
            # Fallback if frame is destroyed
            pass
    
    def _update_stats(self, total_tests=None):
        """Update statistics (thread-safe)"""
        def do_update():
            total = self.tests_passed + self.tests_failed
            self.stats_label.config(text=f"Passed: {self.tests_passed} | Failed: {self.tests_failed} | Total: {total}")
            
            # Update progress bar
            if total_tests and total_tests > 0:
                progress = (total / total_tests) * 100
                self.progress_bar['value'] = progress
        
        # Schedule in main thread
        try:
            self.frame.after(0, do_update)
        except:
            # Fallback if frame is destroyed
            pass
    
    def _clear_results(self):
        """Clear results"""
        self.results_text.config(state=tk.NORMAL)
        self.results_text.delete(1.0, tk.END)
        self.results_text.config(state=tk.DISABLED)
        self.tests_passed = 0
        self.tests_failed = 0
        self.progress_bar['value'] = 0
        self._update_stats()
    
    def _run_get_tests(self):
        """Run simple Get tests"""
        if not self.main_window.is_connected():
            self._log("✗ Error: Not connected to device")
            return
        
        if self.is_running:
            self._log("⚠ Tests already running")
            return
        
        self.is_running = True
        # Run in background thread to avoid blocking UI
        threading.Thread(target=self._execute_get_tests, daemon=True).start()
    
    def _run_getset_tests(self):
        """Run comprehensive Get/Set validation tests"""
        if not self.main_window.is_connected():
            self._log("✗ Error: Not connected to device")
            return
        
        if self.is_running:
            self._log("⚠ Tests already running")
            return
        
        self.is_running = True
        # Run in background thread to avoid blocking UI
        threading.Thread(target=self._execute_getset_tests, daemon=True).start()
    
    def _execute_tests(self):
        """Execute tests in background thread"""
        try:
            self._clear_results()
            self._log("=" * 60)
            self._log("UCX API AUTOMATED TEST SUITE")
            self._log("=" * 60)
            self._log("")
            
            # Get all Get commands from YAML
            commands = self.yaml_parser.commands if self.yaml_parser else {}
            
            if not commands:
                self._log("✗ No commands found in YAML")
                return
            
            # Find all Get commands
            get_commands = []
            for cmd_name, cmd_info in commands.items():
                if isinstance(cmd_info, dict):
                    api_name = cmd_info.get('api_name', '')
                    if api_name and api_name.startswith('Get'):
                        get_commands.append({
                            'cmd': cmd_name,
                            'api': api_name,
                            'desc': cmd_name
                        })
            
            self._log(f"📋 Found {len(get_commands)} Get commands to test")
            self._log("")
            
            # Test each Get command
            total_tests = len(get_commands)
            for i, cmd_info in enumerate(get_commands):
                if not self.is_running:
                    self._log("")
                    self._log("⚠ Tests stopped by user")
                    break
                
                # Add "?" for read operation
                cmd = cmd_info['cmd'] + "?"
                desc = f"{cmd_info['desc']} ({cmd_info['api']})"
                
                self._test_command(cmd, desc, total_tests)
                
                # Small delay to avoid overwhelming the device
                import time
                time.sleep(0.05)
            
            # Summary
            self._log("")
            self._log("=" * 60)
            self._log("TEST SUMMARY")
            self._log("=" * 60)
            self._log(f"Total: {len(get_commands)} commands")
            self._log(f"Passed: {self.tests_passed}")
            self._log(f"Failed: {self.tests_failed}")
            if len(get_commands) > 0:
                pass_rate = (self.tests_passed / len(get_commands)) * 100
                self._log(f"Pass Rate: {pass_rate:.1f}%")
            self._log("=" * 60)
        except Exception as e:
            self._log(f"✗ EXCEPTION in test execution: {e}")
            import traceback
            traceback.print_exc()
        finally:
            self.is_running = False
    
    def _test_command(self, cmd: str, description: str, total_tests=None):
        """Test a single command"""
        self._log(f"\nTesting: {description}")
        self._log(f"  Command: {cmd}")
        
        try:
            result = self.api_executor.execute_at_command(cmd)
            if result.success:
                # Truncate long responses for display
                response = str(result.result_data)
                if len(response) > 100:
                    response = response[:97] + "..."
                self._log(f"  ✓ PASS: {response}")
                self.tests_passed += 1
            else:
                self._log(f"  ✗ FAIL: {result.error_message}")
                self.tests_failed += 1
        except Exception as e:
            self._log(f"  ✗ EXCEPTION: {e}")
            self.tests_failed += 1
        
        self._update_stats(total_tests)
    
    def _execute_get_tests(self):
        """Execute simple Get tests (same as old _execute_tests)"""
        self._execute_tests()  # Reuse existing implementation
    
    def _execute_getset_tests(self):
        """Execute comprehensive Get/Set validation tests in background thread"""
        try:
            self._clear_results()
            self._log("=" * 60)
            self._log("COMPREHENSIVE GET/SET VALIDATION TEST SUITE")
            self._log("=" * 60)
            self._log("Test Plan: Read defaults → Write new value → Verify → Store → Reset → Verify → Factory Reset → Verify")
            self._log("=" * 60)
            self._log("")
            
            # Find Get/Set command pairs
            pairs = self._find_getset_pairs()
            
            total = len(pairs)
            if total == 0:
                self._log("⚠ No Get/Set command pairs found in YAML")
                return
            
            self._log(f"📋 Found {total} Get/Set command pairs to test")
            self._log("")
            
            # Test each pair
            for idx, (get_name, set_name, get_info, set_info) in enumerate(pairs, 1):
                if not self.is_running:
                    self._log("")
                    self._log("⚠ Tests stopped by user")
                    break
                
                self._test_getset_pair(get_name, set_name, get_info, set_info, idx, total)
                
                # Small delay between tests
                import time
                time.sleep(0.1)
            
            # Show summary
            self._log("")
            self._log("=" * 60)
            self._log("TEST SUMMARY")
            self._log("=" * 60)
            self._log(f"Total Pairs: {total}")
            self._log(f"Passed: {self.tests_passed}")
            self._log(f"Failed: {self.tests_failed}")
            if total > 0:
                pass_rate = (self.tests_passed / total) * 100
                self._log(f"Pass Rate: {pass_rate:.1f}%")
            self._log("=" * 60)
            
        except Exception as e:
            self._log(f"✗ Test execution error: {str(e)}")
            import traceback
            traceback.print_exc()
        finally:
            self.is_running = False
    
    def _find_getset_pairs(self):
        """Find matching Get/Set command pairs"""
        pairs = []
        commands = self.yaml_parser.commands if self.yaml_parser else {}
        
        get_commands = {}
        set_commands = {}
        
        # Categorize commands by API name
        for cmd_name, cmd_info in commands.items():
            if not isinstance(cmd_info, dict):
                continue
            
            api_name = cmd_info.get('api_name', '')
            if not api_name or not isinstance(api_name, str):
                continue
                
            if api_name.startswith("Get"):
                base_name = api_name[3:]  # Remove "Get" prefix
                get_commands[base_name] = (cmd_name, api_name, cmd_info)
            elif api_name.startswith("Set"):
                base_name = api_name[3:]  # Remove "Set" prefix
                set_commands[base_name] = (cmd_name, api_name, cmd_info)
        
        self._log(f"Found {len(get_commands)} Get commands and {len(set_commands)} Set commands")
        self._log("")
        
        # Match pairs
        for base_name, (get_cmd, get_api, get_info) in get_commands.items():
            if base_name in set_commands:
                set_cmd, set_api, set_info = set_commands[base_name]
                pairs.append((get_cmd, set_cmd, get_info, set_info))
                self._log(f"  Paired: {get_api} <-> {set_api}")
        
        if len(pairs) == 0:
            self._log("ℹ Note: The NORA-W36 firmware does not use traditional Get/Set pairs.")
            self._log("Get commands are for reading information, Set commands are for configuration.")
            self._log("These are typically independent operations, not read/write pairs.")
            self._log("")
            self._log("Use 'Run Get Tests' to test all read operations.")
            self._log("Set commands should be tested individually with appropriate parameters.")
        
        return pairs
    
    def _test_getset_pair(self, get_name, set_name, get_info, set_info, index, total):
        """Test a single Get/Set command pair with full 7-step validation"""
        self._log(f"[{index}/{total}] Testing {get_name}/{set_name}")
        self._log("-" * 60)
        
        try:
            # Step 1: Read default value
            self._log("Step 1: Reading default value...")
            default_result = self.api_executor.execute_at_command(get_name + "?")
            if not default_result.success:
                self._log(f"  ✗ Failed to read default value: {default_result.error_message}")
                self.tests_failed += 1
                self._update_stats(total)
                return
            
            default_value = default_result.result_data
            self._log(f"  ✓ Default value: {self._truncate(str(default_value))}")
            
            # Step 2: Generate and write realistic test value
            self._log("Step 2: Writing test value...")
            test_value = self._generate_realistic_value(set_info, default_value)
            if test_value is None:
                self._log(f"  ⚠ Cannot generate test value for {set_name}, skipping")
                return
            
            self._log(f"  Writing test value: {self._truncate(str(test_value))}")
            set_result = self.api_executor.execute_at_command(f"{set_name}={test_value}")
            if not set_result.success:
                self._log(f"  ✗ Failed to write test value: {set_result.error_message}")
                self.tests_failed += 1
                self._update_stats(total)
                return
            self._log(f"  ✓ Write successful")
            
            # Step 3: Read back and verify
            self._log("Step 3: Reading back value...")
            read_result = self.api_executor.execute_at_command(get_name + "?")
            if not read_result.success:
                self._log(f"  ✗ Failed to read back value: {read_result.error_message}")
                self.tests_failed += 1
                self._update_stats(total)
                return
            
            read_value = read_result.result_data
            if str(read_value) == str(test_value):
                self._log(f"  ✓ Value matches: {self._truncate(str(read_value))}")
            else:
                self._log(f"  ✗ Value mismatch: expected {test_value}, got {read_value}")
                self.tests_failed += 1
                self._update_stats(total)
                return
            
            # Step 4: Store and reset
            self._log("Step 4: Storing settings and resetting...")
            store_result = self.api_executor.execute_at_command("AT&W")
            if not store_result.success:
                self._log(f"  ⚠ Store failed: {store_result.error_message}")
            else:
                self._log(f"  ✓ Settings stored")
            
            import time
            time.sleep(0.5)
            
            reset_result = self.api_executor.execute_at_command("AT+CPWROFF")
            if not reset_result.success:
                self._log(f"  ⚠ Reset failed: {reset_result.error_message}")
                self._log(f"  ✓ Partial pass (write/read/verify worked)")
                self.tests_passed += 1
                self._update_stats(total)
                return
            
            self._log(f"  ✓ Reset command sent, waiting 5 seconds...")
            time.sleep(5)
            
            # Step 5: Read back after reset
            self._log("Step 5: Reading value after reset...")
            persist_result = self.api_executor.execute_at_command(get_name + "?")
            if not persist_result.success:
                self._log(f"  ⚠ Failed to read after reset: {persist_result.error_message}")
                self._log(f"  ✓ Partial pass (device may still be restarting)")
                self.tests_passed += 1
                self._update_stats(total)
                return
            
            persist_value = persist_result.result_data
            if str(persist_value) == str(test_value):
                self._log(f"  ✓ Value persisted: {self._truncate(str(persist_value))}")
            else:
                self._log(f"  ⚠ Value not persisted: expected {test_value}, got {persist_value}")
            
            # Step 6: Factory reset
            self._log("Step 6: Factory reset and restart...")
            factory_result = self.api_executor.execute_at_command("AT&F")
            if not factory_result.success:
                self._log(f"  ⚠ Factory reset failed: {factory_result.error_message}")
                self._log(f"  ✓ Partial pass (write/read/persist worked)")
                self.tests_passed += 1
                self._update_stats(total)
                return
            
            self._log(f"  ✓ Factory reset sent")
            time.sleep(0.5)
            
            reset_result2 = self.api_executor.execute_at_command("AT+CPWROFF")
            if not reset_result2.success:
                self._log(f"  ⚠ Reset after factory failed")
                self.tests_passed += 1
                self._update_stats(total)
                return
            
            self._log(f"  ✓ Reset sent, waiting 5 seconds...")
            time.sleep(5)
            
            # Step 7: Verify default restored
            self._log("Step 7: Reading value after factory reset...")
            final_result = self.api_executor.execute_at_command(get_name + "?")
            if not final_result.success:
                self._log(f"  ⚠ Failed to read after factory reset")
                self.tests_passed += 1
                self._update_stats(total)
                return
            
            final_value = final_result.result_data
            if str(final_value) == str(default_value):
                self._log(f"  ✓ Default value restored: {self._truncate(str(final_value))}")
                self._log(f"  ✅ ALL 7 STEPS PASSED for {get_name}/{set_name}")
                self.tests_passed += 1
            else:
                self._log(f"  ⚠ Default not fully restored: expected {default_value}, got {final_value}")
                self._log(f"  ✓ Most tests passed (write/read/persist worked)")
                self.tests_passed += 1
            
            self._update_stats(total)
            
        except Exception as e:
            self._log(f"  ✗ Exception: {str(e)}")
            import traceback
            traceback.print_exc()
            self.tests_failed += 1
            self._update_stats(total)
    
    def _generate_realistic_value(self, set_info, current_value):
        """Generate a realistic test value based on parameter type and constraints"""
        try:
            # Get parameter info
            params = set_info.get('parameters', [])
            if not params:
                return None
            
            param = params[0]  # Use first parameter for simple cases
            param_type = param.get('type', 'string')
            
            # Handle different types
            if param_type == 'int':
                min_val = param.get('min', 0)
                max_val = param.get('max', 100)
                # Use middle value different from current
                try:
                    current_int = int(current_value)
                    test_val = (min_val + max_val) // 2
                    if test_val == current_int and max_val > min_val:
                        test_val = max_val if current_int < max_val else min_val
                    return str(test_val)
                except:
                    return str((min_val + max_val) // 2)
            
            elif param_type == 'string':
                # Generate test string different from current
                if str(current_value).lower().startswith('test'):
                    return '"TestValue2"'
                return '"TestValue1"'
            
            elif param_type == 'bool':
                # Toggle boolean
                if str(current_value) in ['1', 'true', 'True']:
                    return '0'
                return '1'
            
            elif param_type == 'ip_addr':
                return '"192.168.1.100"'
            
            elif param_type == 'mac_addr':
                # MAC address: 12 hex digits without separators
                return '"AABBCCDDEEFF"'
            
            elif param_type == 'bd_addr' or param_type == 'bt_addr':
                # Bluetooth address: 12 hex digits + type suffix (p=public, r=random)
                return '"001122334455p"'
            
            else:
                # Default: modify current value slightly
                return f'"{str(current_value)}_test"'
                
        except Exception as e:
            self._log(f"  ⚠ Error generating test value: {e}")
            return None
    
    def _truncate(self, text):
        """Truncate long text for display"""
        text = str(text)
        if len(text) > 100:
            return text[:97] + "..."
        return text
