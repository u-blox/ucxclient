"""
PyInvoke tasks for ucxclient examples.

"""

from invoke import task, Collection
import os
import platform
import sys
import shlex
import time
import re
import threading

# Get absolute paths
EXAMPLES_DIR = os.path.dirname(os.path.abspath(__file__))
REPO_ROOT = os.path.dirname(EXAMPLES_DIR)
DOCKER_DIR = os.path.join(REPO_ROOT, 'docker')
DOCKER_SERVICE = 'stm32f4-builder'
DOCKER_WORKDIR = '/project/examples'

STM32_CMAKE_ARGS = (
    f'-DCMAKE_TOOLCHAIN_FILE={REPO_ROOT}/cmake/arm-none-eabi-gcc.cmake '
    '-DBUILD_STM32_EXAMPLES=ON'
)


class OutputCapture:
    """File-like helper that mirrors output and captures patterns."""

    def __init__(self, pattern_str=None):
        self.captured_data = []
        self.pattern = re.compile(pattern_str) if pattern_str else None
        self.match = None
        self._lock = threading.Lock()

    def write(self, data):
        if not data:
            return 0
        sys.stdout.write(data)
        sys.stdout.flush()

        with self._lock:
            self.captured_data.append(data)
            if self.pattern and self.match is None:
                full_text = ''.join(self.captured_data)
                self.match = self.pattern.search(full_text)

        return len(data)

    def flush(self):
        sys.stdout.flush()

    def get_full_output(self):
        with self._lock:
            return ''.join(self.captured_data)



def _is_windows():
    """Check if running on Windows."""
    return platform.system() == "Windows"


def _is_linux():
    """Check if running on Linux."""
    return platform.system() == "Linux"


def _running_inside_docker():
    """Check if running inside a Docker container."""
    return os.path.exists('/.dockerenv') or os.path.exists('/run/.containerenv')


def _require_linux(task_name):
    """Check if running on Linux and print error if not.

    Args:
        task_name: Name of the task requiring Linux

    Returns:
        True if running on Linux, False otherwise
    """
    if not _is_linux():
        print(f"Error: {task_name} requires Linux")
        print("Please run this task on a Linux host or WSL2")
        return False
    return True


def _reinvoke_inside_docker(c, task_label):
    """Re-run the current invoke command inside the STM32 Docker builder."""
    print(f">>> Reinvoking {task_label} inside Docker...")

    with c.cd(DOCKER_DIR):
        c.run(f'docker compose build {DOCKER_SERVICE}', warn=True)

        # Strip 'examples.' prefix from task names since we cd into DOCKER_WORKDIR
        # This allows running from project root (inv examples.stm32.renode) or
        # from examples dir (inv stm32.renode) and both work inside Docker.
        args = []
        for arg in sys.argv[1:]:
            if arg.startswith('examples.'):
                arg = arg[len('examples.'):]
            args.append(arg)
        argv = " ".join(shlex.quote(a) for a in args)
        env_vars = f"HOST_UID={os.getuid()} HOST_GID={os.getgid()}"

        c.run(
            f"{env_vars} docker compose run --rm {DOCKER_SERVICE} "
            f"bash -c 'cd {DOCKER_WORKDIR} && invoke {argv}'",
            pty=True,
        )


def _wait_for_app_main_return(captures, timeout=120, poll_interval=0.1):
    """Poll OutputCapture instances until pattern match or timeout."""
    start = time.time()
    while time.time() - start < timeout:
        for source, capture in captures:
            if capture.match:
                exit_code = int(capture.match.group(1))
                print(f"\n[Renode] Detected from {source}: app_main returned {exit_code}")
                return exit_code
        time.sleep(poll_interval)
    return None


def _terminate_tasks(tasks):
    """Terminate invoke tasks if they are still running."""
    for label, task in tasks:
        if task is None:
            continue
        try:
            task.terminate()
        except Exception:
            pass


def _configure_cmake(c, build_dir, cmake_args=""):
    """Configure CMake project if not already configured."""
    build_path = os.path.join(EXAMPLES_DIR, build_dir)
    if not os.path.exists(build_path):
        os.makedirs(build_path)

    # Only configure if CMakeCache doesn't exist
    if not os.path.exists(os.path.join(build_path, 'CMakeCache.txt')):
        print("Configuring CMake project...")
        with c.cd(build_path):
            if _is_windows() and not cmake_args:
                # Use default generator on Windows (auto-detects Visual Studio version)
                c.run(f'cmake {EXAMPLES_DIR} -A Win32 {cmake_args}')
            else:
                c.run(f'cmake {EXAMPLES_DIR} {cmake_args}')


