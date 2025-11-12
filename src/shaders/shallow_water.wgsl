// Shallow Water Equations GPU Compute Shader (WGSL)
// Computes flux and updates for finite volume method

struct State {
    h: f32,    // Water height
    hu: f32,   // x-momentum
    hv: f32,   // y-momentum
    padding: f32,
}

@group(0) @binding(0)
var<storage, read> input_state: array<State>;

@group(0) @binding(1)
var<storage, read_write> output_state: array<State>;

@group(0) @binding(2)
var<uniform> params: SimulationParams;

struct SimulationParams {
    dt: f32,
    g: f32,        // Gravitational acceleration
    cfl: f32,
    n_triangles: u32,
}

// Compute wave speed for CFL condition
fn wave_speed(h: f32, u: f32, v: f32, g: f32) -> f32 {
    let velocity = sqrt(u * u + v * v);
    let wave_speed = sqrt(g * h);
    return velocity + wave_speed;
}

// HLL (Harten-Lax-van Leer) Riemann solver
fn hll_flux(
    h_l: f32, hu_l: f32, hv_l: f32,
    h_r: f32, hu_r: f32, hv_r: f32,
    nx: f32, ny: f32,
    g: f32
) -> vec3<f32> {
    // Left state
    let u_l = select(0.0, hu_l / h_l, h_l > 1e-10);
    let v_l = select(0.0, hv_l / h_l, h_l > 1e-10);
    let un_l = u_l * nx + v_l * ny;
    
    // Right state
    let u_r = select(0.0, hu_r / h_r, h_r > 1e-10);
    let v_r = select(0.0, hv_r / h_r, h_r > 1e-10);
    let un_r = u_r * nx + v_r * ny;
    
    // Wave speeds
    let c_l = sqrt(g * h_l);
    let c_r = sqrt(g * h_r);
    
    let s_l = min(un_l - c_l, un_r - c_r);
    let s_r = max(un_l + c_l, un_r + c_r);
    
    // Left flux
    let f_h_l = h_l * un_l;
    let f_hu_l = hu_l * un_l + 0.5 * g * h_l * h_l * nx;
    let f_hv_l = hv_l * un_l + 0.5 * g * h_l * h_l * ny;
    
    // Right flux
    let f_h_r = h_r * un_r;
    let f_hu_r = hu_r * un_r + 0.5 * g * h_r * h_r * nx;
    let f_hv_r = hv_r * un_r + 0.5 * g * h_r * h_r * ny;
    
    // HLL flux
    var flux: vec3<f32>;
    if (s_l >= 0.0) {
        flux = vec3<f32>(f_h_l, f_hu_l, f_hv_l);
    } else if (s_r <= 0.0) {
        flux = vec3<f32>(f_h_r, f_hu_r, f_hv_r);
    } else {
        let denom = 1.0 / (s_r - s_l);
        flux.x = (s_r * f_h_l - s_l * f_h_r + s_l * s_r * (h_r - h_l)) * denom;
        flux.y = (s_r * f_hu_l - s_l * f_hu_r + s_l * s_r * (hu_r - hu_l)) * denom;
        flux.z = (s_r * f_hv_l - s_l * f_hv_r + s_l * s_r * (hv_r - hv_l)) * denom;
    }
    
    return flux;
}

// Main compute kernel
@compute @workgroup_size(64)
fn main(@builtin(global_invocation_id) global_id: vec3<u32>) {
    let idx = global_id.x;
    
    if (idx >= params.n_triangles) {
        return;
    }
    
    // Read current state
    let state = input_state[idx];
    let h = state.h;
    let hu = state.hu;
    let hv = state.hv;
    
    // Compute velocity
    var u: f32 = 0.0;
    var v: f32 = 0.0;
    if (h > 1e-10) {
        u = hu / h;
        v = hv / h;
    }
    
    // Simple explicit Euler update (placeholder)
    // In a full implementation, you would:
    // 1. Compute fluxes across all edges
    // 2. Apply friction source terms
    // 3. Apply topographic source terms
    // 4. Update state
    
    // For now, just preserve state (will be enhanced)
    output_state[idx] = state;
}

// Additional kernel for time step computation
@compute @workgroup_size(256)
fn compute_max_speed(
    @builtin(global_invocation_id) global_id: vec3<u32>,
    @builtin(local_invocation_id) local_id: vec3<u32>
) {
    let idx = global_id.x;
    
    // Compute local maximum wave speed
    var local_max: f32 = 0.0;
    
    if (idx < params.n_triangles) {
        let state = input_state[idx];
        if (state.h > 1e-10) {
            let u = state.hu / state.h;
            let v = state.hv / state.h;
            local_max = wave_speed(state.h, u, v, params.g);
        }
    }
    
    // Would use shared memory reduction here for efficiency
    // This is a simplified version
}
