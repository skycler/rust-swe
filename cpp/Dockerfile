# Dockerfile for building Shallow Water Solver (C++)

FROM ubuntu:22.04

# Install dependencies
RUN apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
    build-essential cmake git ca-certificates wget && \
    update-ca-certificates && \
    rm -rf /var/lib/apt/lists/*

# Build and install Kokkos
ARG KOKKOS_VERSION=4.2.00
RUN cd /tmp && \
    wget https://github.com/kokkos/kokkos/archive/refs/tags/${KOKKOS_VERSION}.tar.gz && \
    tar xf ${KOKKOS_VERSION}.tar.gz && \
    cd kokkos-${KOKKOS_VERSION} && \
    mkdir build && cd build && \
    cmake .. \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INSTALL_PREFIX=/usr/local \
      -DKokkos_ENABLE_SERIAL=ON \
      -DKokkos_ENABLE_OPENMP=ON \
      -DKokkos_ENABLE_AGGRESSIVE_VECTORIZATION=ON && \
    make -j$(nproc) && \
    make install && \
    cd / && rm -rf /tmp/kokkos-${KOKKOS_VERSION} /tmp/${KOKKOS_VERSION}.tar.gz

# Set workdir
WORKDIR /app

# Copy source
COPY . /app

# Build arguments (can be overridden at build time)
ARG BUILD_TYPE=Release
ARG BUILD_TESTS=OFF
ARG BUILD_EXAMPLES=ON

# Configure and build
RUN mkdir -p build && cd build && \
    cmake .. \
      -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
      -DBUILD_TESTS=${BUILD_TESTS} \
      -DBUILD_EXAMPLES=${BUILD_EXAMPLES} \
      -DCMAKE_EXPORT_COMPILE_COMMANDS=ON && \
    cmake --build . --config ${BUILD_TYPE} -j$(nproc)

# Run tests if enabled
RUN if [ "$BUILD_TESTS" = "ON" ]; then cd build && ctest --output-on-failure --build-config ${BUILD_TYPE}; fi

# Create output directory (application writes to ./output by default)
RUN mkdir -p /app/output

# Stay in /app so relative paths work correctly
WORKDIR /app

# Entrypoint for running the solver
ENTRYPOINT ["/app/build/bin/shallow-water-solver"]
CMD ["--help"]

# Usage:
# docker build -t shallow-water-cpp .
# docker run --rm shallow-water-cpp --help
#
# To override build options:
# docker build --build-arg BUILD_TYPE=Debug --build-arg BUILD_TESTS=OFF -t shallow-water-cpp .
