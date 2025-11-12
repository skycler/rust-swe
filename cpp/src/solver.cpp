#include "solver.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

#ifdef USE_OPENMP
#include <omp.h>
#endif

namespace swe {

Solver::Solver(std::shared_ptr<Mesh> mesh, const SolverParameters& params)
    : mesh_(mesh), params_(params), current_time_(0.0), step_count_(0) {
    
    const size_t n_triangles = mesh_->num_triangles();
    const size_t n_edges = mesh_->num_edges();
    
    state_.resize(n_triangles);
    state_new_.resize(n_triangles);
    bathymetry_.resize(n_triangles, 0.0);
    boundary_conditions_.resize(n_edges);
}

void Solver::set_initial_condition(const std::vector<State>& initial_state) {
    if (initial_state.size() != mesh_->num_triangles()) {
        throw std::runtime_error("Initial state size mismatch");
    }
    state_ = initial_state;
    state_new_ = state_;
}

void Solver::set_constant_state(const State& state) {
    std::fill(state_.begin(), state_.end(), state);
    state_new_ = state_;
}

void Solver::set_bathymetry(const std::vector<Real>& bathymetry) {
    if (bathymetry.size() != mesh_->num_triangles()) {
        throw std::runtime_error("Bathymetry size mismatch");
    }
    bathymetry_ = bathymetry;
}

void Solver::set_boundary_condition(size_t edge_id, const BoundaryCondition& bc) {
    if (edge_id >= boundary_conditions_.size()) {
        throw std::out_of_range("Edge ID out of range");
    }
    boundary_conditions_[edge_id] = bc;
}

void Solver::set_all_boundaries(const BoundaryCondition& bc) {
    const auto& edges = mesh_->get_edges();
    for (size_t i = 0; i < edges.size(); ++i) {
        if (mesh_->is_boundary_edge(i)) {
            boundary_conditions_[i] = bc;
        }
    }
}

Real Solver::compute_time_step() const {
    Real max_speed = max_wave_speed();
    Real min_dx = mesh_->min_edge_length();
    
    if (max_speed < 1e-10) {
        return params_.cfl * min_dx / std::sqrt(params_.gravity * params_.min_depth);
    }
    
    return params_.cfl * min_dx / max_speed;
}

void Solver::step(Real dt) {
    std::vector<State> residuals(state_.size());

    // Compute fluxes
    compute_fluxes(residuals, dt);

    // Apply source terms
    apply_source_terms(residuals, dt);

    // Update state
    update_state(residuals);

    current_time_ += dt;
    ++step_count_;
}

void Solver::compute_fluxes(std::vector<State>& residuals, Real dt) const {
    const auto& edges = mesh_->get_edges();
    const auto& triangles = mesh_->get_triangles();

    #ifdef USE_OPENMP
    #pragma omp parallel for
    #endif
    for (size_t i = 0; i < edges.size(); ++i) {
        const auto& edge = edges[i];
        
        int left_id = edge.triangles[0];
        int right_id = edge.triangles[1];

        if (left_id < 0) continue;

        State left_state = state_[left_id];
        State right_state;

        if (right_id >= 0) {
            right_state = state_[right_id];
        } else {
            // Boundary condition
            right_state = apply_boundary_condition(left_state, i, edge.normal);
        }

        // Compute HLL flux
        State flux = compute_hll_flux(left_state, right_state, edge.normal, params_.gravity);

        // Update residuals
        State flux_contribution = flux * edge.length * dt / triangles[left_id].area;
        
        #ifdef USE_OPENMP
        #pragma omp atomic
        #endif
        residuals[left_id].h -= flux_contribution.h;
        
        #ifdef USE_OPENMP
        #pragma omp atomic
        #endif
        residuals[left_id].hu -= flux_contribution.hu;
        
        #ifdef USE_OPENMP
        #pragma omp atomic
        #endif
        residuals[left_id].hv -= flux_contribution.hv;

        if (right_id >= 0) {
            flux_contribution = flux * edge.length * dt / triangles[right_id].area;
            
            #ifdef USE_OPENMP
            #pragma omp atomic
            #endif
            residuals[right_id].h += flux_contribution.h;
            
            #ifdef USE_OPENMP
            #pragma omp atomic
            #endif
            residuals[right_id].hu += flux_contribution.hu;
            
            #ifdef USE_OPENMP
            #pragma omp atomic
            #endif
            residuals[right_id].hv += flux_contribution.hv;
        }
    }
}

void Solver::apply_source_terms(std::vector<State>& residuals, Real dt) const {
    #ifdef USE_OPENMP
    #pragma omp parallel for
    #endif
    for (size_t i = 0; i < state_.size(); ++i) {
        State source = compute_source_terms(i, state_[i]);
        residuals[i] += source * dt;
    }
}

void Solver::update_state(const std::vector<State>& residuals) {
    #ifdef USE_OPENMP
    #pragma omp parallel for
    #endif
    for (size_t i = 0; i < state_.size(); ++i) {
        state_[i] += residuals[i];
        
        // Apply positivity preservation
        if (state_[i].h < params_.dry_tolerance) {
            state_[i].h = 0.0;
            state_[i].hu = 0.0;
            state_[i].hv = 0.0;
        }
    }
}

State Solver::compute_hll_flux(
    const State& left,
    const State& right,
    const Point& normal,
    Real gravity
) const {
    // Compute wave speeds
    Real sl, sr;
    compute_wave_speeds(left, right, normal, gravity, sl, sr);

    // Normal velocities
    Real ul = (left.h > params_.dry_tolerance) ? 
        (left.hu * normal.x + left.hv * normal.y) / left.h : 0.0;
    Real ur = (right.h > params_.dry_tolerance) ? 
        (right.hu * normal.x + right.hv * normal.y) / right.h : 0.0;

    // Compute fluxes in normal direction
    State flux_left, flux_right;

    flux_left.h = left.hu * normal.x + left.hv * normal.y;
    flux_left.hu = left.hu * ul + 0.5 * gravity * left.h * left.h * normal.x;
    flux_left.hv = left.hv * ul + 0.5 * gravity * left.h * left.h * normal.y;

    flux_right.h = right.hu * normal.x + right.hv * normal.y;
    flux_right.hu = right.hu * ur + 0.5 * gravity * right.h * right.h * normal.x;
    flux_right.hv = right.hv * ur + 0.5 * gravity * right.h * right.h * normal.y;

    // HLL flux
    State flux;
    if (sl >= 0.0) {
        flux = flux_left;
    } else if (sr <= 0.0) {
        flux = flux_right;
    } else {
        flux = (flux_right * sl - flux_left * sr + (left - right) * (sl * sr)) / (sl - sr);
    }

    return flux;
}

void Solver::compute_wave_speeds(
    const State& left,
    const State& right,
    const Point& normal,
    Real gravity,
    Real& sl,
    Real& sr
) const {
    // Compute normal velocities
    Real ul = (left.h > params_.dry_tolerance) ? 
        (left.hu * normal.x + left.hv * normal.y) / left.h : 0.0;
    Real ur = (right.h > params_.dry_tolerance) ? 
        (right.hu * normal.x + right.hv * normal.y) / right.h : 0.0;

    // Wave speeds
    Real cl = std::sqrt(gravity * std::max(left.h, params_.min_depth));
    Real cr = std::sqrt(gravity * std::max(right.h, params_.min_depth));

    sl = std::min(ul - cl, ur - cr);
    sr = std::max(ul + cl, ur + cr);
}

State Solver::apply_boundary_condition(
    const State& interior_state,
    size_t edge_id,
    const Point& normal
) const {
    const auto& bc = boundary_conditions_[edge_id];

    switch (bc.type) {
        case BoundaryType::Wall: {
            // Reflective boundary: reverse normal velocity
            State ghost = interior_state;
            Real u = interior_state.h > params_.dry_tolerance ? 
                interior_state.hu / interior_state.h : 0.0;
            Real v = interior_state.h > params_.dry_tolerance ? 
                interior_state.hv / interior_state.h : 0.0;
            
            Real un = u * normal.x + v * normal.y;
            u -= 2.0 * un * normal.x;
            v -= 2.0 * un * normal.y;
            
            ghost.hu = ghost.h * u;
            ghost.hv = ghost.h * v;
            return ghost;
        }

        case BoundaryType::Open:
            // Transmissive boundary
            return interior_state;

        case BoundaryType::Inflow:
            // Fixed inflow state
            return bc.value;

        case BoundaryType::Outflow:
            // Extrapolation
            return interior_state;

        default:
            return interior_state;
    }
}

State Solver::compute_source_terms(size_t /* triangle_id */, const State& state) const {
    State source;

    // Friction term (Manning's formula)
    if (params_.friction > 0.0 && state.h > params_.dry_tolerance) {
        Real u = state.hu / state.h;
        Real v = state.hv / state.h;
        Real speed = std::sqrt(u * u + v * v);
        
        Real friction_coeff = params_.gravity * params_.friction * params_.friction * 
                             speed / std::pow(state.h, 4.0/3.0);
        
        source.hu = -friction_coeff * state.hu;
        source.hv = -friction_coeff * state.hv;
    }

    return source;
}

void Solver::advance_to_time(Real target_time) {
    while (current_time_ < target_time) {
        Real dt = compute_time_step();
        
        // Adjust last time step
        if (current_time_ + dt > target_time) {
            dt = target_time - current_time_;
        }
        
        step(dt);
    }
}

Real Solver::total_mass() const {
    Real mass = 0.0;
    const auto& triangles = mesh_->get_triangles();
    
    for (size_t i = 0; i < state_.size(); ++i) {
        mass += state_[i].h * triangles[i].area;
    }
    
    return mass;
}

Real Solver::total_energy() const {
    Real energy = 0.0;
    const auto& triangles = mesh_->get_triangles();
    
    for (size_t i = 0; i < state_.size(); ++i) {
        Real kinetic = 0.0;
        if (state_[i].h > params_.dry_tolerance) {
            Real u = state_[i].hu / state_[i].h;
            Real v = state_[i].hv / state_[i].h;
            kinetic = 0.5 * state_[i].h * (u * u + v * v);
        }
        
        Real potential = 0.5 * params_.gravity * state_[i].h * state_[i].h;
        energy += (kinetic + potential) * triangles[i].area;
    }
    
    return energy;
}

Real Solver::max_wave_speed() const {
    Real max_speed = 0.0;
    
    for (const auto& s : state_) {
        if (s.h > params_.dry_tolerance) {
            Real u = s.hu / s.h;
            Real v = s.hv / s.h;
            Real speed = std::sqrt(u * u + v * v) + std::sqrt(params_.gravity * s.h);
            max_speed = std::max(max_speed, speed);
        }
    }
    
    return max_speed;
}

} // namespace swe
