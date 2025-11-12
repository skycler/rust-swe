#include "io.hpp"
#include "mesh.hpp"
#include "solver.hpp"

#include <cmath>
#include <iostream>
#include <memory>

int main() {
    std::cout << "Tsunami Wave Propagation Example\n";
    std::cout << "=================================\n\n";

    // Create mesh
    const size_t nx = 200;
    const size_t ny = 100;
    const swe::Real width = 100.0;  // 100 km
    const swe::Real height = 50.0;  // 50 km

    auto mesh = swe::Mesh::create_rectangular(width, height, nx, ny);
    
    std::cout << "Mesh created:\n";
    std::cout << "  Nodes: " << mesh->num_nodes() << "\n";
    std::cout << "  Triangles: " << mesh->num_triangles() << "\n\n";

    // Create solver
    swe::SolverParameters params;
    params.gravity = 9.81;
    params.cfl = 0.5;
    
    auto solver = std::make_shared<swe::Solver>(mesh, params);

    // Setup bathymetry: ocean with continental shelf
    const auto& triangles = mesh->get_triangles();
    std::vector<swe::Real> bathymetry(triangles.size());
    std::vector<swe::State> initial_state(triangles.size());
    
    for (size_t i = 0; i < triangles.size(); ++i) {
        swe::Real x = triangles[i].centroid.x;
        
        // Depth: 4000m deep ocean, rising to 200m at coast
        swe::Real depth = 4000.0;
        if (x > 60.0) {
            depth = 200.0 + (4000.0 - 200.0) * (100.0 - x) / 40.0;
        }
        
        bathymetry[i] = -depth / 1000.0;  // Convert to km, negative for depth
        
        // Initial water surface: small gaussian bump (earthquake)
        swe::Real y = triangles[i].centroid.y;
        swe::Real center_x = 30.0;
        swe::Real center_y = height / 2.0;
        swe::Real sigma = 5.0;
        
        swe::Real r2 = (x - center_x) * (x - center_x) + 
                       (y - center_y) * (y - center_y);
        swe::Real bump = 0.01 * std::exp(-r2 / (2.0 * sigma * sigma));  // 10m bump
        
        initial_state[i] = swe::State(depth / 1000.0 + bump, 0.0, 0.0);
    }
    
    solver->set_initial_condition(initial_state);
    solver->set_bathymetry(bathymetry);
    solver->set_all_boundaries(swe::BoundaryCondition(swe::BoundaryType::Wall));

    std::cout << "Initial conditions set (tsunami source)\n";
    std::cout << "  Initial mass: " << solver->total_mass() << " km³\n\n";

    // Setup output
    swe::VTKWriter writer("tsunami");

    // Time stepping
    const swe::Real total_time = 1.0;  // 1 hour in suitable time units
    const swe::Real output_interval = 0.02;
    swe::Real next_output = 0.0;
    size_t output_count = 0;

    std::cout << "Running simulation...\n";

    while (solver->get_time() < total_time) {
        swe::Real dt = solver->compute_time_step();
        
        if (solver->get_time() + dt > total_time) {
            dt = total_time - solver->get_time();
        }
        
        solver->step(dt);

        if (solver->get_time() >= next_output) {
            std::cout << "  t = " << solver->get_time() << "\n";
            
            writer.write(*mesh, solver->get_state(), solver->get_bathymetry(),
                        solver->get_time(), output_count);
            
            ++output_count;
            next_output += output_interval;
        }
    }

    std::cout << "\nSimulation complete!\n";
    std::cout << "  Output files: " << output_count << "\n";

    return 0;
}
