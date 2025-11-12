#include "gpu_solver.hpp"
#include <stdexcept>
#include <sstream>
#include <cmath>

namespace swe {

#ifdef USE_OPENCL

// OpenCL kernel source code
static const char* flux_kernel_source = R"(
typedef struct {
    float h;
    float hu;
    float hv;
} State;

typedef struct {
    float x;
    float y;
} Point;

typedef struct {
    int nodes[2];
    int triangles[2];
    Point midpoint;
    float length;
    Point normal;
} Edge;

typedef struct {
    float area;
} Triangle;

// Atomic add for float (OpenCL doesn't have built-in)
inline void atomic_add_float(__global float* addr, float val) {
    union {
        unsigned int u32;
        float f32;
    } expected, desired;
    
    do {
        expected.f32 = *addr;
        desired.f32 = expected.f32 + val;
    } while (atomic_cmpxchg((__global unsigned int*)addr, expected.u32, desired.u32) != expected.u32);
}

// HLL Riemann solver
State compute_hll_flux(State left, State right, Point normal, float gravity) {
    State flux;
    
    // Dry tolerance
    const float dry_tol = 1e-8f;
    
    if (left.h < dry_tol && right.h < dry_tol) {
        flux.h = 0.0f;
        flux.hu = 0.0f;
        flux.hv = 0.0f;
        return flux;
    }
    
    // Compute velocities
    float ul = (left.h > dry_tol) ? left.hu / left.h : 0.0f;
    float vl = (left.h > dry_tol) ? left.hv / left.h : 0.0f;
    float ur = (right.h > dry_tol) ? right.hu / right.h : 0.0f;
    float vr = (right.h > dry_tol) ? right.hv / right.h : 0.0f;
    
    // Normal velocities
    float unl = ul * normal.x + vl * normal.y;
    float unr = ur * normal.x + vr * normal.y;
    
    // Wave speeds
    float cl = sqrt(gravity * left.h);
    float cr = sqrt(gravity * right.h);
    
    float sl = fmin(unl - cl, unr - cr);
    float sr = fmax(unl + cl, unr + cr);
    
    // HLL flux
    if (sl >= 0.0f) {
        flux.h = left.hu * normal.x + left.hv * normal.y;
        flux.hu = (left.hu * ul + 0.5f * gravity * left.h * left.h) * normal.x +
                  (left.hu * vl) * normal.y;
        flux.hv = (left.hv * ul) * normal.x +
                  (left.hv * vr + 0.5f * gravity * left.h * left.h) * normal.y;
    } else if (sr <= 0.0f) {
        flux.h = right.hu * normal.x + right.hv * normal.y;
        flux.hu = (right.hu * ur + 0.5f * gravity * right.h * right.h) * normal.x +
                  (right.hu * vr) * normal.y;
        flux.hv = (right.hv * ur) * normal.x +
                  (right.hv * vr + 0.5f * gravity * right.h * right.h) * normal.y;
    } else {
        // Mixed flux
        State fl, fr;
        fl.h = left.hu * normal.x + left.hv * normal.y;
        fl.hu = (left.hu * ul + 0.5f * gravity * left.h * left.h) * normal.x +
                (left.hu * vl) * normal.y;
        fl.hv = (left.hv * ul) * normal.x +
                (left.hv * vl + 0.5f * gravity * left.h * left.h) * normal.y;
        
        fr.h = right.hu * normal.x + right.hv * normal.y;
        fr.hu = (right.hu * ur + 0.5f * gravity * right.h * right.h) * normal.x +
                (right.hu * vr) * normal.y;
        fr.hv = (right.hv * ur) * normal.x +
                (right.hv * vr + 0.5f * gravity * right.h * right.h) * normal.y;
        
        float denom = sr - sl;
        flux.h = (sr * fl.h - sl * fr.h + sl * sr * (right.h - left.h)) / denom;
        flux.hu = (sr * fl.hu - sl * fr.hu + sl * sr * (right.hu - left.hu)) / denom;
        flux.hv = (sr * fl.hv - sl * fr.hv + sl * sr * (right.hv - left.hv)) / denom;
    }
    
    return flux;
}

__kernel void compute_edge_fluxes(
    __global const State* states,
    __global const Edge* edges,
    __global const Triangle* triangles,
    __global State* residuals,
    const float dt,
    const float gravity,
    const int num_edges
) {
    int idx = get_global_id(0);
    if (idx >= num_edges) return;
    
    Edge edge = edges[idx];
    int left_id = edge.triangles[0];
    int right_id = edge.triangles[1];
    
    if (left_id < 0) return;
    
    State left_state = states[left_id];
    State right_state = (right_id >= 0) ? states[right_id] : left_state;
    
    // Compute flux
    State flux = compute_hll_flux(left_state, right_state, edge.normal, gravity);
    
    // Update residuals (atomic operations for thread safety)
    float flux_contrib_h = flux.h * edge.length * dt / triangles[left_id].area;
    float flux_contrib_hu = flux.hu * edge.length * dt / triangles[left_id].area;
    float flux_contrib_hv = flux.hv * edge.length * dt / triangles[left_id].area;
    
    atomic_add_float(&residuals[left_id].h, -flux_contrib_h);
    atomic_add_float(&residuals[left_id].hu, -flux_contrib_hu);
    atomic_add_float(&residuals[left_id].hv, -flux_contrib_hv);
    
    if (right_id >= 0) {
        float flux_contrib_h_r = flux.h * edge.length * dt / triangles[right_id].area;
        float flux_contrib_hu_r = flux.hu * edge.length * dt / triangles[right_id].area;
        float flux_contrib_hv_r = flux.hv * edge.length * dt / triangles[right_id].area;
        
        atomic_add_float(&residuals[right_id].h, flux_contrib_h_r);
        atomic_add_float(&residuals[right_id].hu, flux_contrib_hu_r);
        atomic_add_float(&residuals[right_id].hv, flux_contrib_hv_r);
    }
}
)";

