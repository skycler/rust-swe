#include "types.hpp"

namespace swe {

// Boundary condition implementations
// This file contains helper functions for setting up common boundary conditions

BoundaryCondition wall_boundary() {
    return BoundaryCondition(BoundaryType::Wall);
}

BoundaryCondition open_boundary() {
    return BoundaryCondition(BoundaryType::Open);
}

BoundaryCondition inflow_boundary(Real h, Real hu, Real hv) {
    return BoundaryCondition(BoundaryType::Inflow, State(h, hu, hv));
}

BoundaryCondition outflow_boundary() {
    return BoundaryCondition(BoundaryType::Outflow);
}

} // namespace swe
