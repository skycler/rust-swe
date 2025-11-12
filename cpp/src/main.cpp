#include "io.hpp"
#include "mesh.hpp"
#include "solver.hpp"
#include "types.hpp"

#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>

using namespace swe;

struct CommandLineArgs {
    size_t nx = 50;
    size_t ny = 50;
    Real width = 10.0;
    Real height = 10.0;
    Real total_time = 1.0;
    Real output_interval = 0.1;
    Real cfl = 0.5;
    Real gravity = 9.81;
    Real friction = 0.0;
    std::string output_dir = "output";
    bool use_gpu = false;

    void print() const {
        std::cout << "═══════════════════════════════════════════════════════════\n";
        std::cout << "  Shallow Water Equations Solver (2D Triangular Mesh)\n";
        std::cout << "═══════════════════════════════════════════════════════════\n\n";

#ifdef USE_CUDA
        if (use_gpu) {
            std::cout << "GPU Acceleration: ENABLED (CUDA)\n";
        } else {
            std::cout << "GPU Acceleration: Available but not enabled\n";
        }
#else
        if (use_gpu) {
            std::cout << "WARNING: GPU requested but not compiled with CUDA support\n";
            std::cout << "Falling back to CPU mode.\n";
        }
#endif

#ifdef USE_OPENMP
        std::cout << "OpenMP: ENABLED\n";
#else
        std::cout << "OpenMP: DISABLED\n";
#endif

        std::cout << "\nMesh Configuration:\n";
        std::cout << "  Grid: " << nx << " x " << ny << "\n";
        std::cout << "  Domain: " << width << " x " << height << " m\n";
        
        std::cout << "\nSimulation Parameters:\n";
        std::cout << "  Total time: " << total_time << " s\n";
        std::cout << "  Output interval: " << output_interval << " s\n";
        std::cout << "  CFL number: " << cfl << "\n";
        std::cout << "  Gravity: " << gravity << " m/s²\n";
        std::cout << "  Friction: " << friction << "\n";
        std::cout << "  Output directory: " << output_dir << "\n\n";
    }
};

CommandLineArgs parse_args(int argc, char* argv[]) {
    CommandLineArgs args;

    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);

        if (arg == "--nx" && i + 1 < argc) {
            args.nx = std::stoul(argv[++i]);
        } else if (arg == "--ny" && i + 1 < argc) {
            args.ny = std::stoul(argv[++i]);
        } else if (arg == "--width" && i + 1 < argc) {
            args.width = std::stod(argv[++i]);
        } else if (arg == "--height" && i + 1 < argc) {
            args.height = std::stod(argv[++i]);
        } else if ((arg == "-t" || arg == "--time") && i + 1 < argc) {
            args.total_time = std::stod(argv[++i]);
        } else if (arg == "--output-interval" && i + 1 < argc) {
            args.output_interval = std::stod(argv[++i]);
        } else if (arg == "--cfl" && i + 1 < argc) {
            args.cfl = std::stod(argv[++i]);
        } else if (arg == "--friction" && i + 1 < argc) {
            args.friction = std::stod(argv[++i]);
        } else if ((arg == "-o" || arg == "--output") && i + 1 < argc) {
            args.output_dir = argv[++i];
        } else if (arg == "--use-gpu") {
            args.use_gpu = true;
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: " << argv[0] << " [OPTIONS]\n\n";
            std::cout << "Options:\n";
            std::cout << "  --nx NUM              Number of cells in x-direction (default: 50)\n";
            std::cout << "  --ny NUM              Number of cells in y-direction (default: 50)\n";
            std::cout << "  --width VALUE         Domain width in meters (default: 10.0)\n";
            std::cout << "  --height VALUE        Domain height in meters (default: 10.0)\n";
            std::cout << "  -t, --time VALUE      Total simulation time (default: 1.0)\n";
            std::cout << "  --output-interval VAL Output interval (default: 0.1)\n";
            std::cout << "  --cfl VALUE           CFL number (default: 0.5)\n";
            std::cout << "  --friction VALUE      Manning's friction coefficient (default: 0.0)\n";
            std::cout << "  -o, --output DIR      Output directory (default: output)\n";
            std::cout << "  --use-gpu             Enable GPU acceleration\n";
            std::cout << "  -h, --help            Show this help message\n";
            std::exit(0);
        }
    }

    return args;
}

