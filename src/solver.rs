/// Shallow Water Equations solver using Finite Volume Method
/// Solves: ∂U/∂t + ∂F/∂x + ∂G/∂y = S
/// where U = [h, hu, hv]^T (water height, x-momentum, y-momentum)
/// S includes bottom friction and topographic source terms

use crate::mesh::{TriangularMesh, Edge};
use std::f64::consts::PI;
use rayon::prelude::*;

const G: f64 = 9.81; // Gravitational acceleration (m/s^2)

#[derive(Debug, Clone, Copy)]
pub enum FrictionLaw {
    None,
    Manning { coefficient: f64 }, // Manning's n (s/m^(1/3))
    Chezy { coefficient: f64 },   // Chezy's C (m^(1/2)/s)
}

#[derive(Debug, Clone)]
pub struct State {
    pub h: Vec<f64>,   // Water height
    pub hu: Vec<f64>,  // x-momentum (h * u)
    pub hv: Vec<f64>,  // y-momentum (h * v)
}

impl State {
    pub fn new(n_triangles: usize) -> Self {
        State {
            h: vec![0.0; n_triangles],
            hu: vec![0.0; n_triangles],
            hv: vec![0.0; n_triangles],
        }
    }
    
    pub fn get_velocity(&self, i: usize) -> (f64, f64) {
        let h = self.h[i];
        if h > 1e-10 {
            (self.hu[i] / h, self.hv[i] / h)
        } else {
            (0.0, 0.0)
        }
    }
}

pub struct ShallowWaterSolver {
    pub mesh: TriangularMesh,
    pub state: State,
    pub time: f64,
    pub dt: f64,
    pub cfl: f64,
    pub friction: FrictionLaw,
}

impl ShallowWaterSolver {
    pub fn new(mesh: TriangularMesh, cfl: f64, friction: FrictionLaw) -> Self {
        let n_triangles = mesh.triangles.len();
        let state = State::new(n_triangles);
        
        ShallowWaterSolver {
            mesh,
            state,
            time: 0.0,
            dt: 0.001,
            cfl,
            friction,
        }
    }
    
    /// Compute adaptive time step based on CFL condition
    pub fn compute_timestep(&mut self) {
        let max_speed = (0..self.mesh.triangles.len())
            .into_par_iter()
            .map(|i| {
                let (u, v) = self.state.get_velocity(i);
                let h = self.state.h[i];
                let c = (G * h).sqrt(); // Wave speed
                (u * u + v * v).sqrt() + c
            })
            .reduce(|| 0.0, f64::max);
        
        if max_speed > 1e-10 {
            // Compute minimum element size
            let min_size = self.mesh.triangles.par_iter()
                .map(|t| (t.area * 2.0).sqrt())
                .min_by(|a, b| a.partial_cmp(b).unwrap())
                .unwrap_or(1.0);
            
            self.dt = self.cfl * min_size / max_speed;
        }
    }
    
    /// Second-order Runge-Kutta time stepping
    pub fn step(&mut self) {
        self.compute_timestep();
        
        // RK2 first stage
        let k1 = self.compute_residual(&self.state);
        let state_intermediate = self.update_state(&self.state, &k1, 0.5 * self.dt);
        
        // RK2 second stage
        let k2 = self.compute_residual(&state_intermediate);
        self.state = self.update_state(&self.state, &k2, self.dt);
        
        self.apply_boundary_conditions();
        self.time += self.dt;
    }
    
    fn update_state(&self, state: &State, residual: &State, dt: f64) -> State {
        let n = self.mesh.triangles.len();
        
        // Compute new values in parallel
        let new_h: Vec<f64> = (0..n)
            .into_par_iter()
            .map(|i| {
                let area = self.mesh.triangles[i].area;
                let h = state.h[i] - dt * residual.h[i] / area;
                h.max(0.0) // Ensure positive depth
            })
            .collect();
        
        let new_hu: Vec<f64> = (0..n)
            .into_par_iter()
            .map(|i| {
                let area = self.mesh.triangles[i].area;
                let hu = state.hu[i] - dt * residual.hu[i] / area;
                if new_h[i] < 1e-10 { 0.0 } else { hu }
            })
            .collect();
        
        let new_hv: Vec<f64> = (0..n)
            .into_par_iter()
            .map(|i| {
                let area = self.mesh.triangles[i].area;
                let hv = state.hv[i] - dt * residual.hv[i] / area;
                if new_h[i] < 1e-10 { 0.0 } else { hv }
            })
            .collect();
        
        State {
            h: new_h,
            hu: new_hu,
            hv: new_hv,
        }
    }
    
