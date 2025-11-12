mod mesh;
mod solver;

use clap::{Parser, ValueEnum};
use mesh::{TopographyType, TriangularMesh};
use solver::{FrictionLaw, ShallowWaterSolver};
use std::fs::File;
use std::io::Write;

#[derive(Debug, Clone, ValueEnum)]
enum InitialCondition {
    DamBreak,
    CircularWave,
    StandingWave,
}

#[derive(Debug, Clone, ValueEnum)]
enum Topography {
    Flat,
    Slope,
    Gaussian,
    Channel,
}

#[derive(Debug, Clone, ValueEnum)]
enum Friction {
    None,
    Manning,
    Chezy,
}

#[derive(Parser, Debug)]
#[command(name = "Shallow Water Solver")]
#[command(about = "Solves 2D shallow water equations on triangular mesh", long_about = None)]
struct Args {
    /// Number of grid points in x direction
    #[arg(short = 'x', long, default_value_t = 40)]
    nx: usize,

    /// Number of grid points in y direction
    #[arg(short = 'y', long, default_value_t = 40)]
    ny: usize,

    /// Domain width (meters)
    #[arg(short = 'w', long, default_value_t = 10.0)]
    width: f64,

    /// Domain height (meters)
    #[arg(short = 'h', long, default_value_t = 10.0)]
    height: f64,

    /// Final simulation time (seconds)
    #[arg(short = 't', long, default_value_t = 5.0)]
    final_time: f64,

    /// CFL number for time stepping
    #[arg(short = 'c', long, default_value_t = 0.45)]
    cfl: f64,

    /// Output interval (seconds)
    #[arg(short = 'o', long, default_value_t = 0.1)]
    output_interval: f64,

    /// Initial condition type
    #[arg(short = 'i', long, value_enum, default_value_t = InitialCondition::DamBreak)]
    initial_condition: InitialCondition,

    /// Topography/bathymetry type
    #[arg(long, value_enum, default_value_t = Topography::Flat)]
    topography: Topography,

    /// Bottom friction type
    #[arg(long, value_enum, default_value_t = Friction::None)]
    friction: Friction,

    /// Manning's n coefficient (used if friction=manning)
    #[arg(long, default_value_t = 0.03)]
    manning_n: f64,

    /// Chezy coefficient (used if friction=chezy)
    #[arg(long, default_value_t = 50.0)]
    chezy_c: f64,

    /// Output file prefix
    #[arg(short = 'p', long, default_value = "output")]
    output_prefix: String,
}

fn main() {
    let args = Args::parse();

    println!("═══════════════════════════════════════════════════════════");
    println!("  Shallow Water Equations Solver (2D Triangular Mesh)");
    println!("═══════════════════════════════════════════════════════════");
    println!();
    println!("Mesh Configuration:");
    println!(
        "  Grid points: {}x{} = {} triangles",
        args.nx,
        args.ny,
        2 * (args.nx - 1) * (args.ny - 1)
    );
    println!("  Domain size: {:.2}m × {:.2}m", args.width, args.height);
    println!();
    println!("Simulation Parameters:");
    println!("  Final time: {:.2}s", args.final_time);
    println!("  CFL number: {:.2}", args.cfl);
    println!("  Output interval: {:.2}s", args.output_interval);
    println!("  Initial condition: {:?}", args.initial_condition);
    println!("  Topography: {:?}", args.topography);
    println!("  Friction: {:?}", args.friction);
    if matches!(args.friction, Friction::Manning) {
        println!("  Manning's n: {:.4}", args.manning_n);
    } else if matches!(args.friction, Friction::Chezy) {
        println!("  Chezy C: {:.1}", args.chezy_c);
    }
    println!();

    // Create mesh
    println!("Creating triangular mesh...");
    let topography_type = match args.topography {
        Topography::Flat => TopographyType::Flat,
        Topography::Slope => TopographyType::Slope {
            gradient_x: 0.01,
            gradient_y: 0.005,
        },
        Topography::Gaussian => TopographyType::Gaussian {
            center: (args.width / 2.0, args.height / 2.0),
            amplitude: 1.0,
            width: args.width / 4.0,
        },
        Topography::Channel => TopographyType::Channel {
            depth: 2.0,
            width: args.width / 2.0,
        },
    };

    let mesh =
        TriangularMesh::new_rectangular(args.nx, args.ny, args.width, args.height, topography_type);
    println!("  Nodes: {}", mesh.nodes.len());
    println!("  Triangles: {}", mesh.triangles.len());
    println!("  Edges: {}", mesh.edges.len());

    // Report bed elevation range
    let z_min = mesh
        .nodes
        .iter()
        .map(|n| n.z)
        .min_by(|a, b| a.partial_cmp(b).unwrap())
        .unwrap_or(0.0);
    let z_max = mesh
        .nodes
        .iter()
        .map(|n| n.z)
        .max_by(|a, b| a.partial_cmp(b).unwrap())
        .unwrap_or(0.0);
    println!("  Bed elevation range: [{:.3}, {:.3}] m", z_min, z_max);
    println!();

    // Create solver
    println!("Initializing solver...");
    let friction_law = match args.friction {
        Friction::None => FrictionLaw::None,
        Friction::Manning => FrictionLaw::Manning {
            coefficient: args.manning_n,
        },
        Friction::Chezy => FrictionLaw::Chezy {
            coefficient: args.chezy_c,
        },
    };

    let mut solver = ShallowWaterSolver::new(mesh, args.cfl, friction_law);

    // Set initial condition
    match args.initial_condition {
        InitialCondition::DamBreak => {
            println!("  Setting dam break initial condition...");
            solver.set_dam_break(args.width / 2.0);
        }
        InitialCondition::CircularWave => {
            println!("  Setting circular wave initial condition...");
            solver.set_circular_wave((args.width / 2.0, args.height / 2.0), args.width / 4.0, 0.5);
        }
        InitialCondition::StandingWave => {
            println!("  Setting standing wave initial condition...");
            solver.set_standing_wave(0.1, args.width / 2.0);
        }
    }

    let initial_mass = solver.compute_total_mass();
    let initial_energy = solver.compute_total_energy();
    println!("  Initial mass: {:.6}", initial_mass);
    println!("  Initial energy: {:.6}", initial_energy);
    println!();

    // Save initial state
    save_state(&solver, 0, &args.output_prefix);

    // Time stepping
    println!("Starting time integration...");
    let mut output_counter = 1;
    let mut next_output_time = args.output_interval;
    let mut step_count = 0;

    while solver.time < args.final_time {
        solver.step();
        step_count += 1;

        if solver.time >= next_output_time {
            let mass = solver.compute_total_mass();
            let _energy = solver.compute_total_energy();
            let mass_error = ((mass - initial_mass) / initial_mass * 100.0).abs();

            println!(
                "  t = {:.3}s, dt = {:.6}s, steps = {}, mass error = {:.6}%",
                solver.time, solver.dt, step_count, mass_error
            );

            save_state(&solver, output_counter, &args.output_prefix);
            output_counter += 1;
            next_output_time += args.output_interval;
        }
    }

    println!();
    println!("Simulation completed!");
    println!("  Total steps: {}", step_count);
    println!("  Final time: {:.3}s", solver.time);

    let final_mass = solver.compute_total_mass();
    let final_energy = solver.compute_total_energy();
    let mass_conservation = ((final_mass - initial_mass) / initial_mass * 100.0).abs();

    println!();
    println!("Conservation Properties:");
    println!("  Initial mass: {:.6}", initial_mass);
    println!("  Final mass: {:.6}", final_mass);
    println!("  Mass conservation error: {:.8}%", mass_conservation);
    println!("  Initial energy: {:.6}", initial_energy);
    println!("  Final energy: {:.6}", final_energy);
    println!();
    println!("Output files saved with prefix: {}", args.output_prefix);
    println!("═══════════════════════════════════════════════════════════");
}

