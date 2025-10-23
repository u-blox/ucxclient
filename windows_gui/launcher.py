#!/usr/bin/env python3
"""
Launcher script for ucxclient GUI

This script provides a simple way to launch the GUI application
with proper error handling and environment checks.
"""

# Prevent Python from creating __pycache__ directories
import os
os.environ['PYTHONDONTWRITEBYTECODE'] = '1'

import sys
import tkinter as tk
from tkinter import messagebox

def get_library_name():
    """Get the platform-specific library name"""
    if sys.platform == "win32":
        return "ucxclient_windows.dll"
    elif sys.platform == "linux" or sys.platform == "linux2":
        return "libucxclient.so"
    elif sys.platform == "darwin":
        return "libucxclient.dylib"
    else:
        return "libucxclient.so"  # Default to .so for unknown Unix-like systems

def check_requirements():
    """Check if all requirements are met"""
    errors = []
    
    # Check Python version
    if sys.version_info < (3, 7):
        errors.append("Python 3.7 or higher is required")
    
    # Check tkinter
    try:
        import tkinter
    except ImportError:
        errors.append("tkinter is not available (please install tkinter support)")
    
    # Check if library exists
    launcher_dir = os.path.dirname(os.path.abspath(__file__))
    project_root = os.path.dirname(launcher_dir)  # Go up one directory from windows_gui
    
    lib_name = get_library_name()
    lib_paths = [
        os.path.join(project_root, "build", "Debug", lib_name),
        os.path.join(project_root, "build", "Release", lib_name),
        os.path.join(project_root, "build", lib_name)
    ]
    
    lib_found = False
    for lib_path in lib_paths:
        if os.path.exists(lib_path):
            lib_found = True
            break
    
    if not lib_found:
        errors.append(f"{lib_name} not found. Please build the project first using CMake.")
    
    return errors

def main():
    """Main launcher function"""
    platform_name = {"win32": "Windows", "linux": "Linux", "linux2": "Linux", "darwin": "macOS"}.get(sys.platform, sys.platform)
    print(f"ucxclient GUI Launcher ({platform_name})")
    print("==============================")
    
    # Check requirements
    errors = check_requirements()
    if errors:
        error_msg = "Requirements check failed:\n\n" + "\n".join(f"• {error}" for error in errors)
        print(error_msg)
        
        # Try to show GUI error if tkinter is available
        try:
            root = tk.Tk()
            root.withdraw()  # Hide main window
            # Set icon for error dialogs (Windows only)
            try:
                if sys.platform == "win32":
                    icon_path = os.path.join(os.path.dirname(__file__), "shortrange.ico")
                    if os.path.exists(icon_path):
                        root.iconbitmap(icon_path)
            except Exception:
                pass
            messagebox.showerror("Requirements Error", error_msg)
            root.destroy()
        except:
            pass
        
        return 1
    
    print("Requirements check passed!")
    print("Starting GUI application...")
    
    # Add current directory to path
    sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
    
    try:
        # Import and run the main application
        from main_window import MainWindow
        
        app = MainWindow()
        app.run()
        
        return 0
        
    except ImportError as e:
        error_msg = f"Failed to import application modules:\n{e}"
        print(error_msg)
        
        try:
            root = tk.Tk()
            root.withdraw()
            # Set icon for error dialogs (Windows only)
            try:
                if sys.platform == "win32":
                    icon_path = os.path.join(os.path.dirname(__file__), "shortrange.ico")
                    if os.path.exists(icon_path):
                        root.iconbitmap(icon_path)
            except Exception:
                pass
            messagebox.showerror("Import Error", error_msg)
            root.destroy()
        except:
            pass
        
        return 1
        
    except Exception as e:
        error_msg = f"Application error:\n{e}"
        print(error_msg)
        
        try:
            root = tk.Tk()
            root.withdraw()
            # Set icon for error dialogs (Windows only)
            try:
                if sys.platform == "win32":
                    icon_path = os.path.join(os.path.dirname(__file__), "shortrange.ico")
                    if os.path.exists(icon_path):
                        root.iconbitmap(icon_path)
            except Exception:
                pass
            messagebox.showerror("Application Error", error_msg)
            root.destroy()
        except:
            pass
        
        return 1

if __name__ == "__main__":
    sys.exit(main())