# Quick Start Guide

## Docker Installation (Recommended - 2 minutes)

### Build the Docker Image

```bash
cd cpp
docker build -t shallow-water-cpp .
```

This builds everything automatically, including:
- Kokkos 4.2.00 (performance portability library)
- C++ solver with all dependencies
- Example programs

### Run Your First Simulation

```bash
# Run with default parameters (20×20 mesh)
docker run --rm shallow-water-cpp

# Run with larger mesh
docker run --rm shallow-water-cpp --nx 100 --ny 100 --time 2.0

# Run with Kokkos parallel execution
docker run --rm shallow-water-cpp --nx 100 --ny 100 --use-gpu
```

### Save Output Files

```bash
# Create output directory on host
mkdir -p output

# Run with volume mount to save results
docker run --rm -v $(pwd)/output:/app/output shallow-water-cpp \
  --nx 100 --ny 100 --time 5.0
```

Output VTK files will be in `output/` directory.

---

## Local Installation (Advanced Users)

### Prerequisites

**macOS:**
```bash
brew install cmake wget
```

**Linux (Ubuntu/Debian):**
```bash
sudo apt-get install cmake g++ wget
```

### Build Kokkos

```bash
cd /tmp
wget https://github.com/kokkos/kokkos/archive/refs/tags/4.2.00.tar.gz
tar xf 4.2.00.tar.gz
cd kokkos-4.2.00
mkdir build && cd build
cmake .. \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=/usr/local \
  -DKokkos_ENABLE_SERIAL=ON \
  -DKokkos_ENABLE_OPENMP=ON
make -j$(nproc)
sudo make install
```

### Build Solver

```bash
cd cpp
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
```

## First Run

### Docker (Recommended)

```bash
# Show help
docker run --rm shallow-water-cpp --help

# Run with custom mesh
docker run --rm shallow-water-cpp --nx 100 --ny 100 --time 5.0

# Run with Kokkos acceleration
docker run --rm shallow-water-cpp --nx 200 --ny 200 --use-gpu
```

### Local Build

```bash
# Run with default parameters
./build/bin/shallow-water-solver

# Run with custom mesh
./build/bin/shallow-water-solver --nx 100 --ny 100 --time 5.0

# Check Kokkos backend
./build/bin/shallow-water-solver --help
```

Output files will be in the `output/` directory.

## Visualize Results

### Using ParaView

1. Download ParaView: https://www.paraview.org/download/
2. Open ParaView
3. File → Open → Select `output/solution_*.vtk`
4. Click "Apply"
5. Color by "depth" or "velocity_magnitude"
6. Press Play to animate

### Quick Plot (Python)

```python
import numpy as np
import matplotlib.pyplot as plt
from vtk import vtkUnstructuredGridReader

reader = vtkUnstructuredGridReader()
reader.SetFileName("output/solution_000000.vtk")
reader.Update()

# Extract and plot depth
output = reader.GetOutput()
depth = output.GetCellData().GetArray("depth")

plt.hist([depth.GetValue(i) for i in range(depth.GetNumberOfTuples())])
plt.xlabel('Water Depth (m)')
plt.ylabel('Frequency')
plt.title('Depth Distribution')
plt.show()
```

## Run Examples

### Docker

```bash
# Dam break (20 seconds × 10 meters)
docker run --rm shallow-water-cpp /app/build/bin/dam_break

# Radial dam (circular wave)
docker run --rm shallow-water-cpp /app/build/bin/radial_dam

# Tsunami simulation
docker run --rm shallow-water-cpp /app/build/bin/tsunami
```

### Local Build

```bash
# Dam break
./build/bin/dam_break

# Radial dam  
./build/bin/radial_dam

# Tsunami
./build/bin/tsunami
```

## Run Tests

### Docker

Tests run automatically during build when enabled:
```bash
docker build --build-arg BUILD_TESTS=ON -t shallow-water-cpp-test .
# Tests execute during build - check output for results
# Expected: 26/27 tests passing
```

### Local Build

```bash
cd build
ctest --output-on-failure
```

## Common Issues

### Docker Issues

**"Cannot connect to Docker daemon"**
Start Docker Desktop or Docker daemon.

**"No space left on device"**
```bash
docker system prune -a
```

### Local Build Issues

### "cmake: command not found"
Install CMake: `brew install cmake` (macOS) or `sudo apt install cmake` (Linux)

### "Could not find package Kokkos"
Kokkos is not installed. Either:
- Use Docker (recommended)
- Install Kokkos following instructions above

### Tests failing
The solver uses strict error handling. One numerical precision test may be skipped on some platforms (expected).

## Performance Expectations

### Docker Performance

Mesh Size | Steps/sec | Simulation Speed
----------|-----------|------------------
20×20     | 900+      | Real-time
100×100   | 230+      | ~10× real-time  
200×200   | 60+       | ~3× real-time

**Note**: Kokkos OpenMP backend provides excellent multi-core CPU performance!

## Next Steps

- Read [API.md](docs/API.md) for detailed API documentation
- Read [EXAMPLES.md](docs/EXAMPLES.md) for creating custom scenarios
- Check [README.md](README.md) for full documentation

## Performance Tuning

### Optimal mesh size
- Small tests: 20×20 (< 1 second, ~900 steps/sec)
- Medium simulations: 100×100 (~4 seconds for 2s sim, ~230 steps/sec)
- Large simulations: 200×200 (~15 seconds for 2s sim, ~60 steps/sec)
- Very large: 500×500 (minutes, production runs)

### CFL number
- Stable: 0.3-0.5 (recommended)
- Fast but risky: 0.8-0.9
- Default: 0.5

### Kokkos Backends

The Docker build uses **Kokkos OpenMP** by default, which automatically utilizes all CPU cores.

To check which backend is active:
```bash
docker run --rm shallow-water-cpp --help
# Look for "Kokkos Backend: Kokkos OpenMP"
```

For GPU acceleration, you would need to rebuild Kokkos with CUDA/HIP/SYCL enabled (advanced).

## Help

```bash
./build/bin/shallow-water-solver --help
```

For issues, check the README or open a GitHub issue.