def _build_target(c, target=None, clean=False, build_dir='build', cmake_args=""):
    """Build a specific target or all targets.

    Args:
        c: Invoke context
        target: Target name to build, or None for all targets
        clean: Clean build directory before building
        build_dir: Build directory name
        cmake_args: Additional CMake configuration arguments
    """
    build_path = os.path.join(EXAMPLES_DIR, build_dir)
    if clean and os.path.exists(build_path):
        print(f"Cleaning {build_dir}...")
        if _is_windows():
            c.run(f'rmdir /s /q {build_path}', warn=True)
        else:
            c.run(f'rm -rf {build_path}')

    _configure_cmake(c, build_dir, cmake_args)

    # Build
    target_name = f" --target {target}" if target else ""
    print(f"Building {target or 'all examples'}...")
    c.run(f'cmake --build {build_path}{target_name}')

    print(f"\nBuild completed successfully!")
    bin_dir = os.path.join(EXAMPLES_DIR, 'bin')
    if target:
        print(f"Executable: {os.path.join(bin_dir, target)}")
    else:
        print(f"Executables are located in {bin_dir}")

def _stm32_build_target(c, target=None, clean=False, docker=False, wifi_ssid=None, wifi_psk=None):
    """Unified STM32 build helper with Docker reinvoke pattern.

    Correct behavior:
      docker=False  => run locally (if toolchain available)
      docker=True + NOT in Docker => reinvoke this same task inside Docker
      docker=True + ALREADY in Docker => run the actual build

    Args:
        c: Invoke context
        target: CMake target base name (without .elf) or None for all
        clean: Whether to remove build directory first
        docker: Whether to build inside Docker container
        wifi_ssid: WiFi SSID to embed in the binary (falls back to WIFI_SSID env var)
        wifi_psk: WiFi WPA-PSK password to embed in the binary (falls back to WIFI_PSK env var)
    """
    # Fall back to environment variables for CI/GitHub Actions
    if wifi_ssid is None:
        wifi_ssid = os.environ.get('WIFI_SSID')
    if wifi_psk is None:
        wifi_psk = os.environ.get('WIFI_PSK')
    # User wants Docker, but we are NOT inside Docker → REINVOKE
    if docker and not _running_inside_docker():
        _reinvoke_inside_docker(c, 'stm32 build')
        return

    # Check if ARM toolchain is available
    result = c.run('which arm-none-eabi-gcc', warn=True, hide=True)
    if result.exited != 0:
        print("Error: ARM toolchain (arm-none-eabi-gcc) not found in PATH")
        print("\nTo build STM32 examples, either:")
        print("  1. Install ARM GCC toolchain locally, or")
        print("  2. Use --docker flag to build using stm32f4-builder Docker image")
        return

    build_dir = 'build_stm32'

    # Initialize STM32CubeF4 submodule if needed
    cube_path = os.path.join(REPO_ROOT, 'ports/extra/stm32f4/STM32CubeF4')
    if not os.path.exists(os.path.join(cube_path, '.git')):
        print("[STM32] Initializing STM32CubeF4 submodule...")
        with c.cd(REPO_ROOT):
            c.run('git submodule update --init --depth 1 ports/extra/stm32f4/STM32CubeF4')
            c.run('cd ports/extra/stm32f4/STM32CubeF4 && git submodule update --init --depth 1 Drivers/STM32F4xx_HAL_Driver Drivers/CMSIS Middlewares/Third_Party/FreeRTOS')

    build_path = os.path.join(EXAMPLES_DIR, build_dir)
    bin_path = os.path.join(EXAMPLES_DIR, 'bin')
    os.makedirs(build_path, exist_ok=True)
    os.makedirs(bin_path, exist_ok=True)

    # Build CMake args with optional WiFi credentials
    cmake_args = STM32_CMAKE_ARGS

    if wifi_ssid:
        # Escape special characters for shell
        escaped_ssid = wifi_ssid.replace('"', '\\"')
        cmake_args += f' -DU_EXAMPLE_SSID="{escaped_ssid}"'
    if wifi_psk:
        # Escape special characters for shell - use single quotes to protect #, @, etc
        cmake_args += f" '-DU_EXAMPLE_WPA_PSK={wifi_psk}'"

    # Build using native CMake
    _build_target(c, target=target, clean=clean, build_dir=build_dir,
                  cmake_args=cmake_args)


