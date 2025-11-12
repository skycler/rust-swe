#pragma once

#include "types.hpp"
#include <vector>
#include <memory>

#ifdef USE_OPENCL
#define CL_HPP_TARGET_OPENCL_VERSION 120
#define CL_HPP_MINIMUM_OPENCL_VERSION 120
#include <CL/opencl.hpp>
#endif

namespace swe {

#ifdef USE_OPENCL

class GpuSolver {
public:
    GpuSolver(size_t num_triangles, Real gravity);
    ~GpuSolver();

    // Upload state to GPU
    void upload_state(const std::vector<State>& state);
    
    // Compute fluxes on GPU
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
    std::string get_device_name() const;

private:
    size_t num_triangles_;
    Real gravity_;
    
    cl::Context context_;
    cl::Device device_;
    cl::CommandQueue queue_;
    cl::Program program_;
    cl::Kernel flux_kernel_;
    
    cl::Buffer state_buffer_;
    cl::Buffer edge_buffer_;
    cl::Buffer triangle_buffer_;
    cl::Buffer residual_buffer_;
    
    void initialize_opencl();
    void create_buffers();
    void compile_kernels();
};

#else

// Dummy implementation when OpenCL is not available
class GpuSolver {
public:
    GpuSolver(size_t, Real) {
        throw std::runtime_error("GPU support not compiled. Build with -DENABLE_GPU=ON");
    }
    
    static bool is_available() { return false; }
    std::string get_device_name() const { return "N/A"; }
    void upload_state(const std::vector<State>&) {}
    void compute_fluxes(const std::vector<Edge>&, const std::vector<Triangle>&, Real, std::vector<State>&) {}
    void download_state(std::vector<State>&) {}
};

#endif

} // namespace swe
