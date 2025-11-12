#include "io.hpp"
#include <fstream>
#include <iomanip>
#include <sstream>

namespace swe {

VTKWriter::VTKWriter(const std::string& base_filename)
    : base_filename_(base_filename) {}

std::string VTKWriter::format_filename(size_t step) const {
    std::ostringstream oss;
    oss << base_filename_ << "_" << std::setfill('0') << std::setw(6) << step << ".vtk";
    return oss.str();
}

void VTKWriter::write(
    const Mesh& mesh,
    const std::vector<State>& state,
    const std::vector<Real>& bathymetry,
    Real time,
    size_t step
) {
    std::string filename = format_filename(step);
    std::ofstream file(filename);
    
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + filename);
    }

    const auto& nodes = mesh.get_nodes();
    const auto& triangles = mesh.get_triangles();

    // VTK header
    file << "# vtk DataFile Version 3.0\n";
    file << "Shallow Water Solver Output (t=" << std::scientific << time << ")\n";
    file << "ASCII\n";
    file << "DATASET UNSTRUCTURED_GRID\n\n";

    // Points
    file << "POINTS " << nodes.size() << " double\n";
    for (const auto& node : nodes) {
        file << std::scientific << std::setprecision(12)
             << node.x << " " << node.y << " 0.0\n";
    }
    file << "\n";

    // Cells (triangles)
    file << "CELLS " << triangles.size() << " " << (triangles.size() * 4) << "\n";
    for (const auto& tri : triangles) {
        file << "3 " << tri.nodes[0] << " " << tri.nodes[1] << " " << tri.nodes[2] << "\n";
    }
    file << "\n";

    // Cell types (5 = triangle)
    file << "CELL_TYPES " << triangles.size() << "\n";
    for (size_t i = 0; i < triangles.size(); ++i) {
        file << "5\n";
    }
    file << "\n";

    // Cell data
    file << "CELL_DATA " << triangles.size() << "\n";

    // Water depth
    file << "SCALARS depth double 1\n";
    file << "LOOKUP_TABLE default\n";
    for (const auto& s : state) {
        file << std::scientific << std::setprecision(12) << s.h << "\n";
    }
    file << "\n";

    // Velocity magnitude
    file << "SCALARS velocity_magnitude double 1\n";
    file << "LOOKUP_TABLE default\n";
    for (const auto& s : state) {
        Real vel_mag = 0.0;
        if (s.h > 1e-8) {
            Real u = s.hu / s.h;
            Real v = s.hv / s.h;
            vel_mag = std::sqrt(u * u + v * v);
        }
        file << std::scientific << std::setprecision(12) << vel_mag << "\n";
    }
    file << "\n";

    // Velocity vector
    file << "VECTORS velocity double\n";
    for (const auto& s : state) {
        Real u = 0.0, v = 0.0;
        if (s.h > 1e-8) {
            u = s.hu / s.h;
            v = s.hv / s.h;
        }
        file << std::scientific << std::setprecision(12)
             << u << " " << v << " 0.0\n";
    }
    file << "\n";

    // Bathymetry
    if (!bathymetry.empty()) {
        file << "SCALARS bathymetry double 1\n";
        file << "LOOKUP_TABLE default\n";
        for (Real z : bathymetry) {
            file << std::scientific << std::setprecision(12) << z << "\n";
        }
        file << "\n";
    }

    file.close();
}

void VTKWriter::write_solver_state(const Solver& solver, size_t step) {
    write(
        solver.get_mesh(),
        solver.get_state(),
        solver.get_bathymetry(),
        solver.get_time(),
        step
    );
}

CSVWriter::CSVWriter(const std::string& filename)
    : filename_(filename), header_written_(false) {
    file_.open(filename);
    if (!file_.is_open()) {
        throw std::runtime_error("Cannot open CSV file: " + filename);
    }
}

CSVWriter::~CSVWriter() {
    close();
}

void CSVWriter::write_header() {
    if (!header_written_) {
        file_ << "time,step,total_mass,total_energy,max_wave_speed\n";
        header_written_ = true;
    }
}

void CSVWriter::write_timestep(Real time, const Solver& solver) {
    if (!header_written_) {
        write_header();
    }

    file_ << std::scientific << std::setprecision(12)
          << time << ","
          << solver.get_step_count() << ","
          << solver.total_mass() << ","
          << solver.total_energy() << ","
          << solver.max_wave_speed() << "\n";
    
    file_.flush();
}

void CSVWriter::close() {
    if (file_.is_open()) {
        file_.close();
    }
}

} // namespace swe
