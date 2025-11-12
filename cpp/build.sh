#!/bin/bash

# Build script for Shallow Water Solver (C++)

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}================================${NC}"
echo -e "${GREEN}Building Shallow Water Solver${NC}"
echo -e "${GREEN}================================${NC}"
echo

# Parse arguments
BUILD_TYPE="Release"
ENABLE_OPENMP="ON"
BUILD_TESTS="ON"
BUILD_EXAMPLES="ON"
ENABLE_GPU="OFF"
CLEAN=false

while [[ $# -gt 0 ]]; do
    case $1 in
        --debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        --no-openmp)
            ENABLE_OPENMP="OFF"
            shift
            ;;
        --no-tests)
            BUILD_TESTS="OFF"
            shift
            ;;
        --no-examples)
            BUILD_EXAMPLES="OFF"
            shift
            ;;
        --gpu)
            ENABLE_GPU="ON"
            shift
            ;;
        --clean)
            CLEAN=true
            shift
            ;;
        *)
            echo -e "${RED}Unknown option: $1${NC}"
            exit 1
            ;;
    esac
done

# Clean if requested
if [ "$CLEAN" = true ]; then
    echo -e "${YELLOW}Cleaning build directory...${NC}"
    rm -rf build
fi

# Create build directory
mkdir -p build
cd build

# Configure
echo -e "${YELLOW}Configuring CMake...${NC}"
cmake .. \
    -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
    -DENABLE_OPENMP=$ENABLE_OPENMP \
    -DBUILD_TESTS=$BUILD_TESTS \
    -DBUILD_EXAMPLES=$BUILD_EXAMPLES \
    -DENABLE_GPU=$ENABLE_GPU \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

echo

# Build
echo -e "${YELLOW}Building...${NC}"
cmake --build . --config $BUILD_TYPE -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

echo
echo -e "${GREEN}================================${NC}"
echo -e "${GREEN}Build complete!${NC}"
echo -e "${GREEN}================================${NC}"
echo
echo -e "Build type: ${YELLOW}$BUILD_TYPE${NC}"
echo -e "OpenMP: ${YELLOW}$ENABLE_OPENMP${NC}"
echo -e "Tests: ${YELLOW}$BUILD_TESTS${NC}"
echo -e "Examples: ${YELLOW}$BUILD_EXAMPLES${NC}"
echo -e "GPU: ${YELLOW}$ENABLE_GPU${NC}"
echo

# Run tests if enabled
if [ "$BUILD_TESTS" = "ON" ]; then
    echo -e "${YELLOW}Running tests...${NC}"
    ctest --output-on-failure --build-config $BUILD_TYPE
    echo
fi

echo -e "${GREEN}Executables are in: build/bin/${NC}"
echo
echo -e "${YELLOW}To run the solver:${NC}"
echo -e "  ./build/bin/shallow-water-solver --help"
echo
