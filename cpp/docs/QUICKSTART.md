# Quick Start Guide

## Installation (5 minutes)

### Prerequisites

**macOS:**
```bash
brew install cmake libomp
```

**Linux (Ubuntu/Debian):**
```bash
sudo apt-get install cmake g++ libomp-dev
```

**Windows:**
- Install Visual Studio 2022 with C++ support
- Install CMake from https://cmake.org/download/

### Build

```bash
cd cpp
./build.sh
```

Or manually:
```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -DENABLE_OPENMP=ON ..
make -j$(nproc)
```

## First Run (1 minute)

```bash
# Run with default parameters
./build/bin/shallow-water-solver

# Run with custom mesh
./build/bin/shallow-water-solver --nx 100 --ny 100 -t 5.0
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

```bash
# Dam break (20 seconds × 10 meters)
./build/bin/dam_break

# Radial dam (circular wave)
./build/bin/radial_dam

# Tsunami simulation
./build/bin/tsunami
```

## Run Tests

```bash
cd build
ctest --output-on-failure
```

## Common Issues

### "cmake: command not found"
Install CMake: `brew install cmake` (macOS) or `sudo apt install cmake` (Linux)

### "Could not find OpenMP"
- macOS: `brew install libomp`
- Linux: `sudo apt install libomp-dev`
- Or build without OpenMP: `cmake -DENABLE_OPENMP=OFF ..`

### Tests failing
Try building in Debug mode:
```bash
./build.sh --clean --debug
```

## Next Steps

- Read [API.md](docs/API.md) for detailed API documentation
- Read [EXAMPLES.md](docs/EXAMPLES.md) for creating custom scenarios
- Check [README.md](README.md) for full documentation

## Performance Tuning

### Optimal mesh size
- Small tests: 50×50 (instant)
- Medium simulations: 100×100 (seconds)
- Large simulations: 500×500 (minutes)

### CFL number
- Stable: 0.3-0.5
- Fast but risky: 0.8-0.9

### OpenMP
Automatically uses all CPU cores. To limit:
```bash
export OMP_NUM_THREADS=4
./build/bin/shallow-water-solver
```

## Help

```bash
./build/bin/shallow-water-solver --help
```

For issues, check the README or open a GitHub issue.
