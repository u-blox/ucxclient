"""
PyInvoke tasks for ucxclient examples.

"""

from invoke import task, Collection
import os
import platform


def _is_windows():
    """Check if running on Windows."""
    return platform.system() == "Windows"


def _configure_cmake(c, build_dir):
    """Configure CMake project if not already configured."""
    if not os.path.exists(build_dir):
        os.makedirs(build_dir)

    # Only configure if CMakeCache doesn't exist
    if not os.path.exists(os.path.join(build_dir, 'CMakeCache.txt')):
        print("Configuring CMake project...")
        with c.cd(build_dir):
            if _is_windows():
                # Use default generator on Windows (auto-detects Visual Studio version)
                c.run('cmake .. -A Win32')
            else:
                c.run('cmake ..')


def _build_target(c, target=None, clean=False):
    """Build a specific target or all targets.

    Args:
        c: Invoke context
        target: Target name to build, or None for all targets
        clean: Clean build directory before building
    """
    build_dir = 'build'

    if clean and os.path.exists(build_dir):
        print(f"Cleaning {build_dir}...")
        if _is_windows():
            c.run(f'rmdir /s /q {build_dir}', warn=True)
        else:
            c.run(f'rm -rf {build_dir}')

    _configure_cmake(c, build_dir)

    # Build
    target_name = f" --target {target}" if target else ""
    print(f"Building {target or 'all examples'}...")
    c.run(f'cmake --build {build_dir}{target_name}')

    print(f"\nBuild completed successfully!")
    if target:
        print(f"Executable: bin/{target}")
    else:
        print(f"Executables are located in bin/")


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


# Create namespace
ns = Collection()
ns.add_task(all)
ns.add_task(http)
ns.add_task(fw_upgrade, 'fw-upgrade')
ns.add_task(clean)

