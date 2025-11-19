"""
PyInvoke tasks for ucxclient project.

"""

from invoke import task, Collection
import os


# West workspace directory (relative to project root)
WEST_WORKSPACE = "west-workspace"


@task
def ceedling(c):
    """Run Ceedling unit tests."""
    print("Running Ceedling unit tests...")
    c.run("ceedling test:all", pty=True)


@task
def twister(c, verbose=False):
    """Run Zephyr Twister tests."""
    print("Running Zephyr Twister tests...")

    # Ensure west workspace is initialized
    init_west_workspace(c)

    verbose_flag = "-vv" if verbose else ""

    # Set ZEPHYR_BASE and ZEPHYR_EXTRA_MODULES, run from workspace
    zephyr_base = os.path.abspath(os.path.join(WEST_WORKSPACE, "zephyr"))
    ucxclient_path = os.path.abspath(".")

    with c.cd(WEST_WORKSPACE):
        c.run(f"ZEPHYR_BASE={zephyr_base} ZEPHYR_EXTRA_MODULES={ucxclient_path} "
              f"west twister -T {ucxclient_path}/zephyr/ --integration {verbose_flag}", pty=True)

def init_west_workspace(c):
    """Initialize west workspace if it doesn't exist."""
    west_dir = os.path.join(WEST_WORKSPACE, ".west")

    if not os.path.exists(west_dir):
        print(f"Initializing west workspace in {WEST_WORKSPACE}/...")

        # Create workspace directory
        os.makedirs(WEST_WORKSPACE, exist_ok=True)

        # Copy west.yml manifest to workspace
        c.run(f"cp zephyr/ci-dummy-west.yml {WEST_WORKSPACE}/west.yml")

        # Create a minimal ucxclient directory with just the manifest
        project_dir = os.path.join(WEST_WORKSPACE, "manifest")
        os.makedirs(project_dir, exist_ok=True)
        c.run(f"cp {WEST_WORKSPACE}/west.yml {project_dir}/")

        # Initialize west
        with c.cd(WEST_WORKSPACE):
            c.run("west init -l manifest", pty=True)
            c.run("west update -o=--depth=1 -n", pty=True)

        print(f"West workspace initialized in {WEST_WORKSPACE}/")


@task(pre=[ceedling, twister])
def all_tests(c):
    """Run all tests (Ceedling + Twister)."""
    print("All tests completed!")


@task
def posix(c, clean=False):
    """Build POSIX example (http_example)."""
    print("Building POSIX example...")

    build_dir = "build"
    if clean and os.path.exists(build_dir):
        c.run(f"rm -rf {build_dir}")

    if not os.path.exists(build_dir):
        os.makedirs(build_dir)

    with c.cd(build_dir):
        c.run("cmake ..", pty=True)
        c.run("cmake --build .", pty=True)

    print(f"Built executable: {build_dir}/http_example")


@task
def zephyr(c, board="qemu_x86"):
    """Build Zephyr example.

    Args:
        board: Target board (default: qemu_x86)
    """
    print(f"Building Zephyr http_example for {board}...")

    # Ensure west workspace is initialized
    init_west_workspace(c)

    # Build the example with ZEPHYR_EXTRA_MODULES pointing to ucxclient
    zephyr_base = os.path.abspath(os.path.join(WEST_WORKSPACE, "zephyr"))
    ucxclient_path = os.path.abspath(".")
    example_path = os.path.join(ucxclient_path, "zephyr", "http_example")

    with c.cd(WEST_WORKSPACE):
        c.run(f"ZEPHYR_BASE={zephyr_base} ZEPHYR_EXTRA_MODULES={ucxclient_path} "
              f"west build -b {board} -p -d {example_path}/build {example_path}", pty=True)

    print(f"Build complete: zephyr/http_example/build/zephyr/zephyr.elf")


@task(pre=[posix])
def all_examples(c):
    """Build all examples."""
    print("All examples built!")


@task
def clean_build(c):
    """Clean CMake build directory."""
    print("Cleaning build directory...")
    c.run("rm -rf build", pty=True)


@task
def clean_ceedling(c):
    """Clean Ceedling build artifacts."""
    print("Cleaning Ceedling artifacts...")
    c.run("ceedling clean", pty=True)


@task
def clean_twister(c):
    """Clean Twister output."""
    print("Cleaning Twister output...")
    c.run("rm -rf twister-out twister-out.*", pty=True)


@task
def clean_zephyr(c):
    """Clean Zephyr build."""
    print("Cleaning Zephyr build...")
    c.run("rm -rf zephyr/http_example/build", pty=True)
    c.run("rm -rf zephyr/build", pty=True)


@task
def clean_west(c):
    """Clean west workspace."""
    print(f"Cleaning west workspace ({WEST_WORKSPACE})...")
    c.run(f"rm -rf {WEST_WORKSPACE}", pty=True)


@task(pre=[clean_build, clean_ceedling, clean_twister, clean_zephyr, clean_west])
def clean_all(c):
    """Clean all build artifacts."""
    print("All build artifacts cleaned!")
# Create namespaces for better organization
test_ns = Collection('test')
test_ns.add_task(ceedling, 'ceedling')
test_ns.add_task(twister, 'twister')
test_ns.add_task(all_tests, 'all')

build_ns = Collection('build')
build_ns.add_task(posix, 'posix')
build_ns.add_task(zephyr, 'zephyr')
build_ns.add_task(all_examples, 'all')

clean_ns = Collection('clean')
clean_ns.add_task(clean_build, 'build')
clean_ns.add_task(clean_ceedling, 'ceedling')
clean_ns.add_task(clean_twister, 'twister')
clean_ns.add_task(clean_zephyr, 'zephyr')
clean_ns.add_task(clean_west, 'west')
clean_ns.add_task(clean_all, 'all')

# Create main namespace
ns = Collection()
ns.add_collection(test_ns)
ns.add_collection(build_ns)
ns.add_collection(clean_ns)
