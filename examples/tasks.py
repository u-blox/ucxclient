"""
PyInvoke tasks for ucxclient examples.

"""

from invoke import task, Collection
import os
import platform
import sys
import shlex
import shutil
import time
import re
import threading

# Get absolute paths
EXAMPLES_DIR = os.path.dirname(os.path.abspath(__file__))
REPO_ROOT = os.path.dirname(EXAMPLES_DIR)
DOCKER_DIR = os.path.join(REPO_ROOT, 'docker')
DOCKER_SERVICE = 'stm32f4-builder'
DOCKER_WORKDIR = '/project/examples'

# Default module for u-connectXpress API
DEFAULT_UCX_MODULE = 'NORA-W36X'
# File to persist last used module selection
MODULE_CONFIG_FILE = os.path.join(EXAMPLES_DIR, '.ucx_module')

STM32_CMAKE_ARGS = (
    f'-DCMAKE_TOOLCHAIN_FILE={REPO_ROOT}/cmake/arm-none-eabi-gcc.cmake '
    '-DBUILD_STM32_EXAMPLES=ON'
)

# List of example targets: (task_name, cmake_target_base)
EXAMPLES_TASKS = [
    ('http', 'http_example'),
    ('fw-upgrade', 'fw_upgrade_example'),
    ('ble-scan', 'ble_scan_example'),
    ('ble-advertise', 'ble_advertise_example'),
    ('wifi-scan', 'wifi_scan_example'),
    ('wifi-ap', 'wifi_ap_example'),
    ('socket', 'socket_example'),
]


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


def _get_saved_module():
    """Get the last used module from config file, or DEFAULT_UCX_MODULE if not set."""
    if os.path.exists(MODULE_CONFIG_FILE):
        try:
            with open(MODULE_CONFIG_FILE, 'r') as f:
                module = f.read().strip()
                if set:
                    return module
        except Exception:
            pass
    return DEFAULT_UCX_MODULE


def _save_module(module):
    """Save the module selection to config file."""
    try:
        with open(MODULE_CONFIG_FILE, 'w') as f:
            f.write(module)
    except Exception as e:
        print(f"Warning: Could not save module selection: {e}")


def _running_inside_stm32_docker():
    """Check if running inside our STM32 Docker container."""
    return os.environ.get('UCX_STM32_DOCKER') == '1'


def _init_stm32cubef4_submodule(c):
    """Initialize STM32CubeF4 submodule if needed.

    Must be called BEFORE Docker reinvoke since Docker won't have access
    to parent repo's .git/modules when ucxclient is used as a submodule.
    """
    cube_path = os.path.join(REPO_ROOT, 'ports/extra/stm32f4/STM32CubeF4')
    if not os.path.exists(os.path.join(cube_path, 'Drivers/CMSIS')):
        print("[STM32] Initializing STM32CubeF4 submodule...")
        with c.cd(REPO_ROOT):
            c.run('git submodule update --init --depth 1 ports/extra/stm32f4/STM32CubeF4')
            c.run('cd ports/extra/stm32f4/STM32CubeF4 && git submodule update --init --depth 1 '
                  'Drivers/STM32F4xx_HAL_Driver Drivers/CMSIS Middlewares/Third_Party/FreeRTOS')


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


def _require_windows():
    """Check if running on Windows and exit with error if not."""
    if not _is_windows():
        print("Error: win32 builds are only available on Windows hosts")
        sys.exit(1)


def _require_native_linux():
    """Check if running on Linux and exit with error if not."""
    if not _is_linux():
        print("Error: linux builds are only available on Linux hosts")
        sys.exit(1)


def _reinvoke_inside_docker(c, task_label):
    """Re-run the current invoke command inside the STM32 Docker builder.

    Args:
        c: Invoke context
        task_label: Label for logging

    Environment variables:
        SKIP_DOCKER_BUILD: If set to '1', skip 'docker compose build' (useful in CI
                          where the image is pre-built with caching)
    """
    print(f">>> Reinvoking {task_label} inside Docker...")

    with c.cd(DOCKER_DIR):
        if os.environ.get('SKIP_DOCKER_BUILD') != '1':
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