GpuSolver::GpuSolver(size_t num_triangles, Real gravity)
    : num_triangles_(num_triangles), gravity_(gravity) {
    initialize_opencl();
    create_buffers();
    compile_kernels();
}

GpuSolver::~GpuSolver() {
    // OpenCL objects will be automatically released
}

void GpuSolver::initialize_opencl() {
    try {
        // Get all platforms
        std::vector<cl::Platform> platforms;
        cl::Platform::get(&platforms);
        
        if (platforms.empty()) {
            throw std::runtime_error("No OpenCL platforms found");
        }
        
        // Find a GPU device
        cl::Platform platform = platforms[0];
        std::vector<cl::Device> devices;
        platform.getDevices(CL_DEVICE_TYPE_GPU, &devices);
        
        if (devices.empty()) {
            // Fallback to CPU
            platform.getDevices(CL_DEVICE_TYPE_CPU, &devices);
            if (devices.empty()) {
                throw std::runtime_error("No OpenCL devices found");
            }
        }
        
        device_ = devices[0];
        
        // Create context and command queue
        context_ = cl::Context(device_);
        queue_ = cl::CommandQueue(context_, device_);
        
    } catch (const std::exception& ex) {
        throw std::runtime_error(std::string("OpenCL initialization error: ") + ex.what());
    } catch (...) {
        throw std::runtime_error("Unknown OpenCL initialization error");
    }
}

void GpuSolver::create_buffers() {
    try {
        size_t state_size = num_triangles_ * sizeof(State);
        
        state_buffer_ = cl::Buffer(context_, CL_MEM_READ_WRITE, state_size);
        residual_buffer_ = cl::Buffer(context_, CL_MEM_READ_WRITE, state_size);
        
    } catch (const std::exception& ex) {
        throw std::runtime_error(std::string("Failed to create OpenCL buffers: ") + ex.what());
    } catch (...) {
        throw std::runtime_error("Unknown error creating OpenCL buffers");
    }
}

void GpuSolver::compile_kernels() {
    try {
        // Create program from source
        cl::Program::Sources sources;
        sources.push_back({flux_kernel_source, strlen(flux_kernel_source)});
        
        program_ = cl::Program(context_, sources);
        
        // Build program
        try {
            program_.build({device_});
        } catch (const std::exception& build_ex) {
            // Get build log
            std::string log = program_.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device_);
            throw std::runtime_error("Kernel compilation failed:\n" + log);
        } catch (...) {
            throw std::runtime_error("Unknown kernel compilation error");
        }
        
        // Create kernel
        flux_kernel_ = cl::Kernel(program_, "compute_edge_fluxes");
        
    } catch (const std::exception& ex) {
        throw std::runtime_error(std::string("Failed to compile OpenCL kernels: ") + ex.what());
    } catch (...) {
        throw std::runtime_error("Unknown kernel compilation error");
    }
}

void GpuSolver::upload_state(const std::vector<State>& state) {
    try {
        queue_.enqueueWriteBuffer(state_buffer_, CL_TRUE, 0, 
            state.size() * sizeof(State), state.data());
    } catch (const std::exception& ex) {
        throw std::runtime_error(std::string("Failed to upload state to GPU: ") + ex.what());
    } catch (...) {
        throw std::runtime_error("Unknown error uploading state to GPU");
    }
}

