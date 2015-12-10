#include "gtest/gtest.h"
#include "vec4.h"
#include "quaternion.h"
#include "util.h"
#include "rotation_vector.h"

const static m4 foo = {
    {12.3, 0., -5., .001},
    {1., 1., 1., 1.},
    {-69., 1.e-4, 12.3e5, -1.11},
    {.2, -8.8, -22.2, 0.01}
};

const static m4 bar = {
    {123, 1., 5., -1.},
    {.1, .88, -1123.1, 1.123},
    {-69., 1.e4, -3.e5, -1.11},
    {-3.4, .8, .2, 0.}
};

TEST(Matrix4, Determinant) {
    m4 a = { {5., -2., 1., 0.}, {0., 3., -1., 0.}, {2., 0., 7., 0.}, {0., 0., 0., 0.} };
    EXPECT_NEAR(determinant3(a), 103, F_T_EPS);
    
    m4 b = { {1, 2, 3, 0}, {0, -4, 1, 0}, {0, 3, -1, 0}, {0, 0, 0 ,0} };
    EXPECT_NEAR(determinant3(b), 1, F_T_EPS);

    EXPECT_NEAR(determinant3(foo), 15128654.998270018, 1e-7);
    EXPECT_NEAR(determinant3(bar), 1349053797.5000024, 1e-5);
}

TEST(Vector4, Cross) {
    v4 vec(1.5, -.64, 4.1, 0.);
    v4 vec2(.08, 1.2, -.23, 0.);
    EXPECT_V4_NEAR(cross(vec, vec2), skew3(vec) * vec2, 0) << "a x b = skew(a) * b";
    EXPECT_V4_NEAR(cross(vec, vec2), skew3(vec2).transpose() * vec, 0) << "a x b = skew(b)^T * a";
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
    
    EXPECT_ROTATION_VECTOR_NEAR(to_rotation_vector(to_quaternion(rvec)), rvec, F_T_EPS) << "rot_vec(quaternion(vec)) = vec";
    EXPECT_ROTATION_VECTOR_NEAR(rvec, to_rotation_vector(rotmat), 4*F_T_EPS) << "vec = invrod(rod(vec))";

    EXPECT_M4_NEAR(m4::Identity(), rotmat.transpose() * rotmat, 1.e-15) << "R'R = I";
    EXPECT_V4_NEAR(vec, rotmat.transpose() * (rotmat * vec), 1.e-15) << "R'Rv = v";

    quaternion quat = to_quaternion(rvec);
    EXPECT_M4_NEAR(to_rotation_matrix(quat), to_rotation_matrix(rvec), 1.e-15) << "rot_mat(rotvec_to_quat(v)) = rodrigues(v)";
    quaternion qinv = to_quaternion(rotmat);
    EXPECT_QUATERNION_ROTATION_NEAR(quat, qinv, 1.e-15) << "q = to_quaternion(to_rotation_matrix(q))";

    {
        const v4 lvec(1.5, -.64, 4.1, 0.);
        v4 vrot = quat * lvec;
        EXPECT_V4_NEAR(vrot, rotation_between_two_vectors(lvec, vrot) * lvec, 1.e-12)
            << "q * vec = rotation_between_two_vectors(vec, quaternion_rotation(q, vec)) * vec";
    }
    
    {
        const v4 up(0., 0., 1., 0.);
        EXPECT_V4_NEAR(up, rotation_between_two_vectors(quat * up, up) * (quat * up) , 1.e-12);
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