def _stm32_clean(c):
    """Unified STM32 clean helper."""
    print("Cleaning STM32 build artifacts...")
    build_path = os.path.join(EXAMPLES_DIR, 'build_stm32')
    c.run(f'rm -rf {build_path}', warn=True)
    print("STM32 clean complete!")


def _stm32_cleanup_containers(c):
    """Kill any stale Renode containers and UART console connections."""
    # Only run docker commands if not inside docker
    if not _running_inside_docker():
        c.run('docker ps -aq --filter name=docker-stm32f4-builder-run | xargs -r docker rm -f', warn=True)
    # Kill any netcat processes connected to port 3456 (UART console)
    c.run('pkill -f "nc.*localhost.*3456" || true', warn=True)
    # Kill any invoke stm32.uart-console tasks
    c.run('pkill -f "inv stm32.uart-console" || true', warn=True)
    # Kill any ucx_mock emulator processes
    c.run('pkill -f "ucx_mock.*--pty" || true', warn=True)

def _stm32_renode(c, example='http_example', build=False, gdb=False):
    """Run an STM32 example in Renode emulator with GDB server.

    This function always runs inside Docker and uses reinvoke pattern if needed.

    Args:
        c: Invoke context
        example: Example name (http_example or fw_upgrade_example)
        build: Whether to build before running
        gdb: Whether to start GDB server and pause on startup
    """
    if not _running_inside_docker():
        _reinvoke_inside_docker(c, 'stm32.renode')
        return

    # Build first if requested
    if build:
        print(f"[Renode] Building {example} first...")
        _stm32_build_target(c, target=f"{example}_stm32", clean=False)

    # Check if binary exists
    local_elf = os.path.join(EXAMPLES_DIR, f"bin/{example}_stm32.elf")
    if not os.path.exists(local_elf):
        print(f"Error: {local_elf} not found. Build first with --build flag.")
        return

    elf_path = os.path.join(EXAMPLES_DIR, f"bin/{example}_stm32.elf")
    script_file = os.path.join(REPO_ROOT, "ports/extra/stm32f4/scripts/run_stm32_example.resc")
    if gdb:
        script_file = os.path.join(REPO_ROOT, "ports/extra/stm32f4/scripts/run_stm32_example_gdb.resc")

    ucx_mock = os.path.join(REPO_ROOT, "test/support/ucx_mock")
    ucx_monitor_script = os.path.join(REPO_ROOT, "test/support/ucx_mock_monitor.sh")
    pty_renode = "/tmp/renode_usart3"

    print(f"[Renode] Starting emulation of {example}...")
    print(f"[Renode] Binary: {elf_path}")
    print(f"[Renode] Script: {script_file}")
    print(f"[Renode] USART3 PTY (AT): {pty_renode}")
    if gdb:
        print(f"[Renode] GDB server: localhost:3333 (paused on startup)")
    print(f"[Renode] Press Ctrl+C to exit\n")

    # Cleanup any stale processes
    _stm32_cleanup_containers(c)

    # Create HOME/.config directory for Renode
    os.makedirs(os.path.expanduser("~/.config"), exist_ok=True)

    # Prepare Renode script with variables
    renode_script = (
        f'set ELF_FILE \\"{elf_path}\\" ; '
        f'set EXAMPLE_NAME \\"{example}\\" ; '
        f'set AT_PTY \\"{pty_renode}\\" ; '
        f'i @{script_file}'
    )

    # Start Renode in background using invoke with output capture
    renode_cmd = (
        f'renode --disable-xwt --port 9999 -e "{renode_script}"'
    )

    print(f"[Renode] Starting: {renode_cmd}")
    renode_output = OutputCapture()
    renode_task = c.run(renode_cmd,
                        out_stream=renode_output,
                        err_stream=renode_output,
                        pty=False, warn=True, asynchronous=True)

    # Wait for PTY to be created
    print(f"[Renode] Waiting for PTY: {pty_renode}")
    timeout = 300  # 60 seconds * 0.2s per loop
    while not os.path.exists(pty_renode):
        time.sleep(0.2)
        timeout -= 1
        if timeout <= 0:
            print("[ERROR] Timeout waiting for PTY")
            return

    # Make PTY world-readable
    try:
        os.chmod(pty_renode, 0o666)
    except OSError:
        pass

    print(f"[Renode] PTY created: {pty_renode}")

    # Start ucx_mock emulator in background with output capture using wrapper script
    print(f"[Renode] Starting ucx_mock with auto-restart: {ucx_mock} --pty {pty_renode}")
    ucx_cmd = f"bash {ucx_monitor_script} {ucx_mock} {pty_renode}"
    ucx_output = OutputCapture()
    ucx_task = c.run(ucx_cmd,
                     out_stream=ucx_output,
                     err_stream=ucx_output,
                     pty=False, warn=True, asynchronous=True)

    # Give ucx_mock/Renode a brief moment before opening UART console
    print("[Renode] Connecting to UART console (port 3456)")
    time.sleep(1)

    # Create output capture to watch for "app_main returned"
    nc_output = OutputCapture(pattern_str=r'app_main returned (\d+)')

    # Run nc in background to display UART console output
    nc_task = c.run('nc localhost 3456',
                    out_stream=nc_output,
                    err_stream=nc_output,
                    pty=False, warn=True, asynchronous=True)

    exit_code = None
    try:
        exit_code = _wait_for_app_main_return([
            ("netcat", nc_output),
            ("ucx_mock", ucx_output),
        ])
    except KeyboardInterrupt:
        print("\n[Renode] Received Ctrl+C")
    finally:
        print("\n[Renode] Cleaning up processes...")
        _terminate_tasks([
            ("netcat", nc_task),
            ("ucx_mock", ucx_task),
            ("Renode", renode_task),
        ])

    if exit_code is not None:
        print(f"[Renode] Exiting with code: {exit_code}")
        sys.exit(exit_code)
    else:
        # Pattern not detected - this is a test failure
        print("[Renode] ERROR: Test did not complete - 'app_main returned' not detected")
        print("[Renode] This could mean:")
        print("  - The application hung or crashed")
        print("  - The test timed out (120 seconds)")
        print("  - Output capture failed")
        sys.exit(1)


