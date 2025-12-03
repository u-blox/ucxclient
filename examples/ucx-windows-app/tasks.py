"""
PyInvoke tasks for ucx-windows-app.

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
                c.run('cmake .. -A x64')
            else:
                c.run('cmake ..')


def _build_target(c, target=None, clean=False):
    """Build a specific target or all targets.

    Args:
        c: Invoke context
        target: Target name to build, or None for all targets
        clean: Clean build directory before building
    """
    build_dir = '../../build'

    if clean and os.path.exists(build_dir):
        print(f"Cleaning {build_dir}...")
        if _is_windows():
            c.run(f'rmdir /s /q {build_dir}', warn=True)
        else:
            c.run(f'rm -rf {build_dir}')

    _configure_cmake(c, build_dir)

    # Build
    target_name = f" --target {target}" if target else ""
    print(f"Building {target or 'ucx-windows-app'}...")
    c.run(f'cmake --build {build_dir}{target_name}')

    print(f"\nBuild completed successfully!")
    if target:
        print(f"Executable: bin/{target}.exe")
    else:
        print(f"Executable: bin/ucx-windows-app.exe")


@task(help={'clean': 'Clean build directory before building'})
def build(c, clean=False):
    """Build ucx-windows-app."""
    _build_target(c, target='ucx-windows-app', clean=clean)


@task
def clean(c):
    """Clean all build artifacts."""
    print("Cleaning build artifacts...")
    if _is_windows():
        c.run('rmdir /s /q ..\\..\\build 2>nul', warn=True)
        c.run('rmdir /s /q bin 2>nul', warn=True)
    else:
        c.run('rm -rf ../../build', warn=True)
        c.run('rm -rf bin', warn=True)
    print("Clean complete!")


# Create namespace
ns = Collection()
ns.add_task(build)
ns.add_task(clean)
