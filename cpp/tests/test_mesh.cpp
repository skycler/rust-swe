#include "mesh.hpp"
#include <gtest/gtest.h>

using namespace swe;

TEST(MeshTest, CreateRectangularMesh) {
    auto mesh = Mesh::create_rectangular(10.0, 10.0, 5, 5);
    
    ASSERT_NE(mesh, nullptr);
    EXPECT_EQ(mesh->num_nodes(), 36);      // (5+1) * (5+1)
    EXPECT_EQ(mesh->num_triangles(), 50);   // 5 * 5 * 2
}

TEST(MeshTest, MeshGeometry) {
    auto mesh = Mesh::create_rectangular(10.0, 10.0, 2, 2);
    
    // Check total area
    EXPECT_NEAR(mesh->total_area(), 100.0, 1e-10);
}

TEST(MeshTest, EdgeLengths) {
    auto mesh = Mesh::create_rectangular(10.0, 10.0, 2, 2);
    
    Real min_len = mesh->min_edge_length();
    Real max_len = mesh->max_edge_length();
    
    EXPECT_GT(min_len, 0.0);
    EXPECT_GE(max_len, min_len);
}

TEST(MeshTest, BoundaryDetection) {
    auto mesh = Mesh::create_rectangular(4.0, 4.0, 2, 2);
    
    // Corner node should be on boundary
    EXPECT_TRUE(mesh->is_boundary_node(0));
    
    // Check that we have boundary edges
    size_t boundary_count = 0;
    for (size_t i = 0; i < mesh->num_edges(); ++i) {
        if (mesh->is_boundary_edge(i)) {
            ++boundary_count;
        }
    }
    
    EXPECT_GT(boundary_count, 0);
}

TEST(MeshTest, Connectivity) {
    auto mesh = Mesh::create_rectangular(4.0, 4.0, 2, 2);
    
    // Each internal node should be connected to triangles
    auto node_triangles = mesh->get_node_triangles(4);  // Center node
    EXPECT_GT(node_triangles.size(), 0);
    
    auto node_edges = mesh->get_node_edges(4);
    EXPECT_GT(node_edges.size(), 0);
}

TEST(MeshTest, TriangleAreas) {
    auto mesh = Mesh::create_rectangular(2.0, 2.0, 1, 1);
    
    const auto& triangles = mesh->get_triangles();
    EXPECT_EQ(triangles.size(), 2);
    
    for (const auto& tri : triangles) {
        EXPECT_NEAR(tri.area, 2.0, 1e-10);  // Each triangle is half of 2x2 square
    }
}

TEST(MeshTest, Centroids) {
    auto mesh = Mesh::create_rectangular(3.0, 3.0, 1, 1);
    
    const auto& triangles = mesh->get_triangles();
    
    // Check that centroids are computed
    for (const auto& tri : triangles) {
        EXPECT_GE(tri.centroid.x, 0.0);
        EXPECT_LE(tri.centroid.x, 3.0);
        EXPECT_GE(tri.centroid.y, 0.0);
        EXPECT_LE(tri.centroid.y, 3.0);
    }
}
