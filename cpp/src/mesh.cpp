#include "mesh.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace swe {

std::shared_ptr<Mesh> Mesh::create_rectangular(Real width, Real height, size_t nx, size_t ny) {
    auto mesh = std::make_shared<Mesh>();
    
    const Real dx = width / static_cast<Real>(nx);
    const Real dy = height / static_cast<Real>(ny);

    // Create nodes
    for (size_t j = 0; j <= ny; ++j) {
        for (size_t i = 0; i <= nx; ++i) {
            Real x = i * dx;
            Real y = j * dy;
            mesh->add_node(Point(x, y));
        }
    }

    // Create triangles
    for (size_t j = 0; j < ny; ++j) {
        for (size_t i = 0; i < nx; ++i) {
            size_t n0 = j * (nx + 1) + i;
            size_t n1 = n0 + 1;
            size_t n2 = n0 + (nx + 1);
            size_t n3 = n2 + 1;

            // Two triangles per quad
            mesh->add_triangle({n0, n1, n2});
            mesh->add_triangle({n1, n3, n2});
        }
    }

    mesh->build_connectivity();
    mesh->compute_geometry();

    return mesh;
}

void Mesh::add_node(const Point& p) {
    nodes_.push_back(p);
}

void Mesh::add_triangle(const std::array<size_t, 3>& nodes) {
    Triangle tri;
    tri.nodes = nodes;
    triangles_.push_back(tri);
}

void Mesh::compute_geometry() {
    for (auto& tri : triangles_) {
        compute_triangle_geometry(tri);
    }

    for (auto& edge : edges_) {
        compute_edge_geometry(edge);
    }
}

void Mesh::compute_triangle_geometry(Triangle& tri) {
    const Point& p0 = nodes_[tri.nodes[0]];
    const Point& p1 = nodes_[tri.nodes[1]];
    const Point& p2 = nodes_[tri.nodes[2]];

    // Centroid
    tri.centroid = (p0 + p1 + p2) / 3.0;

    // Area using cross product
    Point v1 = p1 - p0;
    Point v2 = p2 - p0;
    tri.area = std::abs(v1.cross(v2)) * 0.5;
}

void Mesh::compute_edge_geometry(Edge& edge) {
    const Point& p0 = nodes_[edge.nodes[0]];
    const Point& p1 = nodes_[edge.nodes[1]];

    // Midpoint
    edge.midpoint = (p0 + p1) * 0.5;

    // Length
    Point diff = p1 - p0;
    edge.length = diff.norm();

    // Normal vector (perpendicular, pointing right)
    edge.normal = Point(-diff.y, diff.x) / edge.length;
}

void Mesh::build_connectivity() {
    edge_map_.clear();
    edges_.clear();

    for (size_t tri_id = 0; tri_id < triangles_.size(); ++tri_id) {
        const auto& tri = triangles_[tri_id];

        // Three edges per triangle
        for (size_t i = 0; i < 3; ++i) {
            size_t n1 = tri.nodes[i];
            size_t n2 = tri.nodes[(i + 1) % 3];
            
            size_t edge_id = find_or_create_edge(n1, n2, tri_id);

            // Update node connectivity
            node_to_edges_[n1].push_back(edge_id);
            node_to_edges_[n2].push_back(edge_id);
            node_to_triangles_[n1].push_back(tri_id);
            node_to_triangles_[n2].push_back(tri_id);
        }
    }

    // Remove duplicate entries in connectivity maps
    for (auto& [node_id, tri_list] : node_to_triangles_) {
        std::sort(tri_list.begin(), tri_list.end());
        tri_list.erase(std::unique(tri_list.begin(), tri_list.end()), tri_list.end());
    }

    for (auto& [node_id, edge_list] : node_to_edges_) {
        std::sort(edge_list.begin(), edge_list.end());
        edge_list.erase(std::unique(edge_list.begin(), edge_list.end()), edge_list.end());
    }
}

size_t Mesh::find_or_create_edge(size_t n1, size_t n2, size_t tri_id) {
    uint64_t key = edge_key(n1, n2);
    
    auto it = edge_map_.find(key);
    if (it != edge_map_.end()) {
        // Edge already exists, update second triangle
        size_t edge_id = it->second;
        edges_[edge_id].triangles[1] = static_cast<int>(tri_id);
        return edge_id;
    }

    // Create new edge
    size_t edge_id = edges_.size();
    Edge edge;
    edge.nodes = {std::min(n1, n2), std::max(n1, n2)};
    edge.triangles[0] = static_cast<int>(tri_id);
    edge.triangles[1] = -1;  // Boundary until another triangle is found
    
    edges_.push_back(edge);
    edge_map_[key] = edge_id;
    
    return edge_id;
}

std::vector<size_t> Mesh::get_node_triangles(size_t node_id) const {
    auto it = node_to_triangles_.find(node_id);
    if (it != node_to_triangles_.end()) {
        return it->second;
    }
    return {};
}

std::vector<size_t> Mesh::get_node_edges(size_t node_id) const {
    auto it = node_to_edges_.find(node_id);
    if (it != node_to_edges_.end()) {
        return it->second;
    }
    return {};
}

bool Mesh::is_boundary_node(size_t node_id) const {
    auto edges = get_node_edges(node_id);
    for (size_t edge_id : edges) {
        if (is_boundary_edge(edge_id)) {
            return true;
        }
    }
    return false;
}

bool Mesh::is_boundary_edge(size_t edge_id) const {
    return edges_[edge_id].triangles[1] == -1;
}

Real Mesh::min_edge_length() const {
    if (edges_.empty()) return 0.0;
    
    Real min_len = edges_[0].length;
    for (const auto& edge : edges_) {
        min_len = std::min(min_len, edge.length);
    }
    return min_len;
}

Real Mesh::max_edge_length() const {
    if (edges_.empty()) return 0.0;
    
    Real max_len = edges_[0].length;
    for (const auto& edge : edges_) {
        max_len = std::max(max_len, edge.length);
    }
    return max_len;
}

Real Mesh::total_area() const {
    Real area = 0.0;
    for (const auto& tri : triangles_) {
        area += tri.area;
    }
    return area;
}

} // namespace swe