def _build_target(c, target=None, clean=False, build_dir='build', cmake_args="", jobs=None):
    """Build a specific target or all targets.

    Args:
        c: Invoke context
        target: Target name to build, or None for all targets
        clean: Clean build directory before building
        build_dir: Build directory name
        cmake_args: Additional CMake configuration arguments
        jobs: Number of parallel jobs (default: number of CPU cores)
    """
    # Get the saved module selection
    module = _get_saved_module()

    build_path = os.path.join(EXAMPLES_DIR, build_dir)
    if clean and os.path.exists(build_path):
        print(f"Cleaning {build_dir}...")
        if _is_windows():
            c.run(f'rmdir /s /q {build_path}', warn=True)
        else:
            c.run(f'rm -rf {build_path}')

    # Add UCX_MODULE to cmake args
    cmake_args = f'{cmake_args} -DUCX_MODULE={module}'.strip()

    _configure_cmake(c, build_dir, cmake_args)

    # Build with parallel jobs
    if jobs is None:
        jobs = os.cpu_count() or 1
    target_name = f" --target {target}" if target else ""
    print(f"Building {target or 'all examples'} for {module} with {jobs} parallel job(s)...")
    c.run(f'cmake --build {build_path}{target_name} --parallel {jobs}')

    print(f"\nBuild completed successfully!")
    bin_dir = os.path.join(EXAMPLES_DIR, 'bin')
    if target:
        print(f"Executable: {os.path.join(bin_dir, target)}")
    else:
        print(f"Executables are located in {bin_dir}")

