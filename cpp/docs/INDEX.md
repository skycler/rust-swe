# Shallow Water Solver - C++ Version

Modern C++20 implementation of the 2D Shallow Water Equations solver.

## Quick Links

- [README](README.md) - Full documentation
- [Quick Start](docs/QUICKSTART.md) - Get started in 5 minutes
- [API Documentation](docs/API.md) - Complete API reference
- [Examples](docs/EXAMPLES.md) - Example scenarios and tutorials

## Features

✅ Modern C++20 with clean, efficient code  
✅ Finite Volume Method on triangular meshes  
✅ HLL Riemann solver for robust flux computation  
✅ OpenMP parallelization for multi-core CPUs  
✅ Comprehensive unit tests with Google Test  
✅ VTK output for visualization  
✅ Multiple example scenarios  
✅ CI/CD with GitHub Actions  

## Quick Build

```bash
cd cpp
./build.sh
./build/bin/shallow-water-solver --nx 100 --ny 100 -t 10
```

## Project Structure

```
cpp/
├── include/           # Header files (.hpp)
├── src/              # Implementation files (.cpp)
├── tests/            # Unit tests (Google Test)
├── examples/         # Example programs
├── docs/             # Documentation
├── CMakeLists.txt    # CMake configuration
└── build.sh          # Build script
```

## Requirements

- C++20 compiler (GCC 11+, Clang 14+, MSVC 2022+)
- CMake 3.20+
- OpenMP (optional, recommended)

## License

MIT License
