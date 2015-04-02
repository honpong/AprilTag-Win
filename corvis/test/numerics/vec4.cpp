#include "gtest/gtest.h"
#include "vec4.h"
#include "quaternion.h"
#include "util.h"
#include "rotation_vector.h"

const static m4 foo = {{
    {12.3, 0., -5., .001},
    {1., 1., 1., 1.},
    {-69., 1.e-4, 12.3e5, -1.11},
    {.2, -8.8, -22.2, 0.01}
}};

const static m4 bar = {{
    {123, 1., 5., -1.},
    {.1, .88, -1123.1, 1.123},
    {-69., 1.e4, -3.e5, -1.11},
    {-3.4, .8, .2, 0.}
}};

const static m4 symmetric = foo * foo.transpose();

const static v4 vec(1.5, -.64, 4.1, 0.);

const static v4 v4_delta = v4(.01, -.01, .01, 0.);

const static m4 m4_delta = {{
    {0., .01, -.01, 0},
    {-.01, -.01, .01, 0},
    {-.01, 0., -.01, 0},
    {0, 0, 0, 0}
}};

TEST(Vector4, Equality)
{
    v4 v1 = vec, v2 = v1;
    EXPECT_EQ(v1, v2);
    for(int i = 0; i < 4; ++i) {
        v2[i]++;
        EXPECT_FALSE(v1 == v2);
        v2 = v1;
    }
}

TEST(Matrix4, Equality)
{
    m4 m1 = foo, m2 = m1;
    EXPECT_EQ(m1, m2);
    for(int i = 0; i < 4; ++i) {
        for(int j = 0; j < 4; ++j) {
            m2(i, j)++;
            EXPECT_FALSE(m1 == m2);
            m2 = m1;
        }
    }
}

TEST(Matrix4, Identity) {
    EXPECT_EQ(foo, foo);
    EXPECT_EQ(foo, foo * m4::Identity());
    EXPECT_EQ(foo, m4::Identity() * foo);
    EXPECT_EQ(foo + bar, bar + foo);
    EXPECT_EQ(foo + m4(), foo);
    EXPECT_EQ((m4)(foo - foo), (m4)m4::Zero());
    EXPECT_EQ((foo + bar) + m4::Identity(), foo + (bar + m4::Identity()));
    EXPECT_EQ(m4::Identity() * m4::Identity(), m4::Identity());
    EXPECT_EQ(foo.transpose().transpose(), foo);
    EXPECT_EQ((foo * bar).transpose(), bar.transpose() * foo.transpose());
    EXPECT_EQ(symmetric.transpose(), symmetric);
}

TEST(Matrix4, Determinant) {
    m4 a = {{ {5., -2., 1., 0.}, {0., 3., -1., 0.}, {2., 0., 7., 0.}, {0., 0., 0., 0.} }};
    EXPECT_FLOAT_EQ(determinant3(a), 103);
    
    m4 b = {{ {1, 2, 3, 0}, {0, -4, 1, 0}, {0, 3, -1, 0}, {0, 0, 0 ,0} }};
    EXPECT_FLOAT_EQ(determinant3(b), 1);

    EXPECT_FLOAT_EQ(determinant3(foo), 15128654.998270018);
    EXPECT_FLOAT_EQ(determinant3(bar), 1349053797.5000024);
}

TEST(Vector4, Cross) {
    v4 vec2(.08, 1.2, -.23, 0.);
    {
        SCOPED_TRACE("a x b = skew(a) * b");
        EXPECT_EQ(cross(vec, vec2), skew3(vec) * vec2);
    }
    {
        SCOPED_TRACE("a x b = skew(b)^T * a");
        EXPECT_EQ(cross(vec, vec2), skew3(vec2).transpose() * vec);
    }
}

bool same_sign(f_t first, f_t second)
{
    return first * second >= 0. || fabs(first) < 1.e-5;
}

//max error on an individual level is too sensitive to small perturbations that don't matter
/*
 f_t thresh = .1 * fabs(delta[j]);
 if(thresh < 1.e-5) thresh = 1.e-5;
 EXPECT_NEAR(delta[j], lindelta[j], thresh) << "where i is " << i << " and j is " << j;
 f_t err = fabs(delta[j] - lindelta[j]);
 if(fabs(delta[j]) > 1.e-5) err /= fabs(delta[j]);
 if(err > max_err) max_err = err;
*/

f_t test_m4_linearization(const v4 &base, v4 (*nonlinear)(const v4 &base, const void *other), const m4 &jacobian, void *other)
{
    const f_t eps = .1;
    f_t max_err = 0.;
    for(int i = 0; i < 3; ++i) {
        v4 pert(v4::Zero());
        pert[i] = base[i] * eps + 1.e-5;
        v4 delta = nonlinear(base + pert, other) - nonlinear(base, other);
        v4 lindelta = jacobian * pert;
        
        for(int j = 0; j < 3; ++j) {
            EXPECT_PRED2(same_sign, delta[j], lindelta[j]) << "Sign flip, where i is " << i << " and j is " << j;
        }
        f_t vec_pct_err = (delta - lindelta).norm() / delta.norm();
        EXPECT_LT(vec_pct_err, .10);
        if(vec_pct_err > max_err) max_err = vec_pct_err;
    }
    return max_err;
}