void setup_dam_break(swe::Solver& solver, const swe::Mesh& mesh) {
    const auto& triangles = mesh.get_triangles();
    std::vector<swe::State> initial_state(triangles.size());

    for (size_t i = 0; i < triangles.size(); ++i) {
        Real x = triangles[i].centroid.x;
        
        // Dam break: high water on left, low on right
        if (x < 5.0) {
            initial_state[i] = swe::State(2.0, 0.0, 0.0);
        } else {
            initial_state[i] = swe::State(0.5, 0.0, 0.0);
        }
    }

    solver.set_initial_condition(initial_state);
    solver.set_all_boundaries(swe::BoundaryCondition(swe::BoundaryType::Wall));
}

int main(int argc, char* argv[]) {
    auto args = parse_args(argc, argv);
    args.print();

    // Create mesh
    std::cout << "Creating mesh...\n";
    auto start_time = std::chrono::high_resolution_clock::now();
    
    auto mesh = swe::Mesh::create_rectangular(args.width, args.height, args.nx, args.ny);
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    std::cout << "  Nodes: " << mesh->num_nodes() << "\n";
    std::cout << "  Triangles: " << mesh->num_triangles() << "\n";
    std::cout << "  Edges: " << mesh->num_edges() << "\n";
    std::cout << "  Min edge length: " << mesh->min_edge_length() << " m\n";
    std::cout << "  Max edge length: " << mesh->max_edge_length() << " m\n";
    std::cout << "  Total area: " << mesh->total_area() << " m²\n";
    std::cout << "  Mesh creation time: " << duration.count() << " ms\n\n";

    // Create solver
    std::cout << "Initializing solver...\n";
    swe::SolverParameters params;
    params.gravity = args.gravity;
    params.cfl = args.cfl;
    params.friction = args.friction;

    auto solver = std::make_unique<swe::Solver>(mesh, params);
    
    // Setup initial condition (dam break)
    setup_dam_break(*solver, *mesh);

    std::cout << "  Initial mass: " << solver->total_mass() << " kg\n";
    std::cout << "  Initial energy: " << solver->total_energy() << " J\n\n";

    // Setup output
    swe::VTKWriter vtk_writer(args.output_dir + "/solution");
    swe::CSVWriter csv_writer(args.output_dir + "/statistics.csv");

    // Time stepping
    std::cout << "Starting simulation...\n";
    std::cout << "───────────────────────────────────────────────────────────\n";
    std::cout << "  Time      Step    dt         Mass       Energy     Speed\n";
    std::cout << "───────────────────────────────────────────────────────────\n";

    Real current_time = 0.0;
    size_t output_count = 0;
    Real next_output = 0.0;
    size_t total_steps = 0;

    start_time = std::chrono::high_resolution_clock::now();

    while (current_time < args.total_time) {
        Real dt = solver->compute_time_step();
        
        if (current_time + dt > args.total_time) {
            dt = args.total_time - current_time;
        }

        solver->step(dt);
        current_time = solver->get_time();
        ++total_steps;

        // Output
        if (current_time >= next_output || current_time >= args.total_time) {
            std::cout << std::fixed << std::setprecision(4)
                     << "  " << std::setw(8) << current_time
                     << "  " << std::setw(6) << total_steps
                     << "  " << std::scientific << std::setprecision(3) << dt
                     << "  " << std::setprecision(6) << solver->total_mass()
                     << "  " << std::setprecision(4) << solver->total_energy()
                     << "  " << std::setprecision(3) << solver->max_wave_speed() << "\n";

            vtk_writer.write(*mesh, solver->get_state(), solver->get_bathymetry(), 
                           current_time, output_count);
            csv_writer.write_timestep(current_time, *solver);

            ++output_count;
            next_output += args.output_interval;
        }
    }

    end_time = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    std::cout << "───────────────────────────────────────────────────────────\n\n";
    std::cout << "Simulation completed!\n";
    std::cout << "  Total steps: " << total_steps << "\n";
    std::cout << "  Simulation time: " << duration.count() / 1000.0 << " s\n";
    std::cout << "  Steps per second: " << (total_steps * 1000.0) / duration.count() << "\n";
    std::cout << "  Final mass: " << solver->total_mass() << " kg\n";
    std::cout << "  Final energy: " << solver->total_energy() << " J\n";
    std::cout << "  Output files: " << output_count << "\n\n";

    return 0;
}
