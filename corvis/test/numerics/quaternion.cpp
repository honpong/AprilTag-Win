#include "gtest/gtest.h"
#include "quaternion.h"
#include "util.h"

const static quaternion q(.55, -1.2, -.15, 1.6);

TEST(Quaternion, Identity)
{
    quaternion id(1., 0., 0., 0.);
    test_v4_equal(quaternion_product(q, id).data, q.data);
    test_v4_equal(quaternion_product(id, q).data, q.data);
}

TEST(Quaternion, Cross)
{
    v4 v(-1.5, 1.6, -.2, 0.);
    v4 qvec (q[1], q[2], q[3], 0.);
    test_v4_equal(qvec_cross(q, v), cross(qvec, v));
}

TEST(Quaternion, Rotation)
{
    quaternion rotq = quaternion_normalize(q);
    v4 v(-1.5, 1.6, -.2, 0.);
    test_v4_near(quaternion_rotate(rotq,v), quaternion_to_rotation_matrix(rotq)*v, 1.e-15);
}

TEST(Quaternion, ProductJacobian)
{
    quaternion q2(.1, -4.5, .0001, 1.493);
    {
        SCOPED_TRACE("left");
        test_v4_equal(quaternion_product(q, q2).data, quaternion_product_left_jacobian(q2) * v4(q.data));
        test_v4_equal(quaternion_product(q2, q).data, quaternion_product_left_jacobian(q) * v4(q2.data));
    }
    {
        SCOPED_TRACE("right");
        test_v4_equal(quaternion_product(q, q2).data, quaternion_product_right_jacobian(q) * v4(q2.data));
        test_v4_equal(quaternion_product(q2, q).data, quaternion_product_right_jacobian(q2) * v4(q.data));
    }
}

TEST(Quaternion, IntegrateAngularVelocity)
{
    v4 angvel(.13, -.012, .4, 0.);
    test_v4_equal(integrate_angular_velocity_quaternion(q, angvel).data, (q + .5 * quaternion_product(q, quaternion(0., angvel[0], angvel[1], angvel[2]))).data);
}