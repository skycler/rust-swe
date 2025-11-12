/// GPU-accelerated Shallow Water Equations solver using WebGPU
#[cfg(feature = "gpu")]
use bytemuck::{Pod, Zeroable};

#[cfg(feature = "gpu")]
#[repr(C)]
#[derive(Copy, Clone, Debug, Pod, Zeroable)]
pub struct GpuState {
    h: f32,
    hu: f32,
    hv: f32,
    _padding: f32,
}

#[cfg(feature = "gpu")]
#[allow(dead_code)]
pub struct GpuSolver {
    device: wgpu::Device,
    queue: wgpu::Queue,
    compute_pipeline: wgpu::ComputePipeline,
    state_buffer: wgpu::Buffer,
    output_buffer: wgpu::Buffer,
    n_triangles: usize,
}

#[cfg(feature = "gpu")]
#[allow(dead_code)]
impl GpuSolver {
    pub async fn new(n_triangles: usize) -> Result<Self, Box<dyn std::error::Error>> {
        // Initialize WebGPU
        let instance = wgpu::Instance::new(wgpu::InstanceDescriptor {
            backends: wgpu::Backends::all(),
            ..Default::default()
        });

        let adapter = instance
            .request_adapter(&wgpu::RequestAdapterOptions {
                power_preference: wgpu::PowerPreference::HighPerformance,
                compatible_surface: None,
                force_fallback_adapter: false,
            })
            .await
            .ok_or("Failed to find GPU adapter")?;

        let (device, queue) = adapter
            .request_device(
                &wgpu::DeviceDescriptor {
                    label: Some("Shallow Water Solver Device"),
                    required_features: wgpu::Features::empty(),
                    required_limits: wgpu::Limits::default(),
                    memory_hints: wgpu::MemoryHints::Performance,
                },
                None,
            )
            .await?;

        // Create shader module
        let shader = device.create_shader_module(wgpu::ShaderModuleDescriptor {
            label: Some("Shallow Water Compute Shader"),
            source: wgpu::ShaderSource::Wgsl(include_str!("shaders/shallow_water.wgsl").into()),
        });

        // Create compute pipeline
        let compute_pipeline = device.create_compute_pipeline(&wgpu::ComputePipelineDescriptor {
            label: Some("Shallow Water Pipeline"),
            layout: None,
            module: &shader,
            entry_point: Some("main"),
            compilation_options: Default::default(),
            cache: None,
        });

        // Create buffers
        let state_buffer = device.create_buffer(&wgpu::BufferDescriptor {
            label: Some("State Buffer"),
            size: (n_triangles * std::mem::size_of::<GpuState>()) as u64,
            usage: wgpu::BufferUsages::STORAGE
                | wgpu::BufferUsages::COPY_DST
                | wgpu::BufferUsages::COPY_SRC,
            mapped_at_creation: false,
        });

        let output_buffer = device.create_buffer(&wgpu::BufferDescriptor {
            label: Some("Output Buffer"),
            size: (n_triangles * std::mem::size_of::<GpuState>()) as u64,
            usage: wgpu::BufferUsages::STORAGE
                | wgpu::BufferUsages::COPY_SRC
                | wgpu::BufferUsages::MAP_READ,
            mapped_at_creation: false,
        });

        Ok(GpuSolver {
            device,
            queue,
            compute_pipeline,
            state_buffer,
            output_buffer,
            n_triangles,
        })
    }

    pub fn upload_state(&self, h: &[f64], hu: &[f64], hv: &[f64]) {
        let gpu_state: Vec<GpuState> = (0..self.n_triangles)
            .map(|i| GpuState {
                h: h[i] as f32,
                hu: hu[i] as f32,
                hv: hv[i] as f32,
                _padding: 0.0,
            })
            .collect();

        self.queue
            .write_buffer(&self.state_buffer, 0, bytemuck::cast_slice(&gpu_state));
    }

    pub async fn compute_step(&self) -> Result<Vec<GpuState>, Box<dyn std::error::Error>> {
        let mut encoder = self
            .device
            .create_command_encoder(&wgpu::CommandEncoderDescriptor {
                label: Some("Compute Encoder"),
            });

        {
            let mut compute_pass = encoder.begin_compute_pass(&wgpu::ComputePassDescriptor {
                label: Some("Shallow Water Compute Pass"),
                timestamp_writes: None,
            });

            compute_pass.set_pipeline(&self.compute_pipeline);
            let workgroup_size = 64;
            let num_workgroups = (self.n_triangles + workgroup_size - 1) / workgroup_size;
            compute_pass.dispatch_workgroups(num_workgroups as u32, 1, 1);
        }

        encoder.copy_buffer_to_buffer(
            &self.state_buffer,
            0,
            &self.output_buffer,
            0,
            (self.n_triangles * std::mem::size_of::<GpuState>()) as u64,
        );

        self.queue.submit(Some(encoder.finish()));

        // Read back results
        let buffer_slice = self.output_buffer.slice(..);
        let (tx, rx) = futures::channel::oneshot::channel();
        buffer_slice.map_async(wgpu::MapMode::Read, move |result| {
            tx.send(result).unwrap();
        });

        self.device.poll(wgpu::Maintain::Wait);
        rx.await??;

        let data = buffer_slice.get_mapped_range();
        let result: Vec<GpuState> = bytemuck::cast_slice(&data).to_vec();
        drop(data);
        self.output_buffer.unmap();

        Ok(result)
    }
}

// CPU fallback when GPU feature is not enabled
#[cfg(not(feature = "gpu"))]
pub struct GpuSolver;

#[cfg(not(feature = "gpu"))]
impl GpuSolver {
    pub fn new(_n_triangles: usize) -> Result<Self, Box<dyn std::error::Error>> {
        Err("GPU support not compiled. Enable 'gpu' feature.".into())
    }
}