    /// Compute spatial residual using finite volume method
    fn compute_residual(&self, state: &State) -> State {
        let mut residual = State::new(self.mesh.triangles.len());
        
        // Loop over all edges and compute fluxes
        for edge in &self.mesh.edges {
            let flux = self.compute_flux(edge, state);
            
            // Add flux contribution to left triangle
            let left = edge.left_triangle;
            residual.h[left] += flux.0 * edge.length;
            residual.hu[left] += flux.1 * edge.length;
            residual.hv[left] += flux.2 * edge.length;
            
            // Subtract flux contribution from right triangle (if exists)
            if let Some(right) = edge.right_triangle {
                residual.h[right] -= flux.0 * edge.length;
                residual.hu[right] -= flux.1 * edge.length;
                residual.hv[right] -= flux.2 * edge.length;
            }
        }
        
        // Add source terms (friction and topography)
        self.add_source_terms(&mut residual, state);
        
        residual
    }
    
    /// Add source terms: bottom friction and topographic gradients
    fn add_source_terms(&self, residual: &mut State, state: &State) {
        // Parallel computation of source terms
        let source_contributions: Vec<_> = (0..self.mesh.triangles.len())
            .into_par_iter()
            .map(|i| {
                let tri = &self.mesh.triangles[i];
                let h = state.h[i];
                let (u, v) = state.get_velocity(i);
                
                if h < 1e-10 {
                    return (0.0, 0.0, 0.0);
                }
                
                // Bottom friction source term
                let (sf_x, sf_y) = self.compute_friction_slope(h, u, v);
                
                // Topographic source term: -g * h * ∇z_b
                let (dzdx, dzdy) = self.compute_bed_gradient(i);
                
                // Combine friction and topography contributions
                let dhu = -G * h * (sf_x + dzdx) * tri.area;
                let dhv = -G * h * (sf_y + dzdy) * tri.area;
                
                (0.0, dhu, dhv) // No mass source term
            })
            .collect();
        
        // Apply contributions sequentially (fast, no contention)
        for (i, (dh, dhu, dhv)) in source_contributions.iter().enumerate() {
            residual.h[i] += dh;
            residual.hu[i] += dhu;
            residual.hv[i] += dhv;
        }
    }
    
    /// Compute friction slope using Manning's or Chezy's formula
    fn compute_friction_slope(&self, h: f64, u: f64, v: f64) -> (f64, f64) {
        let velocity_mag = (u * u + v * v).sqrt();
        
        if velocity_mag < 1e-10 {
            return (0.0, 0.0);
        }
        
        let sf_mag = match self.friction {
            FrictionLaw::None => 0.0,
            FrictionLaw::Manning { coefficient } => {
                // S_f = n^2 * |v|^2 / h^(4/3)
                let n = coefficient;
                if h > 1e-6 {
                    n * n * velocity_mag * velocity_mag / h.powf(4.0 / 3.0)
                } else {
                    0.0
                }
            }
            FrictionLaw::Chezy { coefficient } => {
                // S_f = |v|^2 / (C^2 * h)
                let c = coefficient;
                if h > 1e-6 {
                    velocity_mag * velocity_mag / (c * c * h)
                } else {
                    0.0
                }
            }
        };
        
        // Direction of friction (opposite to velocity)
        let sf_x = sf_mag * u / velocity_mag;
        let sf_y = sf_mag * v / velocity_mag;
        
        (sf_x, sf_y)
    }
    
    /// Compute bed elevation gradient at triangle center
    fn compute_bed_gradient(&self, tri_idx: usize) -> (f64, f64) {
        let tri = &self.mesh.triangles[tri_idx];
        
        // Use Green-Gauss theorem for gradient computation
        // ∇z_b ≈ (1/A) * Σ z_b_face * n * L
        
        let mut grad_x = 0.0;
        let mut grad_y = 0.0;
        
        for i in 0..3 {
            let n0_idx = tri.nodes[i];
            let n1_idx = tri.nodes[(i + 1) % 3];
            
            let n0 = &self.mesh.nodes[n0_idx];
            let n1 = &self.mesh.nodes[n1_idx];
            
            // Edge midpoint elevation
            let z_mid = (n0.z + n1.z) / 2.0;
            
            // Edge normal vector (pointing outward)
            let dx = n1.x - n0.x;
            let dy = n1.y - n0.y;
            let edge_length = (dx * dx + dy * dy).sqrt();
            let nx = -dy / edge_length;
            let ny = dx / edge_length;
            
            grad_x += z_mid * nx * edge_length;
            grad_y += z_mid * ny * edge_length;
        }
        
        grad_x /= tri.area;
        grad_y /= tri.area;
        
        (grad_x, grad_y)
    }
    
