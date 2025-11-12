#include "types.hpp"
#include <gtest/gtest.h>
#include <cmath>

using namespace swe;

TEST(TypusTest, PointConstructor) {
    Point p(1.0, 2.0);
    EXPECT_DOUBLE_EQ(p.x, 1.0);
    EXPECT_DOUBLE_EQ(p.y, 2.0);
}

TEST(TypesTest, PointAddition) {
    Point p1(1.0, 2.0);
    Point p2(3.0, 4.0);
    Point p3 = p1 + p2;
    
    EXPECT_DOUBLE_EQ(p3.x, 4.0);
    EXPECT_DOUBLE_EQ(p3.y, 6.0);
}

TEST(TypesTest, PointSubtraction) {
    Point p1(5.0, 7.0);
    Point p2(2.0, 3.0);
    Point p3 = p1 - p2;
    
    EXPECT_DOUBLE_EQ(p3.x, 3.0);
    EXPECT_DOUBLE_EQ(p3.y, 4.0);
}

TEST(TypesTest, PointScalarMultiplication) {
    Point p(2.0, 3.0);
    Point p2 = p * 2.5;
    
    EXPECT_DOUBLE_EQ(p2.x, 5.0);
    EXPECT_DOUBLE_EQ(p2.y, 7.5);
}

TEST(TypesTest, PointDotProduct) {
    Point p1(1.0, 2.0);
    Point p2(3.0, 4.0);
    Real dot = p1.dot(p2);
    
    EXPECT_DOUBLE_EQ(dot, 11.0);
}

TEST(TypesTest, PointNorm) {
    Point p(3.0, 4.0);
    EXPECT_DOUBLE_EQ(p.norm(), 5.0);
}

TEST(TypesTest, PointCrossProduct) {
    Point p1(1.0, 0.0);
    Point p2(0.0, 1.0);
    Real cross = p1.cross(p2);
    
    EXPECT_DOUBLE_EQ(cross, 1.0);
}

TEST(TypesTest, StateConstructor) {
    State s(1.5, 2.0, 3.0);
    EXPECT_DOUBLE_EQ(s.h, 1.5);
    EXPECT_DOUBLE_EQ(s.hu, 2.0);
    EXPECT_DOUBLE_EQ(s.hv, 3.0);
}

TEST(TypesTest, StateAddition) {
    State s1(1.0, 2.0, 3.0);
    State s2(0.5, 1.0, 1.5);
    State s3 = s1 + s2;
    
    EXPECT_DOUBLE_EQ(s3.h, 1.5);
    EXPECT_DOUBLE_EQ(s3.hu, 3.0);
    EXPECT_DOUBLE_EQ(s3.hv, 4.5);
}

TEST(TypesTest, StateScalarMultiplication) {
    State s(2.0, 4.0, 6.0);
    State s2 = s * 0.5;
    
    EXPECT_DOUBLE_EQ(s2.h, 1.0);
    EXPECT_DOUBLE_EQ(s2.hu, 2.0);
    EXPECT_DOUBLE_EQ(s2.hv, 3.0);
}
