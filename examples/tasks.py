"""
PyInvoke tasks for ucxclient examples.

"""

from invoke import task, Collection
import os
import platform


def _is_windows():
    """Check if running on Windows."""
    return platform.system() == "Windows"


def _is_linux():
    """Check if running on Linux."""
    return platform.system() == "Linux"


def _configure_cmake(c, build_dir, cmake_args=""):
    """Configure CMake project if not already configured."""
    if not os.path.exists(build_dir):
        os.makedirs(build_dir)

    # Only configure if CMakeCache doesn't exist
    if not os.path.exists(os.path.join(build_dir, 'CMakeCache.txt')):
        print("Configuring CMake project...")
        with c.cd(build_dir):
            if _is_windows() and not cmake_args:
                # Use default generator on Windows (auto-detects Visual Studio version)
                c.run(f'cmake .. -A Win32 {cmake_args}')
            else:
                c.run(f'cmake .. {cmake_args}')


def _build_target(c, target=None, clean=False, build_dir='build', cmake_args=""):
    """Build a specific target or all targets.

    Args:
        c: Invoke context
        target: Target name to build, or None for all targets
        clean: Clean build directory before building
        build_dir: Build directory name
        cmake_args: Additional CMake configuration arguments
    """
    if clean and os.path.exists(build_dir):
        print(f"Cleaning {build_dir}...")
        if _is_windows():
            c.run(f'rmdir /s /q {build_dir}', warn=True)
        else:
            c.run(f'rm -rf {build_dir}')

    _configure_cmake(c, build_dir, cmake_args)

    # Build
    target_name = f" --target {target}" if target else ""
    print(f"Building {target or 'all examples'}...")
    c.run(f'cmake --build {build_dir}{target_name}')

    print(f"\nBuild completed successfully!")
    if target:
        if build_dir == 'build_stm32':
            print(f"Binaries are located in {build_dir}/")
        else:
            print(f"Executable: bin/{target}")
    else:
        if build_dir == 'build_stm32':
            print(f"Binaries are located in {build_dir}/")
        else:
            print(f"Executables are located in bin/")
    
def _stm32_build_target(c, target=None, clean=False):
    """Unified STM32 build helper.

    If not inside docker it will build the docker image and re-run inside
    the stm32f4-builder container. Inside docker it configures and builds
    using CMake in examples/build_stm32.

    Args:
        target: CMake target base name (without .elf) or None for all
        clean: Whether to remove build directory first
    """
    inside_docker = os.path.exists('/.dockerenv')
    if not inside_docker:
        print("[STM32] Re-invoking inside docker container (stm32f4-builder)...")
        with c.cd('../docker'):
            c.run('docker compose build stm32f4-builder', warn=True)
            inner = []
            if clean:
                inner.append('rm -rf examples/build_stm32')
            inner.append('mkdir -p examples/build_stm32')
            inner.append('cd examples/build_stm32')
            inner.append('cmake .. -DCMAKE_TOOLCHAIN_FILE=../cmake/arm-none-eabi-gcc.cmake -DBUILD_STM32_EXAMPLES=ON')
            build_line = 'cmake --build .'
            if target:
                build_line += f' --target {target}.elf'
            inner.append(build_line)
            inner.append('echo "Docker STM32 build finished"')
            script = ' && '.join(inner)
            c.run(f'docker compose run --rm stm32f4-builder bash -c "{script}"')
        return
    # Native inside container build
    _build_target(c, target=target, clean=clean, build_dir='build_stm32',
                  cmake_args='-DCMAKE_TOOLCHAIN_FILE=../cmake/arm-none-eabi-gcc.cmake -DBUILD_STM32_EXAMPLES=ON')

def _stm32_clean(c):
    """Unified STM32 clean helper with docker auto-exec."""
    inside_docker = os.path.exists('/.dockerenv')
    if not inside_docker:
        print("[STM32] Cleaning inside docker container...")
        with c.cd('../docker'):
            c.run('docker compose build stm32f4-builder', warn=True)
            c.run('docker compose run --rm stm32f4-builder bash -c "rm -rf examples/build_stm32"', warn=True)
        return
    print("Cleaning STM32 build artifacts...")
    c.run('rm -rf build_stm32', warn=True)
    print("STM32 clean complete!")


def _stm32_renode(c, example='http_example', build=False, headless=False):
    """Run an STM32 example in Renode emulator.
    
    Args:
        example: Example name (http_example or fw_upgrade_example)
        build: Whether to build before running
        headless: Run headless (no GUI) for automated testing
    """
    script_file = f"/project/ports/extra/stm32/run_{example}.resc"
    
    # Build first if requested
    if build:
        print(f"[Renode] Building {example} first...")
        _stm32_build_target(c, target=f"{example}_stm32", clean=False)
    
    # Check if binary exists
    local_elf = f"bin/{example}_stm32.elf"
    if not os.path.exists(local_elf):
        print(f"Error: {local_elf} not found. Build first with --build flag.")
        return
    
    print(f"[Renode] Starting emulation of {example}...")
    print(f"[Renode] Script: {script_file}")
    print(f"[Renode] UART2 output will be shown in the console")
    print(f"[Renode] Press Ctrl+C to exit\n")
    
    with c.cd('../docker'):
        c.run('docker compose build stm32f4-renode', warn=True)
        
        # Always use console mode in Docker (no XWT GUI)
        renode_cmd = f'renode --disable-xwt --console -e "s @{script_file}"'
        
        c.run(f'docker compose run --rm stm32f4-renode {renode_cmd}', pty=True)


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
    if _is_windows():
        c.run('rmdir /s /q build bin 2>nul', warn=True)
    else:
        c.run('rm -rf build bin', warn=True)
    print("Clean complete!")


# Platform-specific namespaces

# STM32 platform tasks
@task(help={'clean': 'Clean build directory before building'})
def stm32_http(c, clean=False):
    """Build http_example for STM32 (auto docker)."""
    _stm32_build_target(c, target='http_example_stm32', clean=clean)


@task(help={'clean': 'Clean build directory before building'})
def stm32_fw_upgrade(c, clean=False):
    """Build fw_upgrade_example for STM32 (auto docker)."""
    _stm32_build_target(c, target='fw_upgrade_example_stm32', clean=clean)


@task(help={'clean': 'Clean build directory before building'})
def stm32_all(c, clean=False):
    """Build all STM32 examples (auto docker)."""
    _stm32_build_target(c, target=None, clean=clean)


@task
def stm32_clean(c):
    """Clean STM32 build artifacts (auto docker)."""
    _stm32_clean(c)


@task(help={
    'example': 'Example to run (http_example or fw_upgrade_example), default: http_example',
    'build': 'Build before running',
    'headless': 'Run in headless mode (no GUI)'
})
def stm32_renode(c, example='http_example', build=False, headless=False):
    """Run STM32 example in Renode emulator."""
    _stm32_renode(c, example=example, build=build, headless=headless)


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

# Legacy flat tasks
ns.add_task(all)
ns.add_task(http)
ns.add_task(fw_upgrade, 'fw-upgrade')
ns.add_task(clean)

# STM32 platform namespace
stm32_ns = Collection('stm32')
stm32_ns.add_task(stm32_http, 'http')
stm32_ns.add_task(stm32_fw_upgrade, 'fw-upgrade')
stm32_ns.add_task(stm32_all, 'all')
stm32_ns.add_task(stm32_clean, 'clean')
stm32_ns.add_task(stm32_renode, 'renode')
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

