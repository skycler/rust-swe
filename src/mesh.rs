/// Triangular mesh data structures and operations
use std::f64;

#[derive(Debug, Clone)]
pub struct Node {
    pub x: f64,
    pub y: f64,
    pub z: f64,  // Bottom elevation (bathymetry/topography)
}

#[derive(Debug, Clone)]
pub struct Triangle {
    pub id: usize,
    pub nodes: [usize; 3], // Node indices
    pub neighbors: [Option<usize>; 3], // Neighboring triangle indices
    pub area: f64,
    pub centroid: (f64, f64),
    pub z_bed: f64, // Average bed elevation
}

#[derive(Debug, Clone)]
pub struct Edge {
    pub length: f64,
    pub normal: (f64, f64), // Unit normal vector
    pub left_triangle: usize,
    pub right_triangle: Option<usize>, // None for boundary edges
}

pub struct TriangularMesh {
    pub nodes: Vec<Node>,
    pub triangles: Vec<Triangle>,
    pub edges: Vec<Edge>,
}

#[derive(Debug, Clone, Copy)]
pub enum TopographyType {
    Flat,
    Slope { gradient_x: f64, gradient_y: f64 },
    Gaussian { center: (f64, f64), amplitude: f64, width: f64 },
    Channel { depth: f64, width: f64 },
}

impl TriangularMesh {
    /// Create a simple rectangular domain with triangular mesh
    pub fn new_rectangular(
        nx: usize,
        ny: usize,
        width: f64,
        height: f64,
        topography: TopographyType,
    ) -> Self {
        let dx = width / (nx - 1) as f64;
        let dy = height / (ny - 1) as f64;
        
        // Generate nodes
        let mut nodes = Vec::new();
        for j in 0..ny {
            for i in 0..nx {
                let x = i as f64 * dx;
                let y = j as f64 * dy;
                let z = Self::compute_topography(x, y, topography);
                
                nodes.push(Node { x, y, z });
            }
        }
        
        // Generate triangles (two per rectangular cell)
        let mut triangles = Vec::new();
        let mut tri_id = 0;
        
        for j in 0..(ny - 1) {
            for i in 0..(nx - 1) {
                let n0 = j * nx + i;
                let n1 = j * nx + i + 1;
                let n2 = (j + 1) * nx + i;
                let n3 = (j + 1) * nx + i + 1;
                
                // Lower triangle
                let area1 = Self::compute_area(&nodes[n0], &nodes[n1], &nodes[n2]);
                let centroid1 = Self::compute_centroid(&nodes[n0], &nodes[n1], &nodes[n2]);
                let z_bed1 = (nodes[n0].z + nodes[n1].z + nodes[n2].z) / 3.0;
                triangles.push(Triangle {
                    id: tri_id,
                    nodes: [n0, n1, n2],
                    neighbors: [None, None, None],
                    area: area1,
                    centroid: centroid1,
                    z_bed: z_bed1,
                });
                tri_id += 1;
                
                // Upper triangle
                let area2 = Self::compute_area(&nodes[n1], &nodes[n3], &nodes[n2]);
                let centroid2 = Self::compute_centroid(&nodes[n1], &nodes[n3], &nodes[n2]);
                let z_bed2 = (nodes[n1].z + nodes[n3].z + nodes[n2].z) / 3.0;
                triangles.push(Triangle {
                    id: tri_id,
                    nodes: [n1, n3, n2],
                    neighbors: [None, None, None],
                    area: area2,
                    centroid: centroid2,
                    z_bed: z_bed2,
                });
                tri_id += 1;
            }
        }
        
        // Build neighbor connectivity
        Self::build_neighbors(&mut triangles);
        
        // Generate edges
        let edges = Self::generate_edges(&nodes, &triangles);
        
        TriangularMesh {
            nodes,
            triangles,
            edges,
        }
    }
    
    fn compute_area(n0: &Node, n1: &Node, n2: &Node) -> f64 {
        0.5 * ((n1.x - n0.x) * (n2.y - n0.y) - (n2.x - n0.x) * (n1.y - n0.y)).abs()
    }
    
    fn compute_centroid(n0: &Node, n1: &Node, n2: &Node) -> (f64, f64) {
        ((n0.x + n1.x + n2.x) / 3.0, (n0.y + n1.y + n2.y) / 3.0)
    }
    
    fn build_neighbors(triangles: &mut [Triangle]) {
        for i in 0..triangles.len() {
            for j in (i + 1)..triangles.len() {
                let shared = Self::count_shared_nodes(&triangles[i], &triangles[j]);
                if shared == 2 {
                    // These triangles are neighbors
                    let edge_i = Self::find_edge_index(&triangles[i], &triangles[j]);
                    let edge_j = Self::find_edge_index(&triangles[j], &triangles[i]);
                    
                    triangles[i].neighbors[edge_i] = Some(j);
                    triangles[j].neighbors[edge_j] = Some(i);
                }
            }
        }
    }
    
    fn count_shared_nodes(t1: &Triangle, t2: &Triangle) -> usize {
        let mut count = 0;
        for n1 in &t1.nodes {
            for n2 in &t2.nodes {
                if n1 == n2 {
                    count += 1;
                }
            }
        }
        count
    }
    
    fn find_edge_index(t1: &Triangle, t2: &Triangle) -> usize {
        for i in 0..3 {
            let n0 = t1.nodes[i];
            let n1 = t1.nodes[(i + 1) % 3];
            
            if t2.nodes.contains(&n0) && t2.nodes.contains(&n1) {
                return i;
            }
        }
        0
    }
    
    fn generate_edges(nodes: &[Node], triangles: &[Triangle]) -> Vec<Edge> {
        let mut edges = Vec::new();
        let mut edge_set = std::collections::HashSet::new();
        
        for tri in triangles {
            for i in 0..3 {
                let n0 = tri.nodes[i];
                let n1 = tri.nodes[(i + 1) % 3];
                
                let edge_key = if n0 < n1 { (n0, n1) } else { (n1, n0) };
                
                if edge_set.insert(edge_key) {
                    let dx = nodes[n1].x - nodes[n0].x;
                    let dy = nodes[n1].y - nodes[n0].y;
                    let length = (dx * dx + dy * dy).sqrt();
                    
                    // Normal vector (pointing right relative to edge direction)
                    let normal = (-dy / length, dx / length);
                    
                    let right_triangle = tri.neighbors[i];
                    
                    edges.push(Edge {
                        length,
                        normal,
                        left_triangle: tri.id,
                        right_triangle,
                    });
                }
            }
        }
        
        edges
    }
    
    /// Compute topography/bathymetry at a given point
    fn compute_topography(x: f64, y: f64, topo: TopographyType) -> f64 {
        match topo {
            TopographyType::Flat => 0.0,
            TopographyType::Slope { gradient_x, gradient_y } => {
                gradient_x * x + gradient_y * y
            }
            TopographyType::Gaussian { center, amplitude, width } => {
                let dx = x - center.0;
                let dy = y - center.1;
                let r2 = dx * dx + dy * dy;
                amplitude * (-r2 / (width * width)).exp()
            }
            TopographyType::Channel { depth, width } => {
                // Parabolic channel cross-section in y-direction
                let y_center = 5.0; // Assume domain centered at y=5
                let dy = (y - y_center).abs();
                if dy < width / 2.0 {
                    -depth * (1.0 - (2.0 * dy / width).powi(2))
                } else {
                    0.0
                }
            }
        }
    }
}
