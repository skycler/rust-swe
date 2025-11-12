#pragma once

#include "types.hpp"
#include <vector>
#include <memory>

#ifdef USE_KOKKOS
#include <Kokkos_Core.hpp>
#endif

namespace swe {

#ifdef USE_KOKKOS

class GpuSolver {
public:
    GpuSolver(size_t num_triangles, Real gravity);
    ~GpuSolver();

    // Upload state to GPU
    void upload_state(const std::vector<State>& state);
    
    // Compute fluxes on GPU/parallel device
    void compute_fluxes(
        const std::vector<Edge>& edges,
        const std::vector<Triangle>& triangles,
        Real dt,
        std::vector<State>& residuals
    );
    
    // Download results from GPU
    void download_state(std::vector<State>& state);
    
    // Check if GPU is available
    static bool is_available();
    
    // Get device info
    static std::string get_device_name();

private:
    size_t num_triangles_;
    Real gravity_;
    
    // Kokkos Views for device data
    using StateView = Kokkos::View<State*>;
    using EdgeView = Kokkos::View<Edge*>;
    using TriangleView = Kokkos::View<Triangle*>;
    
    StateView state_device_;
    StateView residual_device_;
    EdgeView edges_device_;
    TriangleView triangles_device_;
};

#else

// Dummy implementation when Kokkos is not available
class GpuSolver {
public:
    GpuSolver(size_t, Real) {
        throw std::runtime_error("GPU support not compiled. Build with Kokkos enabled");
    }
    
    static bool is_available() { return false; }
    static std::string get_device_name() { return "N/A"; }
    void upload_state(const std::vector<State>&) {}
    void compute_fluxes(const std::vector<Edge>&, const std::vector<Triangle>&, Real, std::vector<State>&) {}
    void download_state(std::vector<State>&) {}
};

#endif

} // namespace swe
