"""
PyInvoke tasks for ucxclient project.

"""

from invoke import task, Collection
import os


@task
def ceedling(c):
    """Run Ceedling unit tests."""
    print("Running Ceedling unit tests...")
    c.run("ceedling test:all")


@task
def examples(c, clean=False):
    """Build examples (delegates to examples/tasks.py)."""
    print("Building examples...")
    clean_flag = "--clean" if clean else ""
    with c.cd("examples"):
        c.run(f"invoke all {clean_flag}")


@task
def clean_ceedling(c):
    """Clean Ceedling build artifacts."""
    print("Cleaning Ceedling artifacts...")
    c.run("ceedling clean")


@task
def clean_examples(c):
    """Clean examples build artifacts."""
    print("Cleaning examples...")
    with c.cd("examples"):
        c.run("invoke clean")


@task(pre=[clean_ceedling, clean_examples])
def clean_all(c):
    """Clean all build artifacts."""
    print("All build artifacts cleaned!")


# Create namespaces for better organization
test_ns = Collection('test')
test_ns.add_task(ceedling, 'ceedling')

build_ns = Collection('build')
build_ns.add_task(examples, 'examples')

clean_ns = Collection('clean')
clean_ns.add_task(clean_ceedling, 'ceedling')
clean_ns.add_task(clean_examples, 'examples')
clean_ns.add_task(clean_all, 'all')

# Create main namespace
ns = Collection()
ns.add_collection(test_ns)
ns.add_collection(build_ns)
ns.add_collection(clean_ns)
