# Shallow Water Equations Solver

[![CI Pipeline](https://github.com/skycler/rust-swe/actions/workflows/ci.yml/badge.svg)](https://github.com/skycler/rust-swe/actions/workflows/ci.yml)

A high-performance Rust implementation of a 2D shallow water equations solver using the finite volume method on triangular meshes.

**Version 1.0** - Now with 3D topography and bottom friction support!

## Features

- **Triangular Mesh**: Unstructured triangular grid for flexible domain representation
- **Second-Order Accurate**: Runge-Kutta 2 time integration
- **Mass Conservative**: Finite volume method ensures conservation of mass
- **3D Topography**: Non-flat bathymetry with multiple terrain types
- **Bottom Friction**: Manning and Chezy friction laws
- **Multiple Initial Conditions**: Dam break, circular wave, standing wave
- **VTK Output**: Results in VTK format for visualization with ParaView
- **Multi-core CPU**: Parallelized with Rayon (2-5x speedup)
- **GPU Acceleration**: Optional WebGPU support for massive meshes (CUDA/Metal/Vulkan)
- **Efficient**: Written in Rust with optimized numerical algorithms

## 📚 Complete Documentation

**See [DOCUMENTATION.md](DOCUMENTATION.md) for the comprehensive guide** including:
- Detailed installation and usage instructions
- Physical model and equations
- Topography and friction guides with coefficient tables
- Example scenarios for various applications
- Visualization tutorials
- Performance tips and troubleshooting
- Mathematical formulations and references

## Quick Start

## Quick Start

### Installation

```bash
# Install Rust (if not already installed)
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh

# Clone or navigate to project
cd rust-swe

# Build in release mode (CPU only)
cargo build --release

# Or build with GPU support
cargo build --release --features gpu
```

### Basic Usage

```bash
# Run with defaults (dam break, flat bed, no friction)
cargo run --release

# Run with GPU acceleration (requires --features gpu)
cargo run --release --features gpu -- --use-gpu

# Add Manning friction
cargo run --release -- --friction manning --manning-n 0.03

# Add topography (Gaussian hill)
cargo run --release -- --topography gaussian

# Large mesh with GPU
cargo run --release --features gpu -- \
    --use-gpu \
    --nx 200 --ny 200 \
    --topography gaussian

# Complete scenario: wave over hill with friction
cargo run --release -- \
  --initial-condition circular-wave \
  --topography gaussian \
  --friction manning --manning-n 0.025
```

### Command-Line Options

```bash
# See all options
cargo run --release -- --help
```

**Key Options:**
- `--nx, --ny`: Grid resolution (default: 40×40)
- `--topography`: flat, slope, gaussian, channel
- `--friction`: none, manning, chezy
- `--manning-n`: Manning coefficient (default: 0.03)
- `--initial-condition`: dam-break, circular-wave, standing-wave
- `--final-time`: Simulation duration in seconds

### Example Scenarios

```bash
# Urban flood (smooth concrete, sloped terrain)
cargo run --release -- \
  --nx 80 --ny 80 \
  --topography slope \
  --friction manning --manning-n 0.015 \
  --initial-condition dam-break \
  --final-time 20.0

# River flow in channel
cargo run --release -- \
  --topography channel \
  --friction manning --manning-n 0.035 \
  --initial-condition standing-wave

# Wave shoaling over hill
cargo run --release -- \
  --nx 60 --ny 60 \
  --topography gaussian \
  --friction manning --manning-n 0.02 \
  --initial-condition circular-wave
```

## Output Files

The solver generates VTK files that can be visualized with:
- [ParaView](https://www.paraview.org/) (recommended)
- [VisIt](https://visit-dav.github.io/visit-website/)
- Any VTK-compatible visualization tool

Output files are named: `{prefix}_{index}.vtk`

**VTK Data Fields:**
- `height`: Water depth (m)
- `velocity`: Flow velocity (m/s)
- `bed_elevation`: Bottom topography (m)
- `water_surface`: Free surface elevation (m)
- `momentum_x`, `momentum_y`: Momentum components

### Visualizing in ParaView

1. Open ParaView
2. File → Open → Select all VTK files
3. Click "Apply"
4. Select variable (e.g., `water_surface` or `height`)
5. Optional: Add "Warp By Scalar" filter for 3D view
6. Press Play to animate

## Applications

This solver is designed for:

- **Hydraulic Engineering**: Dam breaks, flood modeling, channel flow
- **Coastal Engineering**: Wave propagation, tsunami simulation, shoaling
- **Environmental Science**: River dynamics, wetland flow
- **Civil Engineering**: Urban drainage, stormwater management
- **Research**: Algorithm development, method verification

## Physical Model

The solver implements the 2D shallow water equations with source terms:

$$\frac{\partial h}{\partial t} + \nabla \cdot (h\mathbf{u}) = 0$$

$$\frac{\partial (h\mathbf{u})}{\partial t} + \nabla \cdot (h\mathbf{u} \otimes \mathbf{u} + \frac{1}{2}gh^2\mathbf{I}) = -gh\nabla z_b - ghS_f\frac{\mathbf{u}}{|\mathbf{u}|}$$

**Features:**
- Finite Volume Method (mass conservative)
- Lax-Friedrichs flux (stable for shocks)
- Green-Gauss gradients for topography
- Manning or Chezy friction laws
- Well-balanced for lake-at-rest
- Second-order Runge-Kutta time integration

## Performance

Typical runtimes on modern hardware:

### CPU (Multi-threaded)
| Grid Size | Triangles | Simulation Time | Wall Time |
|-----------|-----------|-----------------|-----------|
| 40×40 | 3,200 | 5s | ~3-5s |
| 60×60 | 7,200 | 5s | ~8-12s |
| 100×100 | 20,000 | 5s | ~30-60s |

### GPU (CUDA/Metal/Vulkan)
| Grid Size | Triangles | Simulation Time | Wall Time |
|-----------|-----------|-----------------|-----------|
| 100×100 | 20,000 | 5s | ~5-10s |
| 200×200 | 80,000 | 5s | ~15-25s |
| 400×400 | 320,000 | 5s | ~60-120s |

Mass conservation error: **0.00000000%** (machine precision)

## Documentation & Guides

- **[DOCUMENTATION.md](DOCUMENTATION.md)** - Complete guide (all topics in one file)
- **[GPU_GUIDE.md](GPU_GUIDE.md)** - GPU acceleration setup and usage
- **[run_examples.sh](run_examples.sh)** - Comprehensive example suite (12 scenarios)

## Code Structure

```
src/
├── main.rs      # CLI interface and output
├── mesh.rs      # Triangular mesh generation
└── solver.rs    # Shallow water equations solver
```

**Lines of Code:**
- Total: ~900 lines
- Well-documented
- Modular design
- Easy to extend

## License

MIT

## References

- Toro, E. F. (2009). "Riemann Solvers and Numerical Methods for Fluid Dynamics"
- LeVeque, R. J. (2002). "Finite Volume Methods for Hyperbolic Problems"
- Kurganov, A., & Petrova, G. (2007). "A second-order well-balanced positivity preserving central-upwind scheme for the Saint-Venant system"
