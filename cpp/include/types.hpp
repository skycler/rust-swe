#pragma once

#include <array>
#include <cmath>
#include <cstddef>
#include <vector>

namespace swe {

// Floating point type
using Real = double;

// 2D Point
struct Point {
    Real x, y;

    Point() : x(0.0), y(0.0) {}
    Point(Real x_, Real y_) : x(x_), y(y_) {}

    Point operator+(const Point& other) const {
        return Point(x + other.x, y + other.y);
    }

    Point operator-(const Point& other) const {
        return Point(x - other.x, y - other.y);
    }

    Point operator*(Real scalar) const {
        return Point(x * scalar, y * scalar);
    }

    Point operator/(Real scalar) const {
        return Point(x / scalar, y / scalar);
    }

    Real dot(const Point& other) const {
        return x * other.x + y * other.y;
    }

    Real norm() const {
        return std::sqrt(x * x + y * y);
    }

    Real cross(const Point& other) const {
        return x * other.y - y * other.x;
    }
};

// State vector for shallow water equations
struct State {
    Real h;   // Water depth
    Real hu;  // Momentum x-direction
    Real hv;  // Momentum y-direction

    State() : h(0.0), hu(0.0), hv(0.0) {}
    State(Real h_, Real hu_, Real hv_) : h(h_), hu(hu_), hv(hv_) {}

    State operator+(const State& other) const {
        return State(h + other.h, hu + other.hu, hv + other.hv);
    }

    State operator-(const State& other) const {
        return State(h - other.h, hu - other.hu, hv - other.hv);
    }

    State operator*(Real scalar) const {
        return State(h * scalar, hu * scalar, hv * scalar);
    }

    State operator/(Real scalar) const {
        return State(h / scalar, hu / scalar, hv / scalar);
    }

    State& operator+=(const State& other) {
        h += other.h;
        hu += other.hu;
        hv += other.hv;
        return *this;
    }
};

// Triangle in mesh
struct Triangle {
    std::array<size_t, 3> nodes;  // Node indices
    Point centroid;
    Real area;

    Triangle() : nodes({0, 0, 0}), area(0.0) {}
};

// Edge in mesh
struct Edge {
    std::array<size_t, 2> nodes;      // Node indices
    std::array<int, 2> triangles;     // Triangle indices (-1 for boundary)
    Point midpoint;
    Real length;
    Point normal;                      // Normal vector (unit)

    Edge() : nodes({0, 0}), triangles({-1, -1}), length(0.0) {}
};

// Boundary condition type
enum class BoundaryType {
    Wall,
    Open,
    Inflow,
    Outflow
};

// Boundary condition
struct BoundaryCondition {
    BoundaryType type;
    State value;  // For inflow conditions

    BoundaryCondition() : type(BoundaryType::Wall) {}
    BoundaryCondition(BoundaryType t) : type(t) {}
    BoundaryCondition(BoundaryType t, const State& v) : type(t), value(v) {}
};

} // namespace swe
