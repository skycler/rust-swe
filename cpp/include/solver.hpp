#pragma once

#include "mesh.hpp"
#include "types.hpp"
#include <memory>
#include <vector>

namespace swe {

struct SolverParameters {
    Real gravity = 9.81;           // Gravitational acceleration (m/s^2)
    Real cfl = 0.5;                // CFL number
    Real friction = 0.0;           // Manning's friction coefficient
    Real min_depth = 1e-6;         // Minimum water depth threshold
    Real dry_tolerance = 1e-8;     // Tolerance for dry cells
    
    SolverParameters() = default;
};

class Solver {
public:
    Solver(std::shared_ptr<Mesh> mesh, const SolverParameters& params = SolverParameters());

    // Initialize state
    void set_initial_condition(const std::vector<State>& initial_state);
    void set_constant_state(const State& state);
    void set_bathymetry(const std::vector<Real>& bathymetry);

    // Boundary conditions
    void set_boundary_condition(size_t edge_id, const BoundaryCondition& bc);
    void set_all_boundaries(const BoundaryCondition& bc);

    // Time stepping
    Real compute_time_step() const;
    void step(Real dt);
    void advance_to_time(Real target_time);

    // Getters
    const std::vector<State>& get_state() const { return state_; }
    const std::vector<Real>& get_bathymetry() const { return bathymetry_; }
    const Mesh& get_mesh() const { return *mesh_; }
    Real get_time() const { return current_time_; }
    size_t get_step_count() const { return step_count_; }

    // Statistics
    Real total_mass() const;
    Real total_energy() const;
    Real max_wave_speed() const;

private:
    std::shared_ptr<Mesh> mesh_;
    SolverParameters params_;
    
    std::vector<State> state_;        // Current state at triangle centroids
    std::vector<State> state_new_;    // New state (for time stepping)
    std::vector<Real> bathymetry_;    // Bathymetry at triangle centroids
    
    std::vector<BoundaryCondition> boundary_conditions_;
    
    Real current_time_;
    size_t step_count_;

    // Numerical flux computation
    State compute_hll_flux(
        const State& left,
        const State& right,
        const Point& normal,
        Real gravity
    ) const;

    // Reconstruct state at edge from triangle state
    State reconstruct_state(size_t triangle_id, size_t edge_id) const;

    // Apply boundary condition
    State apply_boundary_condition(
        const State& interior_state,
        size_t edge_id,
        const Point& normal
    ) const;

    // Source terms
    State compute_source_terms(size_t triangle_id, const State& state) const;

    // Wave speed estimation
    void compute_wave_speeds(
        const State& left,
        const State& right,
        const Point& normal,
        Real gravity,
        Real& sl,
        Real& sr
    ) const;

    // Parallel computation helpers
    void compute_fluxes(std::vector<State>& residuals, Real dt) const;
    void apply_source_terms(std::vector<State>& residuals, Real dt) const;
    void update_state(const std::vector<State>& residuals);
};

} // namespace swe
