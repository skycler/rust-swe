#include "gpu_solver.hpp"
#include <stdexcept>
#include <sstream>
#include <cmath>

namespace swe {

#ifdef USE_KOKKOS

// HLL Riemann solver - used in parallel kernel
KOKKOS_INLINE_FUNCTION
State compute_hll_flux(const State& left, const State& right, 
                       Real gravity, const Point& normal) {
    // Water depths
    Real h_l = left.h;
    Real h_r = right.h;
    
    // Handle dry states
    const Real dry_tol = 1e-6;
    if (h_l < dry_tol && h_r < dry_tol) {
        return {0.0, 0.0, 0.0};
    }
    
    // Rotate velocities to edge-aligned coordinates
    Real u_l = (left.hu * normal.x + left.hv * normal.y) / (h_l + dry_tol);
    Real u_r = (right.hu * normal.x + right.hv * normal.y) / (h_r + dry_tol);
    
    Real v_l = (-left.hu * normal.y + left.hv * normal.x) / (h_l + dry_tol);
    Real v_r = (-right.hu * normal.y + right.hv * normal.x) / (h_r + dry_tol);
    
    // Wave speeds
    Real c_l = Kokkos::sqrt(gravity * h_l);
    Real c_r = Kokkos::sqrt(gravity * h_r);
    
    Real s_l = Kokkos::fmin(u_l - c_l, u_r - c_r);
    Real s_r = Kokkos::fmax(u_l + c_l, u_r + c_r);
    
    // HLL flux
    State flux;
    if (s_l >= 0.0) {
        // Left state
        flux.h = h_l * u_l;
        flux.hu = h_l * u_l * u_l + 0.5 * gravity * h_l * h_l;
        flux.hv = h_l * u_l * v_l;
    } else if (s_r <= 0.0) {
        // Right state
        flux.h = h_r * u_r;
        flux.hu = h_r * u_r * u_r + 0.5 * gravity * h_r * h_r;
        flux.hv = h_r * u_r * v_r;
    } else {
        // Middle state (HLL)
        Real h_flux_l = h_l * u_l;
        Real h_flux_r = h_r * u_r;
        Real hu_flux_l = h_l * u_l * u_l + 0.5 * gravity * h_l * h_l;
        Real hu_flux_r = h_r * u_r * u_r + 0.5 * gravity * h_r * h_r;
        Real hv_flux_l = h_l * u_l * v_l;
        Real hv_flux_r = h_r * u_r * v_r;
        
        Real factor = 1.0 / (s_r - s_l);
        flux.h = factor * (s_r * h_flux_l - s_l * h_flux_r + s_l * s_r * (h_r - h_l));
        flux.hu = factor * (s_r * hu_flux_l - s_l * hu_flux_r + s_l * s_r * (h_r * u_r - h_l * u_l));
        flux.hv = factor * (s_r * hv_flux_l - s_l * hv_flux_r + s_l * s_r * (h_r * v_r - h_l * v_l));
    }
    
    // Rotate flux back to global coordinates
    Real flux_n = flux.hu;
    Real flux_t = flux.hv;
    flux.hu = flux_n * normal.x - flux_t * normal.y;
    flux.hv = flux_n * normal.y + flux_t * normal.x;
    
    return flux;
}

GpuSolver::GpuSolver(size_t num_triangles, Real gravity)
    : num_triangles_(num_triangles), gravity_(gravity) {
    
    // Initialize Kokkos if not already done
    if (!Kokkos::is_initialized()) {
        Kokkos::initialize();
    }
    
    // Allocate device Views
    state_device_ = StateView("state", num_triangles);
    residual_device_ = StateView("residual", num_triangles);
}

GpuSolver::~GpuSolver() {
    // Views are automatically deallocated
}

void GpuSolver::upload_state(const std::vector<State>& state) {
    // Create host mirror and copy data
    auto state_host = Kokkos::create_mirror_view(state_device_);
    for (size_t i = 0; i < state.size(); ++i) {
        state_host(i) = state[i];
    }
    Kokkos::deep_copy(state_device_, state_host);
}

void GpuSolver::compute_fluxes(
    const std::vector<Edge>& edges,
    const std::vector<Triangle>& triangles,
    Real /* dt */,
    std::vector<State>& residuals) {
    
    // Allocate device memory for edges and triangles if needed
    if (edges_device_.extent(0) != edges.size()) {
        edges_device_ = EdgeView("edges", edges.size());
        auto edges_host = Kokkos::create_mirror_view(edges_device_);
        for (size_t i = 0; i < edges.size(); ++i) {
            edges_host(i) = edges[i];
        }
        Kokkos::deep_copy(edges_device_, edges_host);
    }
    
    if (triangles_device_.extent(0) != triangles.size()) {
        triangles_device_ = TriangleView("triangles", triangles.size());
        auto triangles_host = Kokkos::create_mirror_view(triangles_device_);
        for (size_t i = 0; i < triangles.size(); ++i) {
            triangles_host(i) = triangles[i];
        }
        Kokkos::deep_copy(triangles_device_, triangles_host);
    }
    
    // Zero out residuals
    Kokkos::deep_copy(residual_device_, State{0.0, 0.0, 0.0});
    
    // Capture variables for lambda
    auto state_d = state_device_;
    auto residual_d = residual_device_;
    auto edges_d = edges_device_;
    auto triangles_d = triangles_device_;
    Real g = gravity_;
    
    // Parallel edge flux computation
    Kokkos::parallel_for("compute_edge_fluxes", edges.size(), 
        KOKKOS_LAMBDA(const size_t i) {
            const Edge& edge = edges_d(i);
            
            // Get left and right states
            int left_tri = edge.triangles[0];
            int right_tri = edge.triangles[1];
            
            if (left_tri < 0 && right_tri < 0) return; // Skip invalid edges
            
            State left_state = (left_tri >= 0) ? state_d(left_tri) : State{0.0, 0.0, 0.0};
            State right_state = (right_tri >= 0) ? state_d(right_tri) : State{0.0, 0.0, 0.0};
            
            // Compute HLL flux
            State flux = compute_hll_flux(left_state, right_state, g, edge.normal);
            
            // Scale by edge length
            flux.h *= edge.length;
            flux.hu *= edge.length;
            flux.hv *= edge.length;
            
            // Atomic updates to residuals
            if (left_tri >= 0) {
                Kokkos::atomic_sub(&residual_d(left_tri).h, flux.h);
                Kokkos::atomic_sub(&residual_d(left_tri).hu, flux.hu);
                Kokkos::atomic_sub(&residual_d(left_tri).hv, flux.hv);
            }
            
            if (right_tri >= 0) {
                Kokkos::atomic_add(&residual_d(right_tri).h, flux.h);
                Kokkos::atomic_add(&residual_d(right_tri).hu, flux.hu);
                Kokkos::atomic_add(&residual_d(right_tri).hv, flux.hv);
            }
        });
    
    // Normalize by triangle area
    Kokkos::parallel_for("normalize_residuals", triangles.size(),
        KOKKOS_LAMBDA(const size_t i) {
            const Triangle& tri = triangles_d(i);
            Real inv_area = 1.0 / tri.area;
            
            residual_d(i).h *= inv_area;
            residual_d(i).hu *= inv_area;
            residual_d(i).hv *= inv_area;
        });
    
    Kokkos::fence();
    
    // Download residuals
    auto residual_host = Kokkos::create_mirror_view(residual_device_);
    Kokkos::deep_copy(residual_host, residual_device_);
    
    for (size_t i = 0; i < residuals.size(); ++i) {
        residuals[i] = residual_host(i);
    }
}

void GpuSolver::download_state(std::vector<State>& state) {
    auto state_host = Kokkos::create_mirror_view(state_device_);
    Kokkos::deep_copy(state_host, state_device_);
    
    for (size_t i = 0; i < state.size(); ++i) {
        state[i] = state_host(i);
    }
}

bool GpuSolver::is_available() {
    return Kokkos::is_initialized();
}

std::string GpuSolver::get_device_name() {
    std::stringstream ss;
    ss << "Kokkos " << Kokkos::DefaultExecutionSpace::name();
    return ss.str();
}

#endif // USE_KOKKOS

} // namespace swe
