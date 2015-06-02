#include "gtest/gtest.h"
#include "quaternion.h"
#include "util.h"

const static quaternion q(.55, -1.2, -.15, 1.6);

TEST(Quaternion, Identity)
{
    quaternion id(1., 0., 0., 0.);
    EXPECT_QUATERNION_NEAR(q * id, q, 0);
    EXPECT_QUATERNION_NEAR(id * q, q, 0);
    quaternion qn = normalize(q);
    EXPECT_QUATERNION_NEAR(qn * conjugate(qn), id, 1.e-15);
}

TEST(Quaternion, Cross)
{
    v4 v(-1.5, 1.6, -.2, 0.);
    v4 qvec (q.x(), q.y(), q.z(), 0.);
    EXPECT_V4_NEAR(qvec_cross(q, v), cross(qvec, v), 0);
}

TEST(Quaternion, Rotation)
{
    quaternion rotq = normalize(q);
    v4 v(-1.5, 1.6, -.2, 0.); quaternion w(0, v[0], v[1], v[2]);
    quaternion rotv = rotq * w * conjugate(rotq); v4 rotw(rotv.x(), rotv.y(),rotv.z(), rotv.w());
    EXPECT_V4_NEAR(rotq * v, rotw, 1.e-15) << "rotq * v == unqaternion(rotq * quaternion(v) * conjugate(rotq))";
    EXPECT_V4_NEAR(rotq * v, to_rotation_matrix(rotq) * v, 1.e-15);
}

TEST(Quaternion, IntegrateAngularVelocity)
{
    v4 angvel(.13, -.012, .4, 0.);
    quaternion p = q * quaternion(0., angvel[0], angvel[1], angvel[2]);
    quaternion res = quaternion(q.w() + .5 * p.w(), q.x() + .5 * p.x(), q.y() + .5 * p.y(), q.z() + .5 * p.z());
    EXPECT_QUATERNION_NEAR(integrate_angular_velocity(q, angvel), res, 0);
}
