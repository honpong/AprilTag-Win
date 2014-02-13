#include "gtest/gtest.h"
#include "quaternion.h"
#include "util.h"

const static quaternion q(.55, -1.2, -.15, 1.6);

TEST(Quaternion, Identity)
{
    quaternion id(1., 0., 0., 0.);
    EXPECT_EQ(quaternion_product(q, id), q);
    EXPECT_EQ(quaternion_product(id, q), q);
    quaternion qn = normalize(q);
    test_quaternion_near(quaternion_product(qn, conjugate(qn)), id, 1.e-15);
}

TEST(Quaternion, Cross)
{
    v4 v(-1.5, 1.6, -.2, 0.);
    v4 qvec (q.x(), q.y(), q.z(), 0.);
    EXPECT_EQ(qvec_cross(q, v), cross(qvec, v));
}

TEST(Quaternion, Rotation)
{
    quaternion rotq = normalize(q);
    v4 v(-1.5, 1.6, -.2, 0.);
    test_v4_near(quaternion_rotate(rotq,v), to_rotation_matrix(rotq)*v, 1.e-15);
}

TEST(Quaternion, ProductJacobian)
{
    quaternion q2(.1, -4.5, .0001, 1.493);
    {
        SCOPED_TRACE("left");
        v4 a = v4(q.w(), q.x(), q.y(), q.z());
        v4 b = quaternion_product_left_jacobian(q2) * a;
        EXPECT_EQ(quaternion_product(q, q2), quaternion(b[0], b[1], b[2], b[3]));
        a = v4(q2.w(), q2.x(), q2.y(), q2.z());
        b = quaternion_product_left_jacobian(q) * a;
        EXPECT_EQ(quaternion_product(q2, q), quaternion(b[0], b[1], b[2], b[3]));
    }
    {
        SCOPED_TRACE("right");
        v4 a = v4(q2.w(), q2.x(), q2.y(), q2.z());
        v4 b = quaternion_product_right_jacobian(q) * a;
        EXPECT_EQ(quaternion_product(q, q2), quaternion(b[0], b[1], b[2], b[3]));
        a = v4(q.w(), q.x(), q.y(), q.z());
        b = quaternion_product_right_jacobian(q2) * a;
        EXPECT_EQ(quaternion_product(q2, q), quaternion(b[0], b[1], b[2], b[3]));
    }
}

TEST(Quaternion, IntegrateAngularVelocity)
{
    v4 angvel(.13, -.012, .4, 0.);
    quaternion p = quaternion_product(q, quaternion(0., angvel[0], angvel[1], angvel[2]));
    quaternion res = quaternion(q.w() + .5 * p.w(), q.x() + .5 * p.x(), q.y() + .5 * p.y(), q.z() + .5 * p.z());
    EXPECT_EQ(integrate_angular_velocity(q, angvel), res);
}
