#include "solver.hpp"
#include "mesh.hpp"
#include <gtest/gtest.h>
#include <cmath>

using namespace swe;

class SolverTest : public ::testing::Test {
protected:
    void SetUp() override {
        mesh = Mesh::create_rectangular(10.0, 10.0, 5, 5);
        
        SolverParameters params;
        params.gravity = 9.81;
        params.cfl = 0.5;
        
        solver = std::make_unique<Solver>(mesh, params);
    }

    std::shared_ptr<Mesh> mesh;
    std::unique_ptr<Solver> solver;
};

TEST_F(SolverTest, Initialization) {
    EXPECT_EQ(solver->get_time(), 0.0);
    EXPECT_EQ(solver->get_step_count(), 0);
}

TEST_F(SolverTest, SetConstantState) {
    State state(1.0, 0.0, 0.0);
    solver->set_constant_state(state);
    
    const auto& current_state = solver->get_state();
    EXPECT_EQ(current_state.size(), mesh->num_triangles());
    
    for (const auto& s : current_state) {
        EXPECT_DOUBLE_EQ(s.h, 1.0);
        EXPECT_DOUBLE_EQ(s.hu, 0.0);
        EXPECT_DOUBLE_EQ(s.hv, 0.0);
    }
}

TEST_F(SolverTest, MassConservation) {
    State state(2.0, 0.0, 0.0);
    solver->set_constant_state(state);
    solver->set_all_boundaries(BoundaryCondition(BoundaryType::Wall));
    
    Real initial_mass = solver->total_mass();
    
    // Run a few steps
    for (int i = 0; i < 10; ++i) {
        Real dt = solver->compute_time_step();
        solver->step(dt);
    }
    
    Real final_mass = solver->total_mass();
    
    // Mass should be conserved (within numerical precision)
    EXPECT_NEAR(initial_mass, final_mass, initial_mass * 1e-10);
}

TEST_F(SolverTest, StillWater) {
    // Still water should remain still
    State state(1.0, 0.0, 0.0);
    solver->set_constant_state(state);
    solver->set_all_boundaries(BoundaryCondition(BoundaryType::Wall));
    
    // Run a few steps
    for (int i = 0; i < 10; ++i) {
        Real dt = solver->compute_time_step();
        solver->step(dt);
    }
    
    // Check that state hasn't changed significantly
    const auto& current_state = solver->get_state();
    for (const auto& s : current_state) {
        EXPECT_NEAR(s.h, 1.0, 1e-8);
        EXPECT_NEAR(s.hu, 0.0, 1e-8);
        EXPECT_NEAR(s.hv, 0.0, 1e-8);
    }
}

TEST_F(SolverTest, TimeStepComputation) {
    State state(1.0, 0.0, 0.0);
    solver->set_constant_state(state);
    
    Real dt = solver->compute_time_step();
    
    EXPECT_GT(dt, 0.0);
    EXPECT_LT(dt, 1.0);  // Reasonable time step
}

TEST_F(SolverTest, AdvanceToTime) {
    State state(1.0, 0.0, 0.0);
    solver->set_constant_state(state);
    solver->set_all_boundaries(BoundaryCondition(BoundaryType::Wall));
    
    solver->advance_to_time(0.5);
    
    EXPECT_NEAR(solver->get_time(), 0.5, 1e-10);
    EXPECT_GT(solver->get_step_count(), 0);
}

TEST_F(SolverTest, MaxWaveSpeed) {
    State state(1.0, 1.0, 0.0);  // h=1, u=1, v=0
    solver->set_constant_state(state);
    
    Real max_speed = solver->max_wave_speed();
    
    // Should be approximately u + sqrt(g*h) = 1 + sqrt(9.81*1)
    Real expected = 1.0 + std::sqrt(9.81);
    EXPECT_NEAR(max_speed, expected, 0.1);
}

TEST_F(SolverTest, EnergyPositive) {
    State state(1.5, 0.5, 0.3);
    solver->set_constant_state(state);
    
    Real energy = solver->total_energy();
    
    EXPECT_GT(energy, 0.0);
}

TEST_F(SolverTest, BoundaryConditions) {
    State state(1.0, 0.0, 0.0);
    solver->set_constant_state(state);
    
    // Set different boundary conditions
    solver->set_all_boundaries(BoundaryCondition(BoundaryType::Wall));
    
    // Just check it doesn't crash
    Real dt = solver->compute_time_step();
    solver->step(dt);
    
    EXPECT_GT(solver->get_step_count(), 0);
}

TEST_F(SolverTest, DryTolerance) {
    // Very small water depth should be treated as dry
    State state(1e-10, 0.0, 0.0);
    solver->set_constant_state(state);
    
    Real dt = solver->compute_time_step();
    solver->step(dt);
    
    // Should not crash or produce NaN
    const auto& current_state = solver->get_state();
    for (const auto& s : current_state) {
        EXPECT_FALSE(std::isnan(s.h));
        EXPECT_FALSE(std::isnan(s.hu));
        EXPECT_FALSE(std::isnan(s.hv));
    }
}