    /// Compute numerical flux using Lax-Friedrichs (Rusanov) flux
    fn compute_flux(&self, edge: &Edge, state: &State) -> (f64, f64, f64) {
        let left = edge.left_triangle;
        
        // Left state
        let h_l = state.h[left];
        let (u_l, v_l) = state.get_velocity(left);
        let hu_l = state.hu[left];
        let hv_l = state.hv[left];
        
        // Right state (or boundary condition)
        let (h_r, u_r, v_r, hu_r, hv_r) = if let Some(right) = edge.right_triangle {
            let (u, v) = state.get_velocity(right);
            (state.h[right], u, v, state.hu[right], state.hv[right])
        } else {
            // Wall boundary condition (reflective)
            let (nx, ny) = edge.normal;
            let u_normal = u_l * nx + v_l * ny;
            let u_r = u_l - 2.0 * u_normal * nx;
            let v_r = v_l - 2.0 * u_normal * ny;
            (h_l, u_r, v_r, h_l * u_r, h_l * v_r)
        };
        
        let (nx, ny) = edge.normal;
        
        // Compute normal velocities
        let un_l = u_l * nx + v_l * ny;
        let un_r = u_r * nx + v_r * ny;
        
        // Physical fluxes in normal direction
        let f_h_l = hu_l * nx + hv_l * ny;
        let f_hu_l = (hu_l * u_l + 0.5 * G * h_l * h_l) * nx + (hu_l * v_l) * ny;
        let f_hv_l = (hv_l * u_l) * nx + (hv_l * v_l + 0.5 * G * h_l * h_l) * ny;
        
        let f_h_r = hu_r * nx + hv_r * ny;
        let f_hu_r = (hu_r * u_r + 0.5 * G * h_r * h_r) * nx + (hu_r * v_r) * ny;
        let f_hv_r = (hv_r * u_r) * nx + (hv_r * v_r + 0.5 * G * h_r * h_r) * ny;
        
        // Wave speeds
        let c_l = (G * h_l).sqrt();
        let c_r = (G * h_r).sqrt();
        let s_max = (un_l.abs() + c_l).max(un_r.abs() + c_r);
        
        // Lax-Friedrichs flux
        let flux_h = 0.5 * (f_h_l + f_h_r - s_max * (h_r - h_l));
        let flux_hu = 0.5 * (f_hu_l + f_hu_r - s_max * (hu_r - hu_l));
        let flux_hv = 0.5 * (f_hv_l + f_hv_r - s_max * (hv_r - hv_l));
        
        (flux_h, flux_hu, flux_hv)
    }
    
    /// Apply boundary conditions
    pub fn apply_boundary_conditions(&mut self) {
        // Boundary conditions are handled in flux computation
        // This method is for any additional constraints
        for i in 0..self.mesh.triangles.len() {
            if self.state.h[i] < 1e-10 {
                self.state.h[i] = 0.0;
                self.state.hu[i] = 0.0;
                self.state.hv[i] = 0.0;
            }
        }
    }
    
    /// Set initial condition: dam break
    pub fn set_dam_break(&mut self, x_dam: f64) {
        for (i, tri) in self.mesh.triangles.iter().enumerate() {
            if tri.centroid.0 < x_dam {
                self.state.h[i] = 2.0; // High water level
            } else {
                self.state.h[i] = 1.0; // Low water level
            }
            self.state.hu[i] = 0.0;
            self.state.hv[i] = 0.0;
        }
    }
    
    /// Set initial condition: circular wave
    pub fn set_circular_wave(&mut self, center: (f64, f64), radius: f64, amplitude: f64) {
        let h_base = 1.0;
        
        for (i, tri) in self.mesh.triangles.iter().enumerate() {
            let dx = tri.centroid.0 - center.0;
            let dy = tri.centroid.1 - center.1;
            let r = (dx * dx + dy * dy).sqrt();
            
            if r < radius {
                let height = h_base + amplitude * (1.0 + (PI * r / radius).cos());
                self.state.h[i] = height;
            } else {
                self.state.h[i] = h_base;
            }
            self.state.hu[i] = 0.0;
            self.state.hv[i] = 0.0;
        }
    }
    
    /// Set initial condition: standing wave
    pub fn set_standing_wave(&mut self, amplitude: f64, wavelength: f64) {
        let h_base = 1.0;
        
        for (i, tri) in self.mesh.triangles.iter().enumerate() {
            let x = tri.centroid.0;
            let y = tri.centroid.1;
            
            let h = h_base + amplitude * (2.0 * PI * x / wavelength).sin() 
                                       * (2.0 * PI * y / wavelength).sin();
            self.state.h[i] = h;
            self.state.hu[i] = 0.0;
            self.state.hv[i] = 0.0;
        }
    }
    
    /// Compute total mass (should be conserved)
    pub fn compute_total_mass(&self) -> f64 {
        let mut total = 0.0;
        for (i, tri) in self.mesh.triangles.iter().enumerate() {
            total += self.state.h[i] * tri.area;
        }
        total
    }
    
    /// Compute total energy
    pub fn compute_total_energy(&self) -> f64 {
        let mut total = 0.0;
        for (i, tri) in self.mesh.triangles.iter().enumerate() {
            let h = self.state.h[i];
            let (u, v) = self.state.get_velocity(i);
            let kinetic = 0.5 * h * (u * u + v * v);
            let potential = 0.5 * G * h * h;
            total += (kinetic + potential) * tri.area;
        }
        total
    }
}
