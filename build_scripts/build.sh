#!/bin/bash
###############################################################################
# ucxclient Build Script for Linux/macOS
###############################################################################

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Get script directory and project root (parent of build_scripts)
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$( cd "${SCRIPT_DIR}/.." && pwd )"
BUILD_DIR="${PROJECT_ROOT}/build"

# Function to print colored messages
print_header() {
    echo -e "${CYAN}===================================${NC}"
    echo -e "${CYAN}$1${NC}"
    echo -e "${CYAN}===================================${NC}"
    echo
}

print_success() {
    echo -e "${GREEN}$1${NC}"
}

print_error() {
    echo -e "${RED}$1${NC}"
}

print_warning() {
    echo -e "${YELLOW}$1${NC}"
}

# Function to clean build
clean_build() {
    print_header "Cleaning Build"
    if [ -d "${BUILD_DIR}" ]; then
        echo "Removing intermediate files..."
        cd "${BUILD_DIR}"
        make clean 2>/dev/null || true
        rm -rf CMakeFiles/ cmake_install.cmake CMakeCache.txt Makefile 2>/dev/null || true
        print_success "Intermediate files cleaned"
    else
        print_warning "Build directory does not exist"
    fi
}

# Function to clean all
clean_all() {
    print_header "Cleaning All"
    if [ -d "${BUILD_DIR}" ]; then
        echo "Removing entire build directory..."
        rm -rf "${BUILD_DIR}"
        print_success "All build files removed"
    else
        print_warning "Build directory does not exist"
    fi
}

# Function to configure project
configure_project() {
    print_header "Configuring Project with CMake"
    
    # Create build directory if it doesn't exist
    if [ ! -d "${BUILD_DIR}" ]; then
        echo "Creating build directory..."
        mkdir -p "${BUILD_DIR}"
    fi
    
    cd "${BUILD_DIR}"
    
    # Run CMake configuration
    if cmake "${PROJECT_ROOT}" ; then
        print_success "Configuration successful"
        return 0
    else
        print_error "Configuration failed"
        return 1
    fi
}

# Function to build project
build_project() {
    local config=$1
    
    if [ "$config" == "Release" ]; then
        print_header "Building Release Configuration"
    elif [ "$config" == "Debug" ]; then
        print_header "Building Debug Configuration"
    else
        print_header "Building All Configurations"
    fi
    
    cd "${BUILD_DIR}"
    
    if [ -z "$config" ]; then
        # Build both Release and Debug
        if cmake --build . --config Release && cmake --build . --config Debug ; then
            print_success "Build successful"
            return 0
        else
            print_error "Build failed"
            return 1
        fi
    else
        # Build specific configuration
        if cmake --build . --config "$config" ; then
            print_success "Build successful"
            return 0
        else
            print_error "Build failed"
            return 1
        fi
    fi
}

# Function to check build outputs
check_outputs() {
    print_header "Checking Build Outputs"
    
    cd "${BUILD_DIR}"
    
    # Check for library
    if [ -f "libucxclient.so" ] || [ -f "libucxclient.dylib" ]; then
        if [ -f "libucxclient.so" ]; then
            print_success "SUCCESS: Found libucxclient.so"
        else
            print_success "SUCCESS: Found libucxclient.dylib"
        fi
    else
        print_warning "Warning: Library not found"
    fi
    
    echo
    echo "Build outputs:"
    ls -lh libucxclient.* 2>/dev/null || echo "No library files found"
    ls -lh http_example 2>/dev/null || echo "No example executable found"
}

# Function to test python wrapper
test_python_wrapper() {
    print_header "Testing Python Wrapper"
    
    # Check if Python is available
    if ! command -v python3 &> /dev/null; then
        print_warning "Python3 not found, skipping wrapper test"
        return
    fi
    
    # Test the wrapper
    cd "${PROJECT_ROOT}/examples/python_gui"
    if python3 -c "from ucx_wrapper import UcxClientWrapper; w = UcxClientWrapper(); print('Python wrapper test successful')" 2>/dev/null; then
        print_success "Python wrapper test successful"
    else
        print_warning "Python wrapper test failed or dependencies missing"
    fi
}

# Function to show usage
show_usage() {
    echo "ucxclient Build Script for Linux/macOS"
    echo
    echo "Usage:"
    echo "  ./build.sh              - Build all configurations (Release and Debug)"
    echo "  ./build.sh release      - Build Release configuration only"
    echo "  ./build.sh debug        - Build Debug configuration only"
    echo "  ./build.sh rebuild      - Clean and rebuild all"
    echo "  ./build.sh clean        - Clean intermediate files"
    echo "  ./build.sh cleanall     - Clean all including binaries"
    echo "  ./build.sh help         - Show this help message"
    echo
    echo "To run the GUI application:"
    echo "  ./launch_gui.sh"
}

# Main script logic
main() {
    local command=${1:-build}
    
    case "$command" in
        clean)
            clean_build
            ;;
        cleanall)
            clean_all
            ;;
        rebuild)
            clean_all
            if configure_project; then
                if build_project ""; then
                    check_outputs
                    test_python_wrapper
                fi
            fi
            ;;
        release)
            if [ ! -d "${BUILD_DIR}" ]; then
                configure_project || exit 1
            fi
            if build_project "Release"; then
                check_outputs
            fi
            ;;
        debug)
            if [ ! -d "${BUILD_DIR}" ]; then
                configure_project || exit 1
            fi
            if build_project "Debug"; then
                check_outputs
            fi
            ;;
        build)
            if [ ! -d "${BUILD_DIR}" ]; then
                configure_project || exit 1
            fi
            if build_project ""; then
                check_outputs
                test_python_wrapper
            fi
            ;;
        help|--help|-h)
            show_usage
            ;;
        *)
            print_error "Unknown command: $command"
            echo
            show_usage
            exit 1
            ;;
    esac
    
    print_header "Build Complete"
}

# Run main function
main "$@"