@task(help={'clean': 'Clean build directory before building'})
def all(c, clean=False):
    """Build all examples."""
    _build_target(c, clean=clean)


@task(help={'clean': 'Clean build directory before building'})
def http(c, clean=False):
    """Build http_example."""
    _build_target(c, target='http_example', clean=clean)


@task(help={'clean': 'Clean build directory before building'})
def fw_upgrade(c, clean=False):
    """Build fw_upgrade_example."""
    _build_target(c, target='fw_upgrade_example', clean=clean)


@task
def clean(c):
    """Clean all build artifacts."""
    print("Cleaning build artifacts...")
    build_path = os.path.join(EXAMPLES_DIR, 'build')
    bin_path = os.path.join(EXAMPLES_DIR, 'bin')
    if _is_windows():
        c.run(f'rmdir /s /q {build_path} {bin_path} 2>nul', warn=True)
    else:
        c.run(f'rm -rf {build_path} {bin_path}', warn=True)
    print("Clean complete!")


# Platform-specific namespaces

# STM32 platform tasks
@task(help={
    'clean': 'Clean build directory before building',
    'wifi-ssid': 'WiFi SSID to embed in the binary (or set WIFI_SSID env var)',
    'wifi-psk': 'WiFi WPA-PSK password to embed in the binary (or set WIFI_PSK env var)',
    'docker': 'Build inside Docker container (no local ARM toolchain required)',
})
def stm32_http(c, clean=False, wifi_ssid=None, wifi_psk=None, docker=False):
    """Build http_example for STM32 with optional WiFi credentials.

    WiFi credentials can be provided via --wifi-ssid/--wifi-psk arguments
    or via WIFI_SSID/WIFI_PSK environment variables (useful in CI).
    """
    _stm32_build_target(c, target='http_example_stm32.elf', clean=clean, docker=docker,
                        wifi_ssid=wifi_ssid, wifi_psk=wifi_psk)


@task(help={
    'clean': 'Clean build directory before building',
    'docker': 'Build inside Docker container (no local ARM toolchain required)',
})
def stm32_fw_upgrade(c, clean=False, docker=False):
    """Build fw_upgrade_example for STM32."""
    _stm32_build_target(c, target='fw_upgrade_example_stm32', clean=clean, docker=docker)