fn save_state(solver: &ShallowWaterSolver, index: usize, prefix: &str) {
    let filename = format!("{}_{:04}.vtk", prefix, index);

    match File::create(&filename) {
        Ok(mut file) => {
            // Write VTK file format for visualization in ParaView or similar
            writeln!(file, "# vtk DataFile Version 3.0").unwrap();
            writeln!(file, "Shallow Water Solution at t={:.4}", solver.time).unwrap();
            writeln!(file, "ASCII").unwrap();
            writeln!(file, "DATASET UNSTRUCTURED_GRID").unwrap();
            writeln!(file, "POINTS {} float", solver.mesh.nodes.len()).unwrap();

            for node in &solver.mesh.nodes {
                writeln!(file, "{} {} 0.0", node.x, node.y).unwrap();
            }

            writeln!(file, "").unwrap();
            writeln!(
                file,
                "CELLS {} {}",
                solver.mesh.triangles.len(),
                solver.mesh.triangles.len() * 4
            )
            .unwrap();

            for tri in &solver.mesh.triangles {
                writeln!(file, "3 {} {} {}", tri.nodes[0], tri.nodes[1], tri.nodes[2]).unwrap();
            }

            writeln!(file, "").unwrap();
            writeln!(file, "CELL_TYPES {}", solver.mesh.triangles.len()).unwrap();
            for _ in 0..solver.mesh.triangles.len() {
                writeln!(file, "5").unwrap(); // Triangle type
            }

            writeln!(file, "").unwrap();
            writeln!(file, "CELL_DATA {}", solver.mesh.triangles.len()).unwrap();

            writeln!(file, "SCALARS height float 1").unwrap();
            writeln!(file, "LOOKUP_TABLE default").unwrap();
            for &h in &solver.state.h {
                writeln!(file, "{}", h).unwrap();
            }

            writeln!(file, "VECTORS velocity float").unwrap();
            for i in 0..solver.mesh.triangles.len() {
                let (u, v) = solver.state.get_velocity(i);
                writeln!(file, "{} {} 0.0", u, v).unwrap();
            }

            writeln!(file, "SCALARS momentum_x float 1").unwrap();
            writeln!(file, "LOOKUP_TABLE default").unwrap();
            for &hu in &solver.state.hu {
                writeln!(file, "{}", hu).unwrap();
            }

            writeln!(file, "SCALARS momentum_y float 1").unwrap();
            writeln!(file, "LOOKUP_TABLE default").unwrap();
            for &hv in &solver.state.hv {
                writeln!(file, "{}", hv).unwrap();
            }

            writeln!(file, "SCALARS bed_elevation float 1").unwrap();
            writeln!(file, "LOOKUP_TABLE default").unwrap();
            for tri in &solver.mesh.triangles {
                writeln!(file, "{}", tri.z_bed).unwrap();
            }

            writeln!(file, "SCALARS water_surface float 1").unwrap();
            writeln!(file, "LOOKUP_TABLE default").unwrap();
            for (i, tri) in solver.mesh.triangles.iter().enumerate() {
                writeln!(file, "{}", tri.z_bed + solver.state.h[i]).unwrap();
            }
        }
        Err(e) => {
            eprintln!("Warning: Could not write output file {}: {}", filename, e);
        }
    }
}
