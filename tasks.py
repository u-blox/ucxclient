"""
PyInvoke tasks for ucxclient project.

"""

from invoke import task, Collection
import os
import sys
import importlib.util

# Get repository root directory
REPO_ROOT = os.path.dirname(os.path.abspath(__file__))

# Import examples tasks
examples_tasks_path = os.path.join(REPO_ROOT, 'examples', 'tasks.py')
spec = importlib.util.spec_from_file_location("examples_tasks", examples_tasks_path)
examples_tasks = importlib.util.module_from_spec(spec)
spec.loader.exec_module(examples_tasks)
examples_ns = examples_tasks.ns

# West workspace directory (absolute path)
WEST_WORKSPACE = os.path.join(REPO_ROOT, "west-workspace")


def init_west_workspace(c):
    """Initialize west workspace if it doesn't exist."""
    west_dir = os.path.join(WEST_WORKSPACE, ".west")

    if not os.path.exists(west_dir):
        print(f"Initializing west workspace in {WEST_WORKSPACE}/...")

        # Create workspace directory
        os.makedirs(WEST_WORKSPACE, exist_ok=True)

        # Copy west.yml manifest to workspace
        zephyr_manifest = os.path.join(REPO_ROOT, "zephyr/ci-dummy-west.yml")
        west_yml = os.path.join(WEST_WORKSPACE, "west.yml")
        c.run(f"cp {zephyr_manifest} {west_yml}")

        # Create a minimal ucxclient directory with just the manifest
        project_dir = os.path.join(WEST_WORKSPACE, "manifest")
        os.makedirs(project_dir, exist_ok=True)
        c.run(f"cp {west_yml} {project_dir}/")

        # Initialize west
        with c.cd(WEST_WORKSPACE):
            c.run("west init -l manifest", pty=True)
            c.run("west update -o=--depth=1 -n", pty=True)

        print(f"West workspace initialized in {WEST_WORKSPACE}/")


@task
def ceedling(c):
    """Run Ceedling unit tests."""
    print("Running Ceedling unit tests...")
    c.run("ceedling test:all")


@task
def zephyr(c, verbose=False):
    """Run Zephyr Twister tests."""
    print("Running Zephyr Twister tests...")

    # Ensure west workspace is initialized
    init_west_workspace(c)

    verbose_flag = "-vv" if verbose else ""

    # Set ZEPHYR_BASE and ZEPHYR_EXTRA_MODULES, run from workspace
    zephyr_base = os.path.join(WEST_WORKSPACE, "zephyr")
    zephyr_tests = os.path.join(REPO_ROOT, "zephyr")

    with c.cd(WEST_WORKSPACE):
        c.run(f"ZEPHYR_BASE={zephyr_base} ZEPHYR_EXTRA_MODULES={REPO_ROOT} "
              f"west twister -T {zephyr_tests}/ --integration {verbose_flag}", pty=True)


@task
def clean_ceedling(c):
    """Clean Ceedling build artifacts."""
    print("Cleaning Ceedling artifacts...")
    c.run("ceedling clean")


@task
def clean_zephyr(c):
    """Clean Zephyr/Twister artifacts."""
    print("Cleaning Zephyr/Twister artifacts...")
    c.run(f"rm -rf {os.path.join(WEST_WORKSPACE, 'twister-out')} {os.path.join(WEST_WORKSPACE, 'twister-out.*')}", pty=True, warn=True)
    c.run(f"rm -rf {os.path.join(REPO_ROOT, 'zephyr/http_example/build')}", pty=True, warn=True)
    c.run(f"rm -rf {os.path.join(REPO_ROOT, 'zephyr/build')}", pty=True, warn=True)


@task
def clean_west(c):
    """Clean west workspace."""
    print(f"Cleaning west workspace ({WEST_WORKSPACE})...")
    c.run(f"rm -rf {WEST_WORKSPACE}", pty=True)


# Create namespaces for better organization
# Ceedling sub-collection under test
ceedling_ns = Collection('ceedling')
ceedling_ns.add_task(ceedling, 'run')
ceedling_ns.add_task(clean_ceedling, 'clean')

# Zephyr sub-collection under test
zephyr_ns = Collection('zephyr')
zephyr_ns.add_task(zephyr, 'run')
zephyr_ns.add_task(clean_zephyr, 'clean')
zephyr_ns.add_task(clean_west, 'clean-west')

# Test namespace with sub-collections
test_ns = Collection('test')
test_ns.add_collection(ceedling_ns)
test_ns.add_collection(zephyr_ns)

# Create main namespace
ns = Collection()
ns.add_collection(test_ns)
ns.add_collection(examples_ns, 'examples')
