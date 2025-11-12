# Shallow Water Solver - C++ Version

Modern C++20 implementation of the 2D Shallow Water Equations solver with **Kokkos** performance portability.

## Quick Links

- [README](../README.md) - Full documentation
- [Quick Start](QUICKSTART.md) - Get started in 2 minutes with Docker
- [API Documentation](API.md) - Complete API reference
- [Examples](EXAMPLES.md) - Example scenarios and tutorials

## Features

✅ Modern C++20 with clean, efficient code  
✅ **Kokkos 4.2.00** for performance portability (CPU/GPU)  
✅ Finite Volume Method on triangular meshes  
✅ HLL Riemann solver for robust flux computation  
✅ Single-source code for all backends (OpenMP, Serial, CUDA, HIP, SYCL)  
✅ **4.6× faster than Rust** implementation  
✅ Comprehensive unit tests (26/27 passing)  
✅ VTK output for visualization  
✅ Docker containerization - no local dependencies  
✅ Multiple example scenarios  

## Quick Start

### Docker (Recommended)

```bash
cd cpp
docker build -t shallow-water-cpp .
docker run --rm shallow-water-cpp --nx 100 --ny 100 --time 2.0
```

### Performance

- **20×20 mesh**: 900+ steps/sec (real-time)
- **100×100 mesh**: 233 steps/sec (~10× real-time)
- **200×200 mesh**: 60+ steps/sec (~3× real-time)

## Project Structure

```
cpp/
├── include/           # Header files (.hpp)
│   ├── gpu_solver.hpp # Kokkos GPU/parallel solver
│   └── ...
├── src/              # Implementation files (.cpp)
│   ├── gpu_solver.cpp # Kokkos implementation
│   └── ...
├── tests/            # Unit tests (Google Test)
├── examples/         # Example programs
├── docs/             # Documentation
├── CMakeLists.txt    # CMake configuration
└── Dockerfile        # Docker build (includes Kokkos)
```

## Technology Stack

- **Language**: C++20
- **Performance Library**: Kokkos 4.2.00
- **Parallelization**: Kokkos OpenMP backend (CPU multi-threading)
- **GPU Support**: Via Kokkos (CUDA, HIP, SYCL) - optional
- **Build System**: CMake 3.20+
- **Testing**: Google Test
- **Containerization**: Docker

## Requirements

### Docker (Recommended)
- Docker Desktop or Docker Engine

### Local Build (Advanced)
- C++20 compiler (GCC 11+, Clang 14+)
- CMake 3.20+
- Kokkos 4.2.00 (build from source)

## License

MIT License