v4 iavq_angle_stub(const v4 &base, const void *other)
{
    quaternion q(base[0], base[1], base[2], base[3]);
    quaternion res = integrate_angular_velocity(q, *(v4 *)other);
    return v4(res.w(), res.x(), res.y(), res.z());
}

v4 iavq_vel_stub(const v4 &base, const void *other)
{
    v4 b = *(v4 *)other;
    quaternion q(b[0], b[1], b[2], b[3]);
    quaternion res = integrate_angular_velocity(q, base);
    return v4(res.w(), res.x(), res.y(), res.z());
}


v4 iav_angle_stub(const v4 &base, const void *other)
{
    return integrate_angular_velocity(base, *(v4 *)other);
}

v4 iav_vel_stub(const v4 &base, const void *other)
{
    return integrate_angular_velocity(*(v4 *)other, base);
}

void test_rotation(const v4 &vec)
{
    m4v4 dR_dW;
    v4m4 dW_dR;
    v4 pertvec  = vec + v4_delta;
    
    m4 skewmat = skew3(vec);
    {
        SCOPED_TRACE("invskew(skew(vec)) = vec");
        EXPECT_EQ(vec, invskew3(skewmat));
    }
    
    m4 skewpert = skew3(pertvec);
    m4 jacpert = skewmat + apply_jacobian_m4v4(skew3_jacobian, v4_delta);
    {
        SCOPED_TRACE("skew(vec + delta) = skew(vec) + jacobian * delta");
        EXPECT_EQ(skewpert, jacpert);
    }
    
    m4 pertmat = skewmat + m4_delta;
    {
        SCOPED_TRACE("invskew(mat + delta) = invskew(mat) + jacobian * delta");
        EXPECT_EQ(invskew3(pertmat), vec + apply_jacobian_v4m4(invskew3_jacobian, m4_delta));
    }
    
    rotation_vector rvec(vec[0], vec[1], vec[2]);
    m4 rotmat = to_rotation_matrix(rvec);
    dR_dW = to_rotation_matrix_jacobian(rvec);
    {
        SCOPED_TRACE("rotation_vector(vec).[xyz] = vec");
        EXPECT_EQ(rvec.x(), vec[0]);
        EXPECT_EQ(rvec.y(), vec[1]);
        EXPECT_EQ(rvec.z(), vec[2]);
        EXPECT_EQ(rvec.raw_vector(), vec);
    }
    
    {
        SCOPED_TRACE("rodrigues = to_rotation_matrix");
        m4v4 rodjac;
        test_m4_near(rodrigues(vec, &rodjac), rotmat, F_T_EPS);
        EXPECT_EQ(rodjac, dR_dW);
    }
    
    {
        SCOPED_TRACE("rot_vec(quaternion(vec)) = vec");
        test_rotation_vector_near(to_rotation_vector(to_quaternion(rvec)), rvec, F_T_EPS);
    }
    
    {
        rotation_vector inv = to_rotation_vector(rotmat);
        SCOPED_TRACE("vec = invrod(rod(vec))");
        test_rotation_vector_near(rvec, inv, 4*F_T_EPS);
    }
    
    //The next two fail the normal float comparison due to matrix product - (TODO: rearrange matrix operations?)
    {
        SCOPED_TRACE("R'R = I");
        test_m4_near(m4::Identity(), rotmat.transpose() * rotmat, 1.e-15);
    }
    
    {
        SCOPED_TRACE("R'Rv = v");
        test_v4_near(vec, rotmat.transpose() * (rotmat * vec), 1.e-15);
    }
    
    {
        SCOPED_TRACE("rodrigues(v + delta) ~= rodrigues(v) + jacobian * delta");
        m4 rodpert = to_rotation_matrix(rotation_vector(pertvec[0], pertvec[1], pertvec[2]));
        m4 jacpert = rotmat + apply_jacobian_m4v4(dR_dW, v4_delta);
        test_m4_near(jacpert, rodpert, .001);
    }
    
    v4 angvel(-.0514, .023, -.065, 0.);
    
    m4 dW_dW, dW_dw;
    linearize_angular_integration(vec, angvel, dW_dW, dW_dw);
    {
        SCOPED_TRACE("integrate_angular_velocity(W + delta, w) = iav(W, w) + jacobian * delta");
        f_t err = test_m4_linearization(vec, iav_angle_stub, dW_dW, &angvel);
        fprintf(stderr, "Angular velocity integration linearization max error (angle) is %.1f%%\n", err * 100);
    }
    {
        SCOPED_TRACE("integrate_angular_velocity(W, w + delta) = iav(W, w) + jacobian * delta");
        f_t err = test_m4_linearization(angvel, iav_vel_stub, dW_dw, (void *)&vec);
        fprintf(stderr, "Angular velocity integration linearization max error (velocity) is %.1f%%\n", err * 100);
    }
    {
        SCOPED_TRACE("integrate_angular_velocity(v4) = integrate_angular_velocity(rotvec)");
        EXPECT_EQ(integrate_angular_velocity(vec, angvel), integrate_angular_velocity(rvec, angvel).raw_vector());
    }

    quaternion quat = to_quaternion(rvec);
    {
        SCOPED_TRACE("rot_mat(rotvec_to_quat(v)) = rodrigues(v)");
        test_m4_near(to_rotation_matrix(quat), rodrigues(vec, NULL), 1.e-15);
    }
    rotmat = to_rotation_matrix(quat);
    {
        SCOPED_TRACE("quat_to_rotmat(v + delta) ~= quat_to_rotmat(v) + jacobian * delta");
        m4 qpert = to_rotation_matrix(quaternion(quat.w() + v4_delta[0], quat.x() + v4_delta[1], quat.y() + v4_delta[2], quat.z() + v4_delta[3]));
        m4 jacpert = rotmat + apply_jacobian_m4v4(to_rotation_matrix_jacobian(quat), v4_delta);
        test_m4_near(jacpert, qpert, .001);
    }
    quaternion qinv = to_quaternion(rotmat);
    {
        SCOPED_TRACE("q = to_quaternion(to_rotation_matrix(q))");
        test_quaternion_near_rotation(quat, qinv, 1.e-15);
    }

    {
        const v4 lvec(1.5, -.64, 4.1, 0.);
        v4 vrot = quaternion_rotate(quat, lvec);
        SCOPED_TRACE("q * vec = rotation_between_two_vectors(vec, quaternion_rotation(q, vec)) * vec");
        test_v4_near(vrot, quaternion_rotate(rotation_between_two_vectors(lvec, vrot), lvec), 1.e-12);
    }
    
    {
        const v4 up(0., 0., 1., 0.);
        v4 newup = quaternion_rotate(quat, up);
        SCOPED_TRACE("up = quaternion_rotate(rotation_between_two_vectors(quaternion_rotate(q, up), up), quaternion_rotate(q, up))");
        test_v4_near(up, quaternion_rotate(rotation_between_two_vectors(newup, up), newup) , 1.e-12);
    }
    
    integrate_angular_velocity_jacobian(quat, angvel, dW_dW, dW_dw);
    {
        SCOPED_TRACE("quaternion integrate_angular_velocity(W + delta, w) = iav(W, w) + jacobian * delta");
        f_t err = test_m4_linearization(v4(quat.w(), quat.x(), quat.y(), quat.z()), iavq_angle_stub, dW_dW, &angvel);
        fprintf(stderr, "Quaternion Angular velocity integration linearization max error (angle) is %.1f%%\n", err * 100);
    }
    {
        SCOPED_TRACE("quaternion integrate_angular_velocity(W, w + delta) = iav(W, w) + jacobian * delta");
        v4 vq = v4(quat.w(), quat.x(), quat.y(), quat.z());
        f_t err = test_m4_linearization(angvel, iavq_vel_stub, dW_dw, (void *)&vq);
        fprintf(stderr, "Quaternion Angular velocity integration linearization max error (velocity) is %.1f%%\n", err * 100);
    }

    {
        SCOPED_TRACE("integrate_angular_velocity(W, w) = W * integrate_angular_velocity(I, w)");
        quaternion q1 = integrate_angular_velocity(quat, angvel);
        quaternion q2 = quaternion_product(quat, integrate_angular_velocity(quaternion(1., 0., 0., 0.), angvel));
        v4 vq1(q1.w(), q1.x(), q1.y(), q1.z());
        v4 vq2(q2.w(), q2.x(), q2.y(), q2.z());
        test_v4_near(vq1, vq2, 1.e-15);
    }
}


TEST(Matrix4, Rotation) {
    v4 rotvec(.55, -1.2, -.15, 0.);
    
    {
        SCOPED_TRACE("identity matrix = 0 rotation vector");
        EXPECT_EQ(rodrigues(v4(v4::Zero()), NULL), (m4)m4::Identity());
        test_rotation_vector_near(to_rotation_vector(m4::Identity()), rotation_vector(0., 0., 0.), 0);
    }
    
    {
        SCOPED_TRACE("invskew * skew = I");
        m4 m3_identity = m4::Identity();
        m3_identity(3, 3) = 0.;
        EXPECT_EQ(m3_identity, invskew3_jacobian * skew3_jacobian);
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
