#include "io.hpp"
#include "mesh.hpp"
#include "solver.hpp"

#include <iostream>
#include <memory>

int main() {
    std::cout << "Dam Break Example\n";
    std::cout << "=================\n\n";

    // Create mesh
    const size_t nx = 100;
    const size_t ny = 50;
    const swe::Real width = 20.0;
    const swe::Real height = 10.0;

    auto mesh = swe::Mesh::create_rectangular(width, height, nx, ny);
    
    std::cout << "Mesh created:\n";
    std::cout << "  Nodes: " << mesh->num_nodes() << "\n";
    std::cout << "  Triangles: " << mesh->num_triangles() << "\n";
    std::cout << "  Edges: " << mesh->num_edges() << "\n\n";

    // Create solver
    swe::SolverParameters params;
    params.gravity = 9.81;
    params.cfl = 0.5;
    
    auto solver = std::make_shared<swe::Solver>(mesh, params);

    // Setup dam break: high water on left, low on right
    const auto& triangles = mesh->get_triangles();
    std::vector<swe::State> initial_state(triangles.size());
    
    for (size_t i = 0; i < triangles.size(); ++i) {
        swe::Real x = triangles[i].centroid.x;
        
        if (x < width / 2.0) {
            initial_state[i] = swe::State(2.0, 0.0, 0.0);  // High water
        } else {
            initial_state[i] = swe::State(0.5, 0.0, 0.0);  // Low water
        }
    }
    
    solver->set_initial_condition(initial_state);
    solver->set_all_boundaries(swe::BoundaryCondition(swe::BoundaryType::Wall));

    std::cout << "Initial conditions set\n";
    std::cout << "  Initial mass: " << solver->total_mass() << " kg\n";
    std::cout << "  Initial energy: " << solver->total_energy() << " J\n\n";

    // Setup output
    swe::VTKWriter writer("dam_break");
    swe::CSVWriter csv("dam_break_stats.csv");

    // Time stepping
    const swe::Real total_time = 5.0;
    const swe::Real output_interval = 0.1;
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
            std::cout << "  t = " << solver->get_time() 
                     << " s, step = " << solver->get_step_count()
                     << ", mass = " << solver->total_mass() << " kg\n";
            
            writer.write(*mesh, solver->get_state(), solver->get_bathymetry(),
                        solver->get_time(), output_count);
            csv.write_timestep(solver->get_time(), *solver);
            
            ++output_count;
            next_output += output_interval;
        }
    }

    std::cout << "\nSimulation complete!\n";
    std::cout << "  Final time: " << solver->get_time() << " s\n";
    std::cout << "  Total steps: " << solver->get_step_count() << "\n";
    std::cout << "  Final mass: " << solver->total_mass() << " kg\n";
    std::cout << "  Output files: " << output_count << "\n";

    return 0;
}
