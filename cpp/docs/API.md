# API Documentation

## Core Types (`types.hpp`)

### Point

2D point/vector structure with common operations.

```cpp
struct Point {
    Real x, y;
    
    Point(Real x, Real y);
    Point operator+(const Point& other) const;
    Point operator-(const Point& other) const;
    Point operator*(Real scalar) const;
    Real dot(const Point& other) const;
    Real norm() const;
    Real cross(const Point& other) const;
};
```

### State

State vector for shallow water equations.

```cpp
struct State {
    Real h;   // Water depth
    Real hu;  // x-momentum
    Real hv;  // y-momentum
    
    State(Real h, Real hu, Real hv);
    // Arithmetic operators overloaded
};
```

### BoundaryType

```cpp
enum class BoundaryType {
    Wall,      // Reflective boundary
    Open,      // Transmissive boundary
    Inflow,    // Fixed inflow state
    Outflow    // Zero-gradient outflow
};
```

## Mesh Class (`mesh.hpp`)

### Construction

```cpp
// Create rectangular mesh
auto mesh = Mesh::create_rectangular(
    Real width,      // Domain width
    Real height,     // Domain height
    size_t nx,       // Cells in x
    size_t ny        // Cells in y
);

// Load from file (future)
auto mesh = Mesh::from_file(filename);
```

### Mesh Queries

```cpp
const std::vector<Point>& get_nodes() const;
const std::vector<Triangle>& get_triangles() const;
const std::vector<Edge>& get_edges() const;

size_t num_nodes() const;
size_t num_triangles() const;
size_t num_edges() const;

std::vector<size_t> get_node_triangles(size_t node_id) const;
std::vector<size_t> get_node_edges(size_t node_id) const;

bool is_boundary_node(size_t node_id) const;
bool is_boundary_edge(size_t edge_id) const;

Real min_edge_length() const;
Real max_edge_length() const;
Real total_area() const;
```

## Solver Class (`solver.hpp`)

### Construction

```cpp
SolverParameters params;
params.gravity = 9.81;
params.cfl = 0.5;
params.friction = 0.025;
params.min_depth = 1e-6;
params.dry_tolerance = 1e-8;

Solver solver(mesh, params);
```

### Initialization

```cpp
// Set initial conditions
solver.set_initial_condition(initial_state);
solver.set_constant_state(State(1.0, 0.0, 0.0));

// Set bathymetry
solver.set_bathymetry(bathymetry);

// Set boundary conditions
solver.set_boundary_condition(edge_id, bc);
solver.set_all_boundaries(BoundaryCondition(BoundaryType::Wall));
```

### Time Stepping

```cpp
// Compute stable time step
Real dt = solver.compute_time_step();

// Advance one time step
solver.step(dt);

// Advance to specific time
solver.advance_to_time(10.0);
```

### State Access

```cpp
const std::vector<State>& get_state() const;
const std::vector<Real>& get_bathymetry() const;
Real get_time() const;
size_t get_step_count() const;
```

### Diagnostics

```cpp
Real total_mass() const;
Real total_energy() const;
Real max_wave_speed() const;
```

## I/O Classes (`io.hpp`)

### VTKWriter

```cpp
VTKWriter writer("output/solution");

// Write single snapshot
writer.write(mesh, state, bathymetry, time, step);

// Write from solver
writer.write_solver_state(solver, step);
```

### CSVWriter

```cpp
CSVWriter csv("statistics.csv");

// Write header
csv.write_header();

// Write timestep data
csv.write_timestep(time, solver);

// Close
csv.close();
```

## Example Usage

### Basic Simulation

```cpp
#include "mesh.hpp"
#include "solver.hpp"
#include "io.hpp"

int main() {
    // Create mesh
    auto mesh = swe::Mesh::create_rectangular(10.0, 10.0, 100, 100);
    
    // Create solver
    swe::SolverParameters params;
    params.cfl = 0.5;
    auto solver = std::make_unique<swe::Solver>(mesh, params);
    
    // Initialize
    solver->set_constant_state(swe::State(1.0, 0.0, 0.0));
    solver->set_all_boundaries(swe::BoundaryCondition(swe::BoundaryType::Wall));
    
    // Output
    swe::VTKWriter writer("solution");
    
    // Time loop
    Real total_time = 10.0;
    size_t step = 0;
    
    while (solver->get_time() < total_time) {
        Real dt = solver->compute_time_step();
        solver->step(dt);
        
        if (step % 10 == 0) {
            writer.write_solver_state(*solver, step);
        }
        ++step;
    }
    
    return 0;
}
```

### Custom Initial Condition

```cpp
const auto& triangles = mesh->get_triangles();
std::vector<swe::State> initial_state(triangles.size());

for (size_t i = 0; i < triangles.size(); ++i) {
    Real x = triangles[i].centroid.x;
    Real y = triangles[i].centroid.y;
    
    // Gaussian bump
    Real r2 = (x - 5.0) * (x - 5.0) + (y - 5.0) * (y - 5.0);
    Real h = 1.0 + 0.5 * std::exp(-r2 / 2.0);
    
    initial_state[i] = swe::State(h, 0.0, 0.0);
}

solver->set_initial_condition(initial_state);
```

### Bathymetry

```cpp
std::vector<swe::Real> bathymetry(triangles.size());

for (size_t i = 0; i < triangles.size(); ++i) {
    Real x = triangles[i].centroid.x;
    
    // Sloping bottom
    bathymetry[i] = -0.1 * x;
}

solver->set_bathymetry(bathymetry);
```

## Thread Safety

- `Mesh` is thread-safe for read operations after construction
- `Solver::step()` is thread-safe internally (uses OpenMP)
- Multiple solvers can run in parallel on different threads
- I/O operations are not thread-safe

## Performance Tips

1. **Mesh Size**: Use appropriate resolution for your problem
2. **CFL Number**: Lower values (0.3-0.5) are more stable
3. **OpenMP**: Enable for meshes with >10,000 triangles
4. **Output Frequency**: Write only necessary snapshots
5. **Compiler Flags**: Use `-O3 -march=native` for release builds