@task(help={
    'clean': 'Clean build directory before building',
    'docker': 'Build inside Docker container (no local ARM toolchain required)',
})
def stm32_all(c, clean=False, docker=False):
    """Build all STM32 examples."""
    _stm32_build_target(c, target=None, clean=clean, docker=docker)


@task
def stm32_clean(c):
    """Clean STM32 build artifacts."""
    _stm32_clean(c)


@task(help={
    'example': 'Example to run (http_example or fw_upgrade_example), default: http_example',
    'build': 'Build before running',
    'gdb': 'Start GDB server on port 3333 and pause emulation on startup',
})
def stm32_renode(c, example='http_example', build=False, gdb=False):
    """Run STM32 example in Renode emulator with optional GDB server on port 3333."""
    if not _require_linux('stm32.renode'):
        return
    _stm32_renode(c, example=example, build=build, gdb=gdb)


@task
def stm32_cleanup(c):
    """Stop any running Renode containers and UART console connections."""
    if not _require_linux('stm32.cleanup'):
        return
    with c.cd(DOCKER_DIR):
        _stm32_cleanup_containers(c)


# Windows platform tasks
@task(help={'clean': 'Clean build directory before building'})
def win32_http(c, clean=False):
    """Build http_example for Windows (only available on Windows host)."""
    if not _is_windows():
        print("Error: win32 builds are only available on Windows hosts")
        return
    _build_target(c, target='http_example', clean=clean, build_dir='build')


@task(help={'clean': 'Clean build directory before building'})
def win32_fw_upgrade(c, clean=False):
    """Build fw_upgrade_example for Windows (only available on Windows host)."""
    if not _is_windows():
        print("Error: win32 builds are only available on Windows hosts")
        return
    _build_target(c, target='fw_upgrade_example', clean=clean, build_dir='build')


@task(help={'clean': 'Clean build directory before building'})
def win32_all(c, clean=False):
    """Build all examples for Windows (only available on Windows host)."""
    if not _is_windows():
        print("Error: win32 builds are only available on Windows hosts")
        return
    _build_target(c, target=None, clean=clean, build_dir='build')


# Linux platform tasks
@task(help={'clean': 'Clean build directory before building'})
def linux_http(c, clean=False):
    """Build http_example for Linux (only available on Linux host)."""
    if not _is_linux():
        print("Error: linux builds are only available on Linux hosts")
        return
    _build_target(c, target='http_example', clean=clean, build_dir='build')


@task(help={'clean': 'Clean build directory before building'})
def linux_fw_upgrade(c, clean=False):
    """Build fw_upgrade_example for Linux (only available on Linux host)."""
    if not _is_linux():
        print("Error: linux builds are only available on Linux hosts")
        return
    _build_target(c, target='fw_upgrade_example', clean=clean, build_dir='build')


@task(help={'clean': 'Clean build directory before building'})
def linux_all(c, clean=False):
    """Build all examples for Linux (only available on Linux host)."""
    if not _is_linux():
        print("Error: linux builds are only available on Linux hosts")
        return
    _build_target(c, target=None, clean=clean, build_dir='build')


# Create namespaces
ns = Collection()

# STM32 platform namespace
stm32_ns = Collection('stm32')
stm32_ns.add_task(stm32_http, 'http')
stm32_ns.add_task(stm32_fw_upgrade, 'fw-upgrade')
stm32_ns.add_task(stm32_all, 'all')
stm32_ns.add_task(stm32_clean, 'clean')
stm32_ns.add_task(stm32_renode, 'renode')
stm32_ns.add_task(stm32_cleanup, 'cleanup')
ns.add_collection(stm32_ns)

# Windows platform namespace
if _is_windows():
    win32_ns = Collection('win32')
    win32_ns.add_task(win32_http, 'http')
    win32_ns.add_task(win32_fw_upgrade, 'fw-upgrade')
    win32_ns.add_task(win32_all, 'all')
    ns.add_collection(win32_ns)

# Linux platform namespace
if _is_linux():
    linux_ns = Collection('linux')
    linux_ns.add_task(linux_http, 'http')
    linux_ns.add_task(linux_fw_upgrade, 'fw-upgrade')
    linux_ns.add_task(linux_all, 'all')
    ns.add_collection(linux_ns)

