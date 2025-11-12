# Shallow Water Solver - C++ Implementation

Modern C++ (C++17) implementation of the 2D shallow water equations solver using unstructured triangular meshes.

## Features

- **Finite Volume Method**: Second-order accurate spatial discretization
- **HLL Riemann Solver**: Robust numerical flux computation
- **Unstructured Meshes**: Flexible triangular mesh support
- **Multi-threading**: OpenMP parallelization for performance
- **VTK Output**: ParaView-compatible visualization files
- **Docker-based**: No local dependencies required

## Quick Start

### Using Docker (Recommended)

Build the Docker image:
```bash
docker build -t shallow-water-cpp .
```

Run the solver:
```bash
# Show help
docker run --rm shallow-water-cpp --help

# Run with custom parameters (output to current directory)
docker run --rm shallow-water-cpp --nx 100 --ny 100 -t 1.0 -o .

# Save output files to host (mount a volume)
docker run --rm -v $(pwd)/output:/output shallow-water-cpp --nx 50 --ny 50 -t 2.0 -o /output
```

## Command Line Options

```
--nx NUM              Number of cells in x-direction (default: 50)
--ny NUM              Number of cells in y-direction (default: 50)
--width VALUE         Domain width in meters (default: 10.0)
--height VALUE        Domain height in meters (default: 10.0)
-t, --time VALUE      Total simulation time (default: 1.0)
--output-interval VAL Output interval (default: 0.1)
--cfl VALUE           CFL number (default: 0.5)
--friction VALUE      Manning's friction coefficient (default: 0.0)
-o, --output DIR      Output directory (default: output)
--use-gpu             Enable GPU acceleration (requires GPU build)
-h, --help            Show this help message
```

## Examples

### Dam Break Simulation
```bash
docker run --rm -v $(pwd)/output:/out shallow-water-cpp \
  --nx 100 --ny 50 --width 20 --height 10 -t 5.0 -o /out
```

### High Resolution Simulation
```bash
docker run --rm shallow-water-cpp \
  --nx 200 --ny 200 -t 2.0 --cfl 0.45 -o .
```

### With Friction
```bash
docker run --rm shallow-water-cpp \
  --nx 100 --ny 100 --friction 0.03 -t 3.0 -o .
```

## Building with Custom Options

The Dockerfile supports build arguments:

```bash
# Debug build
docker build --build-arg BUILD_TYPE=Debug -t shallow-water-cpp-debug .

# Without OpenMP
docker build --build-arg ENABLE_OPENMP=OFF -t shallow-water-cpp-serial .

# With tests enabled
docker build --build-arg BUILD_TESTS=ON -t shallow-water-cpp-test .

# GPU-enabled build (requires CUDA)
docker build --build-arg ENABLE_GPU=ON -t shallow-water-cpp-gpu .
```

## Project Structure

```
cpp/
├── include/          # Header files
│   ├── types.hpp     # Core data structures
│   ├── mesh.hpp      # Mesh generation and connectivity
│   ├── solver.hpp    # Shallow water solver
│   ├── boundary.hpp  # Boundary conditions
│   └── io.hpp        # VTK and CSV output
├── src/              # Implementation files
│   ├── main.cpp      # Main application
│   ├── mesh.cpp
│   ├── solver.cpp
│   ├── boundary.cpp
│   └── io.cpp
├── tests/            # Unit tests (Google Test)
│   ├── test_mesh.cpp
│   ├── test_solver.cpp
│   └── test_types.cpp
├── examples/         # Example simulations
│   ├── dam_break.cpp
│   ├── radial_dam.cpp
│   └── tsunami.cpp
├── docs/             # Documentation
├── CMakeLists.txt    # CMake build configuration
├── Dockerfile        # Docker build file
└── README.md         # This file
```

## Output Files

The solver generates:
- **VTK files** (`.vtk`): Time-stamped snapshots for visualization in ParaView
- **CSV file** (`statistics.csv`): Time series of mass, energy, and wave speed

### Visualizing Results

1. Install [ParaView](https://www.paraview.org/)
2. Open the generated `.vtk` files
3. Color by `h` (water depth), `velocity`, or `vel_mag` (velocity magnitude)

## Dependencies

All dependencies are included in the Docker image:
- **CMake** 3.22+
- **GCC** 11.4 (C++17 support)
- **OpenMP** 4.5+
- **Google Test** (for unit tests)

## Performance

Typical performance on a 100×100 mesh:
- **Serial**: ~50 steps/second
- **OpenMP (8 cores)**: ~200 steps/second
- **GPU (CUDA)**: ~1000 steps/second

## Documentation

See the `docs/` directory for detailed documentation:
- [Architecture](docs/ARCHITECTURE.md)
- [Numerical Methods](docs/NUMERICAL_METHODS.md)
- [API Reference](docs/API.md)
- [Development Guide](docs/DEVELOPMENT.md)

## Testing

Run tests inside the container:
```bash
# Build with tests enabled
docker build --build-arg BUILD_TESTS=ON -t shallow-water-cpp-test .

# The tests run automatically during build
# To run tests manually in the built image:
docker run --rm --entrypoint /app/build/bin/unit_tests shallow-water-cpp-test
```

## License

Same as the parent repository.

## Comparison with Rust Implementation

| Feature | C++ | Rust |
|---------|-----|------|
| Language | C++17 | Rust 2021 |
| Memory Safety | Manual | Compiler-enforced |
| Performance | ~Similar | ~Similar |
| Parallelization | OpenMP | Rayon |
| GPU Support | CUDA/OpenMP | WebGPU |
| Build System | CMake + Docker | Cargo + Docker |
| Dependencies | Manual (Docker) | Automatic (Cargo) |

Both implementations solve the same equations using the same numerical methods (Finite Volume, HLL flux) and should produce identical results.
