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
    EXPECT_QUATERNION_NEAR(qn * conjugate(qn), id, F_T_EPS);
}

TEST(Quaternion, Rotation)
{
    quaternion rotq = normalize(q);
    v4 v(-1.5, 1.6, -.2, 0.); quaternion w(0, v[0], v[1], v[2]);
    quaternion rotv = rotq * w * conjugate(rotq); v4 rotw(rotv.x(), rotv.y(),rotv.z(), rotv.w());
    EXPECT_V4_NEAR(rotq * v, rotw, F_T_EPS) << "rotq * v == unqaternion(rotq * quaternion(v) * conjugate(rotq))";
    EXPECT_V4_NEAR(rotq * v, to_rotation_matrix(rotq) * v, F_T_EPS);
}
