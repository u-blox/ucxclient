# Building Linux Library from Windows

You have several options to build the Linux `.so` library while on Windows:

## Option 1: WSL (Windows Subsystem for Linux) ⭐ **RECOMMENDED**

This is the easiest and most reliable way to build native Linux binaries on Windows.

### Setup WSL (One-Time)

1. **Install WSL2** (Windows 10/11):
   ```powershell
   # Run in PowerShell as Administrator
   wsl --install
   ```
   
   Or install a specific distribution:
   ```powershell
   wsl --install -d Ubuntu
   ```

2. **Restart your computer** when prompted

3. **Launch Ubuntu** from Start Menu and create a user account

### Build in WSL

1. **Access your Windows files** (they're mounted at `/mnt/c/`):
   ```bash
   cd /mnt/c/u-blox/ucxclient
   ```

2. **Install build dependencies**:
   ```bash
   sudo apt-get update
   sudo apt-get install -y build-essential cmake python3 python3-tk python3-pip
   pip3 install pyserial
   ```

3. **Build the library**:
   ```bash
   mkdir -p build
   cd build
   cmake ..
   cmake --build .
   ```

4. **Output**: `build/libucxclient.so` is now available!

5. **Test the GUI from WSL**:
   ```bash
   cd /mnt/c/u-blox/ucxclient/examples/python_gui
   python3 launcher.py
   ```
   
   Note: For GUI apps, you may need an X server (see X Server Setup below)

### X Server Setup (for GUI)

WSL doesn't include a display server by default. For GUI apps:

**Option A: WSLg (Windows 11 or Windows 10 with recent updates)**
- WSLg includes built-in GUI support - nothing to install!
- Just run: `python3 launcher.py`

**Option B: X Server (Windows 10)**
1. Install [VcXsrv](https://sourceforge.net/projects/vcxsrv/) or [Xming](https://sourceforge.net/projects/xming/)
2. Launch XLaunch with default settings
3. In WSL, set display:
   ```bash
   export DISPLAY=:0
   python3 launcher.py
   ```

---

## Option 2: Docker 🐳

Build in a Linux container without full WSL installation.

### Setup (One-Time)

1. Install [Docker Desktop for Windows](https://www.docker.com/products/docker-desktop)
2. Enable WSL 2 backend in Docker settings

### Build with Docker

1. **Create a Dockerfile** (already included in this guide):
   
   Create `Dockerfile.build` in project root:
   ```dockerfile
   FROM ubuntu:22.04
   
   RUN apt-get update && apt-get install -y \
       build-essential \
       cmake \
       python3 \
       python3-pip \
       && rm -rf /var/lib/apt/lists/*
   
   WORKDIR /workspace
   ```

2. **Build the library**:
   ```powershell
   # In PowerShell from ucxclient directory
   docker build -f Dockerfile.build -t ucxclient-builder .
   docker run --rm -v ${PWD}:/workspace ucxclient-builder bash -c "mkdir -p build && cd build && cmake .. && cmake --build ."
   ```

3. **Output**: `build/libucxclient.so` created in your Windows directory!

---

## Option 3: Cross-Compilation (Advanced) ⚠️

Use a cross-compiler toolchain to build Linux binaries on Windows.

### Setup

1. **Install MinGW-w64 with Linux target**:
   - Download from [winlibs.com](https://winlibs.com/)
   - Or use [MSYS2](https://www.msys2.org/):
     ```bash
     pacman -S mingw-w64-x86_64-gcc
     ```

2. **Create a CMake toolchain file** (`linux-toolchain.cmake`):
   ```cmake
   set(CMAKE_SYSTEM_NAME Linux)
   set(CMAKE_C_COMPILER x86_64-linux-gnu-gcc)
   set(CMAKE_CXX_COMPILER x86_64-linux-gnu-g++)
   set(CMAKE_FIND_ROOT_PATH /usr/x86_64-linux-gnu)
   set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
   set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
   set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
   ```

3. **Build with toolchain**:
   ```powershell
   mkdir build-linux
   cd build-linux
   cmake .. -DCMAKE_TOOLCHAIN_FILE=../linux-toolchain.cmake
   cmake --build .
   ```

**Challenges**:
- Complex setup
- May need to compile dependencies for Linux
- Hard to troubleshoot linking issues
- Serial port libraries may not cross-compile easily

---

## Option 4: Virtual Machine

Run a full Linux VM on Windows.

### Setup

1. Install [VirtualBox](https://www.virtualbox.org/) or [VMware Workstation Player](https://www.vmware.com/products/workstation-player.html)
2. Create Ubuntu VM
3. Install build tools:
   ```bash
   sudo apt-get update
   sudo apt-get install -y build-essential cmake python3 python3-tk
   pip3 install pyserial
   ```

### Build

1. Share your Windows folder with the VM (VirtualBox Shared Folders)
2. Mount in Linux and build:
   ```bash
   cd /mnt/ucxclient  # or wherever you mounted
   mkdir -p build
   cd build
   cmake ..
   cmake --build .
   ```

---

## Recommendation Matrix

| Method | Ease of Setup | Build Speed | GUI Testing | Best For |
|--------|--------------|-------------|-------------|----------|
| **WSL2** | ⭐⭐⭐⭐⭐ Easy | ⚡⚡⚡ Fast | ✅ Yes (WSLg) | **Most users** |
| **Docker** | ⭐⭐⭐⭐ Easy | ⚡⚡⚡ Fast | ❌ No | CI/CD builds |
| **Cross-compile** | ⭐⭐ Hard | ⚡⚡ Medium | ❌ No | Advanced users |
| **VM** | ⭐⭐⭐ Medium | ⚡ Slow | ✅ Yes | Full Linux environment |

---

## Quick Start: WSL2 (Step by Step)

Here's the complete workflow for most users:

```powershell
# 1. Install WSL (PowerShell as Admin, one-time only)
wsl --install -d Ubuntu

# 2. Restart Windows

# 3. Open Ubuntu from Start Menu, then:
cd /mnt/c/u-blox/ucxclient
sudo apt-get update
sudo apt-get install -y build-essential cmake python3 python3-tk python3-pip
pip3 install pyserial

# 4. Build
mkdir -p build
cd build
cmake ..
cmake --build .

# 5. Run GUI (if you have WSLg on Windows 11)
cd ../examples/python_gui
python3 launcher.py

# Done! You now have libucxclient.so
```

---

## Testing the Linux Build on Windows

### With WSL2 + WSLg (Windows 11)
- The GUI will work directly!
- USB devices are accessible via `usbipd` tool

### With WSL2 (Windows 10)
- Install an X server (VcXsrv)
- Set `export DISPLAY=:0`
- USB passthrough requires additional setup

### Verifying the Build
```bash
# Check the library was created
ls -lh build/libucxclient.so

# Check it's a proper ELF binary
file build/libucxclient.so
# Should output: "ELF 64-bit LSB shared object, x86-64..."

# Check exported symbols
nm -D build/libucxclient.so | grep uCx
```

---

## USB Device Access in WSL

To access USB serial devices from WSL:

### Windows 11 (Built-in)
```powershell
# In PowerShell as Admin
usbipd wsl list
usbipd wsl attach --busid <BUSID>
```

### Windows 10
Install [usbipd-win](https://github.com/dorssel/usbipd-win):
```powershell
winget install --interactive --exact dorssel.usbipd-win
```

Then use same commands as Windows 11.

---

## Summary

**For most users**: Use **WSL2** - it's built into Windows 10/11, fast, and gives you a real Linux environment.

**For CI/CD**: Use **Docker** - containerized, reproducible builds.

**For learning**: Try **cross-compilation** - but be prepared for complexity.

The Linux `.so` library will work identically to the Windows `.dll` - the Python GUI will automatically detect and use it!
