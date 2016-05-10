#include "gtest/gtest.h"
#include "vec4.h"
#include "quaternion.h"
#include "util.h"
#include "rotation_vector.h"

TEST(Vector4, Cross) {
    v4 vec(1.5, -.64, 4.1, 0.);
    v4 vec2(.08, 1.2, -.23, 0.);
    EXPECT_V4_NEAR(cross(vec, vec2), skew3(vec) * vec2, F_T_EPS) << "a x b = skew(a) * b";
    EXPECT_V4_NEAR(cross(vec, vec2), skew3(vec2).transpose() * vec, F_T_EPS) << "a x b = skew(b)^T * a";
}

static void test_rotation(const v4 &vec)
{
    m4 skewmat = skew3(vec);
    EXPECT_V4_NEAR(vec, invskew3(skewmat), 0) << "invskew(skew(vec)) = vec";
    
    rotation_vector rvec(vec[0], vec[1], vec[2]);
    m4 rotmat = to_rotation_matrix(rvec);
    {
        SCOPED_TRACE("rotation_vector(vec).[xyz] = vec");
        EXPECT_DOUBLE_EQ(rvec.x(), vec[0]);
        EXPECT_DOUBLE_EQ(rvec.y(), vec[1]);
        EXPECT_DOUBLE_EQ(rvec.z(), vec[2]);
        EXPECT_V4_NEAR(rvec.raw_vector(), vec, 0);
    }
    
    EXPECT_ROTATION_VECTOR_NEAR(to_rotation_vector(to_quaternion(rvec)), rvec, 4*F_T_EPS) << "rot_vec(quaternion(vec)) = vec";
    EXPECT_ROTATION_VECTOR_NEAR(rvec, to_rotation_vector(rotmat), 4*F_T_EPS) << "vec = invrod(rod(vec))";

    EXPECT_M4_NEAR(m4::Identity(), rotmat.transpose() * rotmat, 4*F_T_EPS) << "R'R = I";
    EXPECT_V4_NEAR(vec, rotmat.transpose() * (rotmat * vec), 4*F_T_EPS) << "R'Rv = v";

    quaternion quat = to_quaternion(rvec);
    EXPECT_M4_NEAR(to_rotation_matrix(quat), to_rotation_matrix(rvec), 4*F_T_EPS) << "rot_mat(rotvec_to_quat(v)) = rodrigues(v)";
    quaternion qinv = to_quaternion(rotmat);
    EXPECT_QUATERNION_ROTATION_NEAR(quat, qinv, 2*F_T_EPS) << "q = to_quaternion(to_rotation_matrix(q))";

    {
        const v4 lvec(1.5, -.64, 4.1, 0.);
        v4 vrot = quat * lvec;
        EXPECT_V4_NEAR(vrot, rotation_between_two_vectors(lvec, vrot) * lvec, 20*F_T_EPS)
            << "q * vec = rotation_between_two_vectors(vec, quaternion_rotation(q, vec)) * vec";
    }
    
    {
        const v4 up(0., 0., 1., 0.);
        EXPECT_V4_NEAR(up, rotation_between_two_vectors(quat * up, up) * (quat * up) , 8*F_T_EPS);
    }
    
}


TEST(Matrix4, Rotation) {
    v4 rotvec(.55, -1.2, -.15, 0.);
    
    {
        SCOPED_TRACE("identity matrix = 0 rotation vector");
        EXPECT_ROTATION_VECTOR_NEAR(to_rotation_vector(m4::Identity()), rotation_vector(0., 0., 0.), 0);
    }
    
    {
        SCOPED_TRACE("generic vector");
        test_rotation(rotvec);
        test_rotation(rotvec*2.);
    }
    {
        SCOPED_TRACE("zero vector");
        test_rotation(v4(v4::Zero()));
    }
    {
        SCOPED_TRACE("+pi vector");
        test_rotation(v4(0., M_PI, 0., 0.));
        test_rotation(v4(0., M_PI-.1, 0., 0.));
    }
    {
        SCOPED_TRACE("-pi vector");
        test_rotation(v4(0., 0., -M_PI, 0.));
    }
    {
        SCOPED_TRACE("pi-offaxis vector");
        test_rotation(rotvec.normalized() * (M_PI));
    }
}