struct GpuEdge {
    int nodes[2];
    int triangles[2];
    float midpoint[2];
    float length;
    float normal[2];
};

struct GpuTriangle {
    float area;
};

void GpuSolver::compute_fluxes(
    const std::vector<Edge>& edges,
    const std::vector<Triangle>& triangles,
    Real dt,
    std::vector<State>& residuals
) {
    try {
        // Convert edges to GPU format
        std::vector<GpuEdge> gpu_edges(edges.size());
        for (size_t i = 0; i < edges.size(); ++i) {
            gpu_edges[i].nodes[0] = edges[i].nodes[0];
            gpu_edges[i].nodes[1] = edges[i].nodes[1];
            gpu_edges[i].triangles[0] = edges[i].triangles[0];
            gpu_edges[i].triangles[1] = edges[i].triangles[1];
            gpu_edges[i].midpoint[0] = edges[i].midpoint.x;
            gpu_edges[i].midpoint[1] = edges[i].midpoint.y;
            gpu_edges[i].length = edges[i].length;
            gpu_edges[i].normal[0] = edges[i].normal.x;
            gpu_edges[i].normal[1] = edges[i].normal.y;
        }
        
        // Convert triangles to GPU format
        std::vector<GpuTriangle> gpu_triangles(triangles.size());
        for (size_t i = 0; i < triangles.size(); ++i) {
            gpu_triangles[i].area = triangles[i].area;
        }
        
        // Create temporary buffers
        cl::Buffer edge_buffer(context_, CL_MEM_READ_ONLY, 
            gpu_edges.size() * sizeof(GpuEdge));
        cl::Buffer triangle_buffer(context_, CL_MEM_READ_ONLY,
            gpu_triangles.size() * sizeof(GpuTriangle));
        
        // Upload data
        queue_.enqueueWriteBuffer(edge_buffer, CL_TRUE, 0,
            gpu_edges.size() * sizeof(GpuEdge), gpu_edges.data());
        queue_.enqueueWriteBuffer(triangle_buffer, CL_TRUE, 0,
            gpu_triangles.size() * sizeof(GpuTriangle), gpu_triangles.data());
        
        // Zero residuals
        std::vector<State> zero_residuals(num_triangles_, State(0, 0, 0));
        queue_.enqueueWriteBuffer(residual_buffer_, CL_TRUE, 0,
            zero_residuals.size() * sizeof(State), zero_residuals.data());
        
        // Set kernel arguments
        flux_kernel_.setArg(0, state_buffer_);
        flux_kernel_.setArg(1, edge_buffer);
        flux_kernel_.setArg(2, triangle_buffer);
        flux_kernel_.setArg(3, residual_buffer_);
        flux_kernel_.setArg(4, static_cast<float>(dt));
        flux_kernel_.setArg(5, static_cast<float>(gravity_));
        flux_kernel_.setArg(6, static_cast<int>(edges.size()));
        
        // Execute kernel
        size_t global_size = ((edges.size() + 255) / 256) * 256;  // Round up to multiple of 256
        size_t local_size = 256;
        
        queue_.enqueueNDRangeKernel(flux_kernel_, cl::NullRange,
            cl::NDRange(global_size), cl::NDRange(local_size));
        
        // Download results
        queue_.enqueueReadBuffer(residual_buffer_, CL_TRUE, 0,
            residuals.size() * sizeof(State), residuals.data());
        
        queue_.finish();
        
    } catch (const std::exception& ex) {
        throw std::runtime_error(std::string("Failed to compute fluxes on GPU: ") + ex.what());
    } catch (...) {
        throw std::runtime_error("Unknown error computing fluxes on GPU");
    }
}

void GpuSolver::download_state(std::vector<State>& state) {
    try {
        queue_.enqueueReadBuffer(state_buffer_, CL_TRUE, 0,
            state.size() * sizeof(State), state.data());
    } catch (const std::exception& ex) {
        throw std::runtime_error(std::string("Failed to download state from GPU: ") + ex.what());
    } catch (...) {
        throw std::runtime_error("Unknown error downloading state from GPU");
    }
}

bool GpuSolver::is_available() {
    try {
        std::vector<cl::Platform> platforms;
        cl::Platform::get(&platforms);
        return !platforms.empty();
    } catch (...) {
        return false;
    }
}

std::string GpuSolver::get_device_name() const {
    try {
        return device_.getInfo<CL_DEVICE_NAME>();
    } catch (...) {
        return "Unknown Device";
    }
}

#endif // USE_OPENCL

} // namespace swe
