# Examples Guide

## 1. Dam Break

Classic dam break problem with discontinuous initial condition.

### Problem Setup
- Domain: 20m × 10m
- Mesh: 100 × 50 cells
- Initial condition:
  - Left side (x < 10m): h = 2.0 m
  - Right side (x ≥ 10m): h = 0.5 m
- Boundary: All walls (reflective)
- Duration: 5 seconds

### Running

```bash
./bin/dam_break
```

### Expected Results
- Shock wave propagates to the right
- Rarefaction wave propagates to the left
- Wave reflects from right wall
- Mass is conserved

### Visualization
In ParaView:
1. Open `dam_break_*.vtk` files
2. Color by "depth"
3. Apply "Plot Over Line" filter along x-axis for water depth profile

## 2. Radial Dam Break

Axisymmetric dam break with circular initial condition.

### Problem Setup
- Domain: 20m × 20m
- Mesh: 100 × 100 cells
- Initial condition:
  - Inside circle (r < 3m): h = 3.0 m
  - Outside circle (r ≥ 3m): h = 0.5 m
  - Center: (10m, 10m)
- Boundary: All walls
- Duration: 3 seconds

### Running

```bash
./bin/radial_dam
```

### Expected Results
- Circular wave propagates outward
- Approximately radially symmetric solution
- Wave reflects from walls
- Mass conservation

### Visualization
- Color by "velocity_magnitude"
- Use "Contour" filter for wave fronts
- Animate to see wave propagation

## 3. Tsunami Propagation

Tsunami wave propagation over variable bathymetry.

### Problem Setup
- Domain: 100km × 50km
- Mesh: 200 × 100 cells
- Bathymetry:
  - Deep ocean (x < 60km): 4000m depth
  - Continental shelf (x > 60km): Linear slope to 200m
- Initial condition:
  - Gaussian bump (earthquake source)
  - Center: (30km, 25km)
  - Amplitude: 10m
  - Width: 5km

### Running

```bash
./bin/tsunami
```

### Expected Results
- Wave disperses in deep ocean
- Wave amplifies on continental shelf
- Realistic tsunami behavior

### Visualization
- Use "Warp by Scalar" with depth field
- Color by velocity
- View from side to see wave evolution

## Creating Custom Scenarios

### Template

```cpp
#include "io.hpp"
#include "mesh.hpp"
#include "solver.hpp"

int main() {
    // 1. Create mesh
    auto mesh = swe::Mesh::create_rectangular(width, height, nx, ny);
    
    // 2. Setup solver parameters
    swe::SolverParameters params;
    params.gravity = 9.81;
    params.cfl = 0.5;
    params.friction = 0.0;  // Add friction if needed
    
    auto solver = std::make_unique<swe::Solver>(mesh, params);
    
    // 3. Set initial conditions
    const auto& triangles = mesh->get_triangles();
    std::vector<swe::State> initial_state(triangles.size());
    
    for (size_t i = 0; i < triangles.size(); ++i) {
        swe::Real x = triangles[i].centroid.x;
        swe::Real y = triangles[i].centroid.y;
        
        // Define your initial condition here
        swe::Real h = 1.0;  // depth
        swe::Real u = 0.0;  // x-velocity
        swe::Real v = 0.0;  // y-velocity
        
        initial_state[i] = swe::State(h, h * u, h * v);
    }
    
    solver->set_initial_condition(initial_state);
    
    // 4. Set boundary conditions
    solver->set_all_boundaries(swe::BoundaryCondition(swe::BoundaryType::Wall));
    
    // 5. Setup output
    swe::VTKWriter writer("my_simulation");
    swe::CSVWriter csv("my_stats.csv");
    
    // 6. Time stepping loop
    swe::Real total_time = 10.0;
    swe::Real output_interval = 0.1;
    swe::Real next_output = 0.0;
    size_t output_count = 0;
    
    while (solver->get_time() < total_time) {
        swe::Real dt = solver->compute_time_step();
        
        if (solver->get_time() + dt > total_time) {
            dt = total_time - solver->get_time();
        }
        
        solver->step(dt);
        
        if (solver->get_time() >= next_output) {
            writer.write(*mesh, solver->get_state(), 
                        solver->get_bathymetry(),
                        solver->get_time(), output_count);
            csv.write_timestep(solver->get_time(), *solver);
            
            ++output_count;
            next_output += output_interval;
        }
    }
    
    return 0;
}
```

## Common Initial Conditions

### Gaussian Bump

```cpp
swe::Real x0 = width / 2.0;
swe::Real y0 = height / 2.0;
swe::Real sigma = 1.0;
swe::Real amplitude = 0.5;

swe::Real r2 = (x - x0) * (x - x0) + (y - y0) * (y - y0);
swe::Real h = 1.0 + amplitude * std::exp(-r2 / (2.0 * sigma * sigma));
```

### Standing Wave

```cpp
swe::Real lambda = 4.0;  // wavelength
swe::Real amplitude = 0.1;

swe::Real h = 1.0 + amplitude * std::sin(2.0 * M_PI * x / lambda);
```

### Vortex

```cpp
swe::Real x0 = width / 2.0;
swe::Real y0 = height / 2.0;
swe::Real omega = 1.0;  // angular velocity

swe::Real dx = x - x0;
swe::Real dy = y - y0;
swe::Real r = std::sqrt(dx * dx + dy * dy);

swe::Real u = -omega * dy;
swe::Real v = omega * dx;
swe::Real h = 1.0;
```

## Common Bathymetries

### Flat Bottom

```cpp
std::vector<swe::Real> bathymetry(triangles.size(), 0.0);
```

### Sloped Bottom

```cpp
swe::Real slope = 0.01;
bathymetry[i] = -slope * x;
```

### Gaussian Bump

```cpp
swe::Real x0 = width / 2.0;
swe::Real y0 = height / 2.0;
swe::Real sigma = 2.0;
swe::Real height = 0.5;

swe::Real r2 = (x - x0) * (x - x0) + (y - y0) * (y - y0);
bathymetry[i] = -height * std::exp(-r2 / (2.0 * sigma * sigma));
```

### Step

```cpp
if (x < width / 2.0) {
    bathymetry[i] = -2.0;
} else {
    bathymetry[i] = -0.5;
}
```

## Boundary Condition Examples

### Mixed Boundaries

```cpp
const auto& edges = mesh->get_edges();

for (size_t i = 0; i < edges.size(); ++i) {
    if (!mesh->is_boundary_edge(i)) continue;
    
    swe::Real x = edges[i].midpoint.x;
    swe::Real y = edges[i].midpoint.y;
    
    if (x < 0.1) {
        // Left: inflow
        solver->set_boundary_condition(i, 
            swe::BoundaryCondition(swe::BoundaryType::Inflow, 
                swe::State(1.5, 0.5, 0.0)));
    } else if (x > width - 0.1) {
        // Right: outflow
        solver->set_boundary_condition(i,
            swe::BoundaryCondition(swe::BoundaryType::Outflow));
    } else {
        // Top/bottom: wall
        solver->set_boundary_condition(i,
            swe::BoundaryCondition(swe::BoundaryType::Wall));
    }
}
```

## Tips

1. **Start Simple**: Begin with coarse mesh and short duration
2. **Check Conservation**: Monitor mass and energy conservation
3. **CFL Condition**: Reduce CFL if solution becomes unstable
4. **Friction**: Add friction for realistic flow damping
5. **Output**: Don't output too frequently (wastes disk space)
6. **Visualization**: Use ParaView for best results
