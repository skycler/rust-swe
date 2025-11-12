#pragma once

#include "mesh.hpp"
#include "solver.hpp"
#include "types.hpp"
#include <fstream>
#include <memory>
#include <string>

namespace swe {

class VTKWriter {
public:
    VTKWriter(const std::string& base_filename);

    void write(
        const Mesh& mesh,
        const std::vector<State>& state,
        const std::vector<Real>& bathymetry,
        Real time,
        size_t step
    );

    void write_solver_state(
        const Solver& solver,
        size_t step
    );

private:
    std::string base_filename_;
    
    std::string format_filename(size_t step) const;
};

class CSVWriter {
public:
    CSVWriter(const std::string& filename);
    ~CSVWriter();

    void write_header();
    void write_timestep(Real time, const Solver& solver);
    void close();

private:
    std::string filename_;
    std::ofstream file_;
    bool header_written_;
};

} // namespace swe
