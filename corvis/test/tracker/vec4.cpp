#include <cmath>
#include "gtest/gtest.h"
#include "vec4.h"
#include "quaternion.h"
#include "util.h"
#include "rotation_vector.h"

TEST(Vector3, Cross) {
    v3 vec(1.5f, -.64f, 4.1f);
    v3 vec2(.08f, 1.2f, -.23f);
    EXPECT_V3_NEAR(vec.cross(vec2), skew(vec) * vec2, F_T_EPS) << "a x b = skew(a) * b";
    EXPECT_V3_NEAR(vec.cross(vec2), skew(vec2).transpose() * vec, F_T_EPS) << "a x b = skew(b)^T * a";
}

static void test_rotation(const v3 &vec)
{
    m3 skewmat = skew(vec);
    EXPECT_V3_NEAR(vec, invskew(skewmat), 0) << "invskew(skew(vec)) = vec";
    
    rotation_vector rvec(vec[0], vec[1], vec[2]);
    m3 rotmat = to_rotation_matrix(rvec);
    {
        SCOPED_TRACE("rotation_vector(vec).[xyz] = vec");
        EXPECT_DOUBLE_EQ(rvec.x(), vec[0]);
        EXPECT_DOUBLE_EQ(rvec.y(), vec[1]);
        EXPECT_DOUBLE_EQ(rvec.z(), vec[2]);
        EXPECT_V3_NEAR(rvec.raw_vector(), vec, 0);
    }
    
    EXPECT_ROTATION_VECTOR_NEAR(to_rotation_vector(to_quaternion(rvec)), rvec, 3*F_T_EPS) << "rot_vec(quaternion(vec)) = vec";
    EXPECT_ROTATION_VECTOR_NEAR(rvec, to_rotation_vector(rotmat), 3*F_T_EPS) << "vec = invrod(rod(vec))";

    EXPECT_M3_NEAR(m3::Identity(), rotmat.transpose() * rotmat, 3*F_T_EPS) << "R'R = I";
    EXPECT_V3_NEAR(vec, rotmat.transpose() * (rotmat * vec), 3*F_T_EPS) << "R'Rv = v";

    quaternion quat = to_quaternion(rvec);
    EXPECT_M3_NEAR(quat.toRotationMatrix(), to_rotation_matrix(rvec), 3*F_T_EPS) << "rot_mat(rotvec_to_quat(v)) = rodrigues(v)";
    quaternion qinv = to_quaternion(rotmat);
    EXPECT_QUATERNION_ROTATION_NEAR(quat, qinv, 2*F_T_EPS) << "q = to_quaternion(to_rotation_matrix(q))";

    {
        const v3 lvec(1.5f, -.64f, 4.1f);
        v3 vrot = quat * lvec;
        EXPECT_V3_NEAR(vrot, rotation_between_two_vectors(lvec, vrot) * lvec, 16*F_T_EPS)
            << "q * vec = rotation_between_two_vectors(vec, quaternion_rotation(q, vec)) * vec";
    }
    
    {
        const v3 up(0, 0, 1);
        EXPECT_V3_NEAR(up, rotation_between_two_vectors(quat * up, up) * (quat * up) , 3*F_T_EPS);
    }
    
}


TEST(Matrix3, Rotation) {
    v3 rotvec(.55f, -1.2f, -.15f);
    
    {
        SCOPED_TRACE("identity matrix = 0 rotation vector");
        EXPECT_ROTATION_VECTOR_NEAR(to_rotation_vector(m3::Identity()), rotation_vector(0., 0., 0.), 0);
    }
    
    {
        SCOPED_TRACE("generic vector");
        test_rotation(rotvec);
        test_rotation(rotvec*2.);
    }
    {
        SCOPED_TRACE("zero vector");
        test_rotation(v3(v3::Zero()));
    }
    {
        SCOPED_TRACE("+pi vector");
        test_rotation(v3(0, f_t(M_PI), 0));
        test_rotation(v3(0, f_t(M_PI-.1), 0));
    }
    {
        SCOPED_TRACE("-pi vector");
        test_rotation(v3(0, 0, f_t(-M_PI)));
    }
    {
        SCOPED_TRACE("pi-offaxis vector");
        test_rotation(rotvec.normalized() * (M_PI));
    }
}

TEST(Quaternion,Log) {
    for (f_t i=-100*F_T_EPS; i <100*F_T_EPS; i += F_T_EPS)
        for (f_t j=0; j<2*M_PI; j+=M_PI/16)
            EXPECT_ROTATION_VECTOR_NEAR(to_rotation_vector(quaternion(cos(j+i), sin(j+i),0,0)), rotation_vector(2*(j+i),0,0), 6*F_T_EPS);
}

TEST(Quaternion,LogNormalization) {
    for (f_t i=-100*F_T_EPS; i <100*F_T_EPS; i += F_T_EPS)
        for (f_t j=0; j<2*M_PI; j+=M_PI/16)
            EXPECT_LE(to_rotation_vector(quaternion(cos(j+i), sin(j+i),0,0)).raw_vector().norm(), (f_t)M_PI);
}
