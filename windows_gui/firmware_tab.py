"""
Firmware Update Tab for ucxclient GUI

Provides firmware update functionality using XMODEM protocol.
"""

import os
os.environ['PYTHONDONTWRITEBYTECODE'] = '1'

import tkinter as tk
from tkinter import ttk, filedialog, messagebox
import threading
import time

class FirmwareUpdateTab:
    """Firmware update tab with XMODEM protocol support"""
    
    def __init__(self, parent, ucx_wrapper, settings_manager=None):
        """
        Initialize firmware update tab
        
        Args:
            parent: Parent widget
            ucx_wrapper: UCX wrapper instance
            settings_manager: Settings manager instance
        """
        self.parent = parent
        self.ucx_wrapper = ucx_wrapper
        self.settings = settings_manager
        self.update_in_progress = False
        
        # Create main frame
        self.main_frame = ttk.Frame(parent, padding=10)
        self.main_frame.pack(fill=tk.BOTH, expand=True)
        
        self._create_widgets()
        self._load_saved_settings()
    
    def _create_widgets(self):
        """Create all widgets for the firmware update tab"""
        
        # Title
        title_label = ttk.Label(self.main_frame, text="Firmware Update via XMODEM",
                               font=('Segoe UI', 12, 'bold'))
        title_label.pack(pady=(0, 15))
        
        # File selection frame
        file_frame = ttk.LabelFrame(self.main_frame, text="Firmware File", padding=10)
        file_frame.pack(fill=tk.X, pady=(0, 10))
        
        # File path
        file_row = ttk.Frame(file_frame)
        file_row.pack(fill=tk.X)
        
        ttk.Label(file_row, text="File:").pack(side=tk.LEFT, padx=(0, 5))
        
        self.file_path_var = tk.StringVar()
        self.file_entry = ttk.Entry(file_row, textvariable=self.file_path_var, state='readonly')
        self.file_entry.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=5)
        
        self.browse_button = ttk.Button(file_row, text="Browse...", command=self._browse_file)
        self.browse_button.pack(side=tk.LEFT, padx=(5, 0))
        
        # File info
        self.file_info_label = ttk.Label(file_frame, text="No file selected", foreground='gray')
        self.file_info_label.pack(pady=(5, 0))
        
        # Configuration frame
        config_frame = ttk.LabelFrame(self.main_frame, text="Transfer Configuration", padding=10)
        config_frame.pack(fill=tk.X, pady=(0, 10))
        
        # Baudrate selection
        baud_row = ttk.Frame(config_frame)
        baud_row.pack(fill=tk.X, pady=(0, 5))
        
        ttk.Label(baud_row, text="Baudrate:").pack(side=tk.LEFT, padx=(0, 5))
        
        self.baudrate_var = tk.StringVar(value="921600")
        baudrate_combo = ttk.Combobox(baud_row, textvariable=self.baudrate_var, 
                                     values=["115200", "230400", "460800", "921600"],
                                     state='readonly', width=15)
        baudrate_combo.pack(side=tk.LEFT, padx=5)
        
        ttk.Label(baud_row, text="(higher = faster transfer)", 
                 foreground='gray').pack(side=tk.LEFT, padx=(5, 0))
        
        # Block size selection
        block_row = ttk.Frame(config_frame)
        block_row.pack(fill=tk.X)
        
        ttk.Label(block_row, text="Block size:").pack(side=tk.LEFT, padx=(0, 5))
        
        self.block_size_var = tk.StringVar(value="1K")
        ttk.Radiobutton(block_row, text="128 bytes", variable=self.block_size_var, 
                       value="128").pack(side=tk.LEFT, padx=5)
        ttk.Radiobutton(block_row, text="1K bytes (faster)", variable=self.block_size_var,
                       value="1K").pack(side=tk.LEFT, padx=5)
        
        # Progress frame
        progress_frame = ttk.LabelFrame(self.main_frame, text="Transfer Progress", padding=10)
        progress_frame.pack(fill=tk.BOTH, expand=True, pady=(0, 10))
        
        # Progress bar
        self.progress_var = tk.DoubleVar(value=0)
        self.progress_bar = ttk.Progressbar(progress_frame, variable=self.progress_var,
                                           maximum=100, mode='determinate')
        self.progress_bar.pack(fill=tk.X, pady=(0, 5))
        
        # Progress text
        self.progress_text_var = tk.StringVar(value="Ready to update")
        self.progress_label = ttk.Label(progress_frame, textvariable=self.progress_text_var)
        self.progress_label.pack(pady=(0, 10))
        
        # Status log
        log_frame = ttk.Frame(progress_frame)
        log_frame.pack(fill=tk.BOTH, expand=True)
        
        log_scroll = ttk.Scrollbar(log_frame)
        log_scroll.pack(side=tk.RIGHT, fill=tk.Y)
        
        self.log_text = tk.Text(log_frame, height=10, width=60, 
                               yscrollcommand=log_scroll.set,
                               state='disabled', wrap=tk.WORD,
                               background='#1e1e1e', foreground='#d4d4d4',
                               font=('Consolas', 9))
        self.log_text.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        log_scroll.config(command=self.log_text.yview)
        
        # Configure text tags for colored output
        self.log_text.tag_config('info', foreground='#4ec9b0')
        self.log_text.tag_config('success', foreground='#4ec9b0', font=('Consolas', 9, 'bold'))
        self.log_text.tag_config('warning', foreground='#dcdcaa')
        self.log_text.tag_config('error', foreground='#f48771')
        
        # Control buttons frame
        button_frame = ttk.Frame(self.main_frame)
        button_frame.pack(fill=tk.X, pady=(0, 0))
        
        # Update button
        self.update_button = ttk.Button(button_frame, text="Start Firmware Update",
                                       command=self._start_update, style='Accent.TButton')
        self.update_button.pack(side=tk.LEFT, padx=(0, 5))
        
        # Cancel button
        self.cancel_button = ttk.Button(button_frame, text="Cancel", 
                                       command=self._cancel_update, state='disabled')
        self.cancel_button.pack(side=tk.LEFT, padx=5)
        
        # Clear log button
        ttk.Button(button_frame, text="Clear Log", 
                  command=self._clear_log).pack(side=tk.RIGHT, padx=(5, 0))
        
        # Warning label
        warning_frame = ttk.Frame(self.main_frame)
        warning_frame.pack(fill=tk.X, pady=(10, 0))
        
        warning_text = ("⚠ Warning: Module will reboot after update. "
                       "Disconnect and reconnect after completion.")
        ttk.Label(warning_frame, text=warning_text, foreground='orange',
                 font=('Segoe UI', 9, 'italic')).pack()
    
    def _browse_file(self):
        """Open file browser to select firmware file"""
        filename = filedialog.askopenfilename(
            title="Select Firmware File",
            filetypes=[
                ("Binary files", "*.bin"),
                ("All files", "*.*")
            ],
            initialdir=os.path.expanduser("~")
        )
        
        if filename:
            self.file_path_var.set(filename)
            self._update_file_info(filename)
            self._save_settings()
    
    def _update_file_info(self, filepath):
        """Update file information label"""
        try:
            size = os.path.getsize(filepath)
            size_mb = size / (1024 * 1024)
            filename = os.path.basename(filepath)
            
            # Estimate transfer time
            baudrate = int(self.baudrate_var.get())
            block_size = 1024 if self.block_size_var.get() == "1K" else 128
            
            # Rough estimate: bytes / (baudrate/10) seconds
            # Add overhead for protocol (headers, CRC, ACKs)
            overhead = 1.3  # 30% overhead
            est_seconds = (size * 10 * overhead) / baudrate
            
            if est_seconds < 60:
                time_str = f"~{int(est_seconds)}s"
            else:
                time_str = f"~{int(est_seconds/60)}m {int(est_seconds%60)}s"
            
            info_text = f"{filename} - {size_mb:.2f} MB - Est. time: {time_str} @ {baudrate} baud"
            self.file_info_label.config(text=info_text, foreground='white')
            
        except Exception as e:
            self.file_info_label.config(text=f"Error reading file: {e}", foreground='red')
    
    def _start_update(self):
        """Start firmware update process"""
        # Validate
        if not self.file_path_var.get():
            messagebox.showwarning("No File Selected", "Please select a firmware file first.")
            return
        
        if not os.path.exists(self.file_path_var.get()):
            messagebox.showerror("File Not Found", "The selected file does not exist.")
            return
        
        if not self.ucx_wrapper or not hasattr(self.ucx_wrapper, 'handle'):
            messagebox.showerror("Not Connected", 
                               "Please connect to the device before updating firmware.")
            return
        
        # Confirm
        result = messagebox.askyesno(
            "Confirm Firmware Update",
            f"This will update the firmware using:\n\n"
            f"File: {os.path.basename(self.file_path_var.get())}\n"
            f"Baudrate: {self.baudrate_var.get()}\n"
            f"Block size: {self.block_size_var.get()} bytes\n\n"
            f"The module will reboot after update.\n\n"
            f"Continue?",
            icon='warning'
        )
        
        if not result:
            return
        
        # Disable controls
        self.update_in_progress = True
        self.update_button.config(state='disabled')
        self.cancel_button.config(state='normal')
        self.browse_button.config(state='disabled')
        
        # Clear log
        self._clear_log()
        
        # Start update in background thread
        thread = threading.Thread(target=self._update_thread, daemon=True)
        thread.start()
    
    def _update_thread(self):
        """Background thread for firmware update"""
        try:
            filepath = self.file_path_var.get()
            baudrate = int(self.baudrate_var.get())
            use_1k = (self.block_size_var.get() == "1K")
            
            self._log("=== Firmware Update Started ===", 'info')
            self._log(f"File: {os.path.basename(filepath)}", 'info')
            self._log(f"Size: {os.path.getsize(filepath) / (1024*1024):.2f} MB", 'info')
            self._log(f"Baudrate: {baudrate}", 'info')
            self._log(f"Block size: {'1K' if use_1k else '128'} bytes", 'info')
            self._log("")
            
            # Get current connection parameters
            port_name = self.ucx_wrapper._port_name if self.ucx_wrapper._port_name else "COM3"
            use_flow_control = self.ucx_wrapper._flow_control
            
            # Call the firmware update function
            result = self.ucx_wrapper.lib.uCxFirmwareUpdate(
                self.ucx_wrapper.handle,
                filepath.encode('utf-8'),
                port_name.encode('utf-8'),
                baudrate,
                use_flow_control,
                self.ucx_wrapper.lib.uCxFirmwareUpdateProgress_t(self._progress_callback),
                None
            )
            
            if result == 0:
                self._log("", 'success')
                self._log("✓ Firmware update completed successfully!", 'success')
                self._log("", 'success')
                self._log("Module will now reboot.", 'warning')
                self._log("Please disconnect and reconnect to the device.", 'warning')
                
                self.parent.after(0, lambda: messagebox.showinfo(
                    "Update Complete",
                    "Firmware updated successfully!\n\n"
                    "The module will reboot.\n"
                    "Please disconnect and reconnect."
                ))
            else:
                self._log(f"✗ Firmware update failed with error code: {result}", 'error')
                self.parent.after(0, lambda: messagebox.showerror(
                    "Update Failed",
                    f"Firmware update failed with error code: {result}\n\n"
                    "Check the log for details."
                ))
        
        except Exception as e:
            error_msg = str(e)
            self._log(f"✗ Exception during update: {error_msg}", 'error')
            self.parent.after(0, lambda msg=error_msg: messagebox.showerror(
                "Update Error",
                f"An error occurred during firmware update:\n\n{msg}"
            ))
        
        finally:
            # Re-enable controls
            self.update_in_progress = False
            self.parent.after(0, self._update_complete)
    
    def _progress_callback(self, total, transferred, percent, userdata):
        """Progress callback from XMODEM transfer"""
        # Update progress bar and text (must be done in main thread)
        self.parent.after(0, lambda: self._update_progress(total, transferred, percent))
    
    def _update_progress(self, total, transferred, percent):
        """Update progress display (runs in main thread)"""
        self.progress_var.set(percent)
        
        total_mb = total / (1024 * 1024)
        transferred_mb = transferred / (1024 * 1024)
        
        self.progress_text_var.set(
            f"{percent}% - {transferred_mb:.2f} MB / {total_mb:.2f} MB"
        )
    
    def _log(self, message, tag='info'):
        """Add message to log"""
        def _do_log():
            self.log_text.config(state='normal')
            self.log_text.insert(tk.END, message + '\n', tag)
            self.log_text.see(tk.END)
            self.log_text.config(state='disabled')
        
        # Schedule in main thread
        self.parent.after(0, _do_log)
    
    def _clear_log(self):
        """Clear the log text"""
        self.log_text.config(state='normal')
        self.log_text.delete('1.0', tk.END)
        self.log_text.config(state='disabled')
    
    def _cancel_update(self):
        """Cancel firmware update"""
        if messagebox.askyesno("Cancel Update", 
                              "Are you sure you want to cancel the firmware update?\n\n"
                              "This may leave the module in an unstable state."):
            self._log("⚠ Update cancelled by user", 'warning')
            self.update_in_progress = False
            self._update_complete()
    
    def _update_complete(self):
        """Re-enable controls after update completion"""
        self.update_button.config(state='normal')
        self.cancel_button.config(state='disabled')
        self.browse_button.config(state='normal')
        self.progress_var.set(0)
        self.progress_text_var.set("Ready to update")
    
    def _load_saved_settings(self):
        """Load saved settings"""
        if self.settings:
            firmware_config = self.settings.get('firmware', {})
            
            last_file = firmware_config.get('last_file', '')
            if last_file and os.path.exists(last_file):
                self.file_path_var.set(last_file)
                self._update_file_info(last_file)
            
            self.baudrate_var.set(str(firmware_config.get('baudrate', '921600')))
            self.block_size_var.set(firmware_config.get('block_size', '1K'))
    
    def _save_settings(self):
        """Save current settings"""
        if self.settings:
            if 'firmware' not in self.settings.settings:
                self.settings.settings['firmware'] = {}
            
            self.settings.settings['firmware']['last_file'] = self.file_path_var.get()
            self.settings.settings['firmware']['baudrate'] = self.baudrate_var.get()
            self.settings.settings['firmware']['block_size'] = self.block_size_var.get()
            self.settings.save()
    
    def get_frame(self):
        """Get the main frame widget"""
        return self.main_frame
