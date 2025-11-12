# Dockerfile for building Shallow Water Solver (C++)

FROM ubuntu:22.04

# Install dependencies
RUN apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
    build-essential cmake git ca-certificates libomp-dev \
    ocl-icd-opencl-dev opencl-headers clinfo && \
    update-ca-certificates && \
    rm -rf /var/lib/apt/lists/*

# Set workdir
WORKDIR /app

# Copy source
COPY . /app

# Build arguments (can be overridden at build time)
ARG BUILD_TYPE=Release
ARG ENABLE_OPENMP=ON
ARG BUILD_TESTS=OFF
ARG BUILD_EXAMPLES=ON
ARG ENABLE_GPU=OFF

# Configure and build
RUN mkdir -p build && cd build && \
    cmake .. \
      -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
      -DENABLE_OPENMP=${ENABLE_OPENMP} \
      -DBUILD_TESTS=${BUILD_TESTS} \
      -DBUILD_EXAMPLES=${BUILD_EXAMPLES} \
      -DENABLE_GPU=${ENABLE_GPU} \
      -DCMAKE_EXPORT_COMPILE_COMMANDS=ON && \
    cmake --build . --config ${BUILD_TYPE} -j$(nproc)

# Run tests if enabled
RUN if [ "$BUILD_TESTS" = "ON" ]; then cd build && ctest --output-on-failure --build-config ${BUILD_TYPE}; fi

# Create output directory
RUN mkdir -p /app/output

# Set working directory to where outputs will be written
WORKDIR /app/output

# Entrypoint for running the solver
ENTRYPOINT ["/app/build/bin/shallow-water-solver"]
CMD ["--help"]

# Usage:
# docker build -t shallow-water-cpp .
# docker run --rm shallow-water-cpp --help
#
# To override build options:
# docker build --build-arg BUILD_TYPE=Debug --build-arg ENABLE_OPENMP=OFF --build-arg BUILD_TESTS=OFF --build-arg ENABLE_GPU=ON -t shallow-water-cpp .