def _stm32_build_target(c, target=None, clean=False, docker=False, jobs=None):
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
        jobs: Number of parallel jobs (default: number of CPU cores)
    """

    _init_stm32cubef4_submodule(c)

    # User wants Docker, but we are NOT inside Docker → REINVOKE
    if docker and not _running_inside_stm32_docker():
        _reinvoke_inside_docker(c, 'stm32 build')
        return

    # Check if ARM toolchain is available
    result = c.run('which arm-none-eabi-gcc', warn=True, hide=True)
    if result.exited != 0:
        print("Error: ARM toolchain (arm-none-eabi-gcc) not found in PATH")
        print("\nTo build STM32 examples, either:")
        print("  1. Install ARM GCC toolchain locally, or")
        print("  2. Use --docker flag to build using stm32f4-builder Docker image")
        sys.exit(1)

    build_dir = 'build_stm32'

    build_path = os.path.join(EXAMPLES_DIR, build_dir)
    bin_path = os.path.join(EXAMPLES_DIR, 'bin')
    os.makedirs(build_path, exist_ok=True)
    os.makedirs(bin_path, exist_ok=True)

    # Build using native CMake
    _build_target(c, target=target, clean=clean, build_dir=build_dir,
                  cmake_args=STM32_CMAKE_ARGS, jobs=jobs)


def _stm32_clean(c):
    """Unified STM32 clean helper."""
    print("Cleaning STM32 build artifacts...")
    build_path = os.path.join(EXAMPLES_DIR, 'build_stm32')
    c.run(f'rm -rf {build_path}', warn=True)
    print("STM32 clean complete!")


def _stm32_cleanup_containers(c):
    """Kill any stale Renode containers and UART console connections."""
    # Only run docker commands if not inside docker
    if not _running_inside_stm32_docker():
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
    if build:
        _init_stm32cubef4_submodule(c)

    if not _running_inside_stm32_docker():
        _reinvoke_inside_docker(c, 'stm32.renode')
        return

    # Build first if requested
    if build:
        print(f"[Renode] Building {example} first...")
        _stm32_build_target(c, target=f"{example}_stm32.elf", clean=False)

    # Check if binary exists
    local_elf = os.path.join(EXAMPLES_DIR, f"bin/{example}_stm32.elf")
    if not os.path.exists(local_elf):
        print(f"Error: {local_elf} not found. Build first with --build flag.")
        sys.exit(1)

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
            sys.exit(1)

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


@task
def generate_config(c):
    """Generate config.local.h from template.

    Creates config.local.h with default values for CI builds.
    The defaults in the template are suitable for testing with mock/emulated hardware.
    """
    template = os.path.join(EXAMPLES_DIR, 'config.local.h.template')
    config = os.path.join(EXAMPLES_DIR, 'config.local.h')

    if os.path.exists(config):
        print(f"config.local.h already exists - skipping")
        return

    shutil.copy(template, config)
    print(f"Generated {config} from template")


@task
def ucx_module(c, set=''):
    """Show or set the target u-connectXpress module.

    The module selection persists across builds.

    Usage:
        invoke ucx-module                 # Show current module
        invoke ucx-module --set NORA-W36X # Set module

    Available modules are subdirectories in ucx_api/generated/.
    """
    # Get available modules
    generated_dir = os.path.join(REPO_ROOT, 'ucx_api', 'generated')
    available_modules = []
    if os.path.isdir(generated_dir):
        available_modules = [d for d in os.listdir(generated_dir)
                            if os.path.isdir(os.path.join(generated_dir, d))]

    if set:
        # Validate the module exists
        if set not in available_modules:
            print(f"Error: Module '{set}' not found.")
            if available_modules:
                print(f"Available modules: {', '.join(sorted(available_modules))}")
            else:
                print("No modules available in ucx_api/generated/")
            sys.exit(1)
        _save_module(set)
        print(f"Module set to: {set}")
    else:
        current = _get_saved_module()
        print(f"Current module: {current}")

    # List available modules
    if available_modules:
        print(f"Available modules: {', '.join(sorted(available_modules))}")


@task(help={
    'clean': 'Clean build directory before building',
    'jobs': 'Number of parallel jobs (default: CPU cores)',
})
def all(c, clean=False, jobs=None):
    """Build all examples."""
    _build_target(c, clean=clean, jobs=jobs)


@task(help={
    'clean': 'Clean build directory before building',
    'jobs': 'Number of parallel jobs (default: CPU cores)',
})
def http(c, clean=False, jobs=None):
    """Build http_example."""
    _build_target(c, target='http_example', clean=clean, jobs=jobs)


@task(help={
    'clean': 'Clean build directory before building',
    'jobs': 'Number of parallel jobs (default: CPU cores)',
})
def fw_upgrade(c, clean=False, jobs=None):
    """Build fw_upgrade_example."""
    _build_target(c, target='fw_upgrade_example', clean=clean, jobs=jobs)


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
    'docker': 'Build inside Docker container (no local ARM toolchain required)',
    'jobs': 'Number of parallel jobs (default: CPU cores)',
})
def stm32_all(c, clean=False, docker=False, jobs=None):
    """Build all STM32 examples."""
    _stm32_build_target(c, target=None, clean=clean, docker=docker, jobs=jobs)


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


# Windows/Linux platform tasks
@task(help={
    'clean': 'Clean build directory before building',
    'jobs': 'Number of parallel jobs (default: CPU cores)',
})
def win32_all(c, clean=False, jobs=None):
    """Build all examples for Windows (only available on Windows host)."""
    _require_windows()
    _build_target(c, target=None, clean=clean, build_dir='build', jobs=jobs)


@task(help={
    'clean': 'Clean build directory before building',
    'jobs': 'Number of parallel jobs (default: CPU cores)',
})
def linux_all(c, clean=False, jobs=None):
    """Build all examples for Linux (only available on Linux host)."""
    _require_native_linux()
    _build_target(c, target=None, clean=clean, build_dir='build', jobs=jobs)


# Task factory functions for generating per-example tasks
def _make_stm32_task(target_base):
    """Create an STM32 build task for a specific example."""
    def task_func(c, clean=False, docker=False, jobs=None):
        _stm32_build_target(c, target=f'{target_base}_stm32.elf', clean=clean, docker=docker, jobs=jobs)
    return task_func


def _make_win32_task(target_base):
    """Create a Windows build task for a specific example."""
    def task_func(c, clean=False, jobs=None):
        _require_windows()
        _build_target(c, target=target_base, clean=clean, build_dir='build', jobs=jobs)
    return task_func


def _make_linux_task(target_base):
    """Create a Linux build task for a specific example."""
    def task_func(c, clean=False, jobs=None):
        _require_native_linux()
        _build_target(c, target=target_base, clean=clean, build_dir='build', jobs=jobs)
    return task_func


# Create namespaces
ns = Collection()

# Add common tasks to root namespace
ns.add_task(generate_config, 'generate-config')
ns.add_task(ucx_module, 'ucx-module')

# STM32 platform namespace
stm32_ns = Collection('stm32')
stm32_ns.add_task(stm32_all, 'all')
stm32_ns.add_task(stm32_clean, 'clean')
stm32_ns.add_task(stm32_renode, 'renode')
stm32_ns.add_task(stm32_cleanup, 'cleanup')

# Generate STM32 tasks for each example
for task_name, target_base in EXAMPLES_TASKS:
    stm32_task = task(
        _make_stm32_task(target_base),
        help={
            'clean': 'Clean build directory before building',
            'docker': 'Build inside Docker container (no local ARM toolchain required)',
            'jobs': 'Number of parallel jobs (default: CPU cores)',
        }
    )
    stm32_task.__doc__ = f"Build {target_base} for STM32."
    stm32_ns.add_task(stm32_task, task_name)

ns.add_collection(stm32_ns)

# Windows platform namespace
if _is_windows():
    win32_ns = Collection('win32')
    win32_ns.add_task(win32_all, 'all')

    for task_name, target_base in EXAMPLES_TASKS:
        win32_task = task(
            _make_win32_task(target_base),
            help={
                'clean': 'Clean build directory before building',
                'jobs': 'Number of parallel jobs (default: CPU cores)',
            }
        )
        win32_task.__doc__ = f"Build {target_base} for Windows."
        win32_ns.add_task(win32_task, task_name)

    ns.add_collection(win32_ns)

# Linux platform namespace
if _is_linux():
    linux_ns = Collection('linux')
    linux_ns.add_task(linux_all, 'all')

    for task_name, target_base in EXAMPLES_TASKS:
        linux_task = task(
            _make_linux_task(target_base),
            help={
                'clean': 'Clean build directory before building',
                'jobs': 'Number of parallel jobs (default: CPU cores)',
            }
        )
        linux_task.__doc__ = f"Build {target_base} for Linux."
        linux_ns.add_task(linux_task, task_name)

    ns.add_collection(linux_ns)


