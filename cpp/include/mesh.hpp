#pragma once

#include "types.hpp"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace swe {

class Mesh {
public:
    // Constructors
    Mesh() = default;

    // Static factory methods
    static std::shared_ptr<Mesh> create_rectangular(
        Real width, Real height,
        size_t nx, size_t ny
    );

    static std::shared_ptr<Mesh> from_file(const std::string& filename);

    // Mesh initialization
    void compute_geometry();
    void build_connectivity();

    // Getters
    const std::vector<Point>& get_nodes() const { return nodes_; }
    const std::vector<Triangle>& get_triangles() const { return triangles_; }
    const std::vector<Edge>& get_edges() const { return edges_; }
    
    size_t num_nodes() const { return nodes_.size(); }
    size_t num_triangles() const { return triangles_.size(); }
    size_t num_edges() const { return edges_.size(); }

    // Mesh queries
    std::vector<size_t> get_node_triangles(size_t node_id) const;
    std::vector<size_t> get_node_edges(size_t node_id) const;
    bool is_boundary_node(size_t node_id) const;
    bool is_boundary_edge(size_t edge_id) const;

    // Setters (for mesh construction)
    void add_node(const Point& p);
    void add_triangle(const std::array<size_t, 3>& nodes);

    // Statistics
    Real min_edge_length() const;
    Real max_edge_length() const;
    Real total_area() const;

private:
    std::vector<Point> nodes_;
    std::vector<Triangle> triangles_;
    std::vector<Edge> edges_;

    // Connectivity information
    std::unordered_map<size_t, std::vector<size_t>> node_to_triangles_;
    std::unordered_map<size_t, std::vector<size_t>> node_to_edges_;

    // Helper methods
    void compute_triangle_geometry(Triangle& tri);
    void compute_edge_geometry(Edge& edge);
    size_t find_or_create_edge(size_t n1, size_t n2, size_t tri_id);
    
    // Edge map for avoiding duplicates
    std::unordered_map<uint64_t, size_t> edge_map_;
    
    static uint64_t edge_key(size_t n1, size_t n2) {
        if (n1 > n2) std::swap(n1, n2);
        return (static_cast<uint64_t>(n1) << 32) | n2;
    }
};

} // namespace swe
