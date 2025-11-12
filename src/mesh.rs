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

#[derive(Clone)]
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

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_mesh_creation_basic() {
        let mesh = TriangularMesh::new_rectangular(3, 3, 1.0, 1.0, TopographyType::Flat);
        
        // Should have 3x3 = 9 nodes
        assert_eq!(mesh.nodes.len(), 9);
        
        // Should have 2 triangles per cell = 2*(3-1)*(3-1) = 8 triangles
        assert_eq!(mesh.triangles.len(), 8);
        
        // All nodes should have z = 0 for flat topography
        for node in &mesh.nodes {
            assert_eq!(node.z, 0.0);
        }
    }

    #[test]
    fn test_mesh_dimensions() {
        let width = 10.0;
        let height = 5.0;
        let mesh = TriangularMesh::new_rectangular(11, 6, width, height, TopographyType::Flat);
        
        // Check boundary nodes
        assert_eq!(mesh.nodes[0].x, 0.0);
        assert_eq!(mesh.nodes[0].y, 0.0);
        
        // Check last node
        let last_node = mesh.nodes.last().unwrap();
        assert!((last_node.x - width).abs() < 1e-10);
        assert!((last_node.y - height).abs() < 1e-10);
    }

    #[test]
    fn test_triangle_area_positive() {
        let mesh = TriangularMesh::new_rectangular(5, 5, 10.0, 10.0, TopographyType::Flat);
        
        // All triangles should have positive area
        for tri in &mesh.triangles {
            assert!(tri.area > 0.0, "Triangle area should be positive");
        }
    }

    #[test]
    fn test_topography_flat() {
        let mesh = TriangularMesh::new_rectangular(5, 5, 10.0, 10.0, TopographyType::Flat);
        
        for tri in &mesh.triangles {
            assert_eq!(tri.z_bed, 0.0);
        }
    }

    #[test]
    fn test_topography_slope() {
        let gradient_x = 0.1;
        let gradient_y = 0.05;
        let mesh = TriangularMesh::new_rectangular(
            5, 5, 10.0, 10.0,
            TopographyType::Slope { gradient_x, gradient_y }
        );
        
        // Check that bed elevation increases with x and y
        let node_00 = &mesh.nodes[0]; // (0, 0)
        let node_max = mesh.nodes.last().unwrap(); // (10, 10)
        
        assert!(node_max.z > node_00.z);
        
        // Check approximate slope
        let expected_z = gradient_x * node_max.x + gradient_y * node_max.y;
        assert!((node_max.z - expected_z).abs() < 1e-10);
    }

    #[test]
    fn test_topography_gaussian() {
        let center = (5.0, 5.0);
        let amplitude = 2.0;
        let width = 2.0;
        let mesh = TriangularMesh::new_rectangular(
            11, 11, 10.0, 10.0,
            TopographyType::Gaussian { center, amplitude, width }
        );
        
        // Find node closest to center
        let center_node = mesh.nodes.iter()
            .min_by(|a, b| {
                let dist_a = ((a.x - center.0).powi(2) + (a.y - center.1).powi(2)).sqrt();
                let dist_b = ((b.x - center.0).powi(2) + (b.y - center.1).powi(2)).sqrt();
                dist_a.partial_cmp(&dist_b).unwrap()
            })
            .unwrap();
        
        // Center should have highest elevation (close to amplitude)
        assert!(center_node.z > 0.8 * amplitude, "Center should be near peak amplitude");
        
        // Check Gaussian decay
        for node in &mesh.nodes {
            let r2 = (node.x - center.0).powi(2) + (node.y - center.1).powi(2);
            let expected = amplitude * (-r2 / width.powi(2)).exp();
            assert!((node.z - expected).abs() < 1e-10);
        }
    }

    #[test]
    fn test_edges_generation() {
        let mesh = TriangularMesh::new_rectangular(4, 4, 10.0, 10.0, TopographyType::Flat);
        
        // Should have edges (Euler formula for planar graphs)
        assert!(mesh.edges.len() > 0);
        
        // All edges should have positive length
        for edge in &mesh.edges {
            assert!(edge.length > 0.0);
        }
        
        // Normal vectors should be unit vectors
        for edge in &mesh.edges {
            let norm = (edge.normal.0.powi(2) + edge.normal.1.powi(2)).sqrt();
            assert!((norm - 1.0).abs() < 1e-10, "Normal should be unit vector");
        }
    }

    #[test]
    fn test_neighbor_connectivity() {
        let mesh = TriangularMesh::new_rectangular(4, 4, 10.0, 10.0, TopographyType::Flat);
        
        // Check that neighbor references are valid
        for tri in &mesh.triangles {
            for neighbor_id in &tri.neighbors {
                if let Some(id) = neighbor_id {
                    assert!(*id < mesh.triangles.len(), "Neighbor ID should be valid");
                }
            }
        }
    }

    #[test]
    fn test_mesh_consistency() {
        let mesh = TriangularMesh::new_rectangular(5, 5, 10.0, 10.0, TopographyType::Flat);
        
        // Total number of nodes should match grid size
        let nx = 5;
        let ny = 5;
        assert_eq!(mesh.nodes.len(), nx * ny);
        
        // Number of triangles = 2 * (nx-1) * (ny-1)
        let expected_triangles = 2 * (nx - 1) * (ny - 1);
        assert_eq!(mesh.triangles.len(), expected_triangles);
    }
}
