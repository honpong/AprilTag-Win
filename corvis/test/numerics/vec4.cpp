#include "gtest/gtest.h"
#include "vec4.h"

const static m4 foo = { {
    v4(12.3, 0., -5., .001),
    v4(1., 1., 1., 1.),
    v4(-69., 1.e-4, 12.3e5, -1.11),
    v4(.2, -8.8, -22.2, 0.01)
} };

const static m4 bar = { {
    v4(123, 1., 5., -1.),
    v4(.1, .88, -1123.1, 1.123),
    v4(-69., 1.e4, -3.e5, -1.11),
    v4(-3.4, .8, .2, 0.)
} };

const static m4 symmetric = foo * transpose(foo);

const static v4 vec(1.5, -.64, 4.1, 0.);

const static v4 v4_delta = v4(.01, -.01, .01, 0.);

const static m4 m4_delta = { {
    v4(0., .01, -.01, 0),
    v4(-.01, -.01, .01, 0),
    v4(-.01, 0., -.01, 0),
    v4(0, 0, 0, 0)
}};


void test_m4_equal(const m4 &a, const m4 &b)
{
    for(int i = 0; i < 4; ++i) {
        for(int j = 0; j < 4; ++j) {
#ifdef F_T_IS_DOUBLE
            EXPECT_DOUBLE_EQ(a[i][j], b[i][j]) << "Where i is " << i << " and j is " << j;
#else
            EXPECT_FLOAT_EQ(a[i][j], b[i][j]) << "Where i is " << i << " and j is " << j;

#endif
        }
    }
}

void test_m4_near(const m4 &a, const m4 &b, const f_t bounds)
{
    for(int i = 0; i < 4; ++i) {
        for(int j = 0; j < 4; ++j) {
#ifdef F_T_IS_DOUBLE
            EXPECT_NEAR(a[i][j], b[i][j], bounds) << "Where i is " << i << " and j is " << j;
#else
            EXPECT_NEAR(a[i][j], b[i][j], bounds) << "Where i is " << i << " and j is " << j;
            
#endif
        }
    }
}

void test_v4_equal(const v4 &a, const v4 &b)
{
    for(int i = 0; i < 4; ++i) {
#ifdef F_T_IS_DOUBLE
            EXPECT_DOUBLE_EQ(a[i], b[i]) << "Where i is " << i;
#else
            EXPECT_FLOAT_EQ(a[i], b[i]) << "Where i is " << i;
            
#endif
    }
}

void test_v4_near(const v4 &a, const v4 &b, const f_t bounds)
{
    for(int i = 0; i < 4; ++i) {
#ifdef F_T_IS_DOUBLE
        EXPECT_NEAR(a[i], b[i], bounds) << "Where i is " << i;
#else
        EXPECT_NEAR(a[i], b[i], bounds) << "Where i is " << i;
        
#endif
    }
}

TEST(Matrix4, Identity) {
    test_m4_equal(foo, foo);
    test_m4_equal(foo, foo * m4_identity);
    test_m4_equal(foo, m4_identity * foo);
    test_m4_equal(foo + bar, bar + foo);
    test_m4_equal(foo + m4(), foo);
    test_m4_equal(foo - foo, m4());
    test_m4_equal((foo + bar) + m4_identity, foo + (bar + m4_identity));
    test_m4_equal(m4_identity * m4_identity, m4_identity);
    test_m4_equal(transpose(transpose(foo)), foo);
    test_m4_equal(transpose(foo * bar), transpose(bar) * transpose(foo));
    test_m4_equal(transpose(symmetric), symmetric);
}

void test_rotation(const v4 &vec)
{
    m4v4 dR_dW;
    v4m4 dW_dR;
    v4 pertvec  = vec + v4_delta;

    m4 skewmat = skew3(vec);
    {
        SCOPED_TRACE("invskew(skew(vec)) = vec");
        test_v4_equal(vec, invskew3(skewmat));
    }
    
    m4 skewpert = skew3(pertvec);
    m4 jacpert = skewmat + apply_jacobian_m4v4(skew3_jacobian, v4_delta);
    {
        SCOPED_TRACE("skew(vec + delta) = skew(vec) + jacobian * delta");
        test_m4_equal(skewpert, jacpert);
    }
    
    m4 pertmat = skewmat + m4_delta;
    {
        SCOPED_TRACE("invskew(mat + delta) = invskew(mat) + jacobian * delta");
        test_v4_equal(invskew3(pertmat), vec + apply_jacobian_v4m4(invskew3_jacobian, m4_delta));
    }
    
    m4 rotmat = rodrigues(vec, &dR_dW);
    v4 inv = invrodrigues(rotmat, &dW_dR);
    //if it's close to pi, regularize it
    if(norm(vec) > M_PI - .001) {
        for(int i = 0; i < 3; ++i) {
            if(vec[i] * inv[i] < 0.) inv = -inv;
        }
    }
    
    {
        SCOPED_TRACE("vec = invrod(rod(vec))");
        test_v4_near(vec, inv, 1.e-7);
    }
    
    //The next two fail the normal float comparison due to matrix product - (TODO: rearrange matrix operations?)
    {
        SCOPED_TRACE("R'R = I");
        test_m4_near(m4_identity, transpose(rotmat) * rotmat, 1.e-15);
    }
    
    {
        SCOPED_TRACE("R'Rv = v");
        test_v4_near(vec, transpose(rotmat) * (rotmat * vec), 1.e-15);
    }
    
    {
        SCOPED_TRACE("rodrigues(v + delta) ~= rodrigues(v) + jacobian * delta");
        m4 rodpert = rodrigues(pertvec, NULL);
        m4 jacpert = rotmat + apply_jacobian_m4v4(dR_dW, v4_delta);
        test_m4_near(jacpert, rodpert, .001);
    }

    {
        SCOPED_TRACE("invrod(m + delta) ~= invrod(m) + jacobian * delta");
        v4 inv = invrodrigues(rotmat + m4_delta, NULL);
        v4 jacvec = vec + apply_jacobian_v4m4(dW_dR, m4_delta);
        f_t theta = norm(jacvec);
        if(theta > M_PI) {
            jacvec = -(jacvec / theta) * (2. * M_PI - theta);
        }
        test_v4_near(inv, jacvec, .0001);
        vec.print();
        inv.print();
        jacvec.print();
    }
}

TEST(Matrix4, Rodrigues) {
    v4 rotvec(.55, -1.2, -.15, 0.);

    {
        SCOPED_TRACE("identity matrix = 0 rotation vector");
        test_m4_equal(rodrigues(v4(0.), NULL), m4_identity);
        test_v4_equal(invrodrigues(m4_identity, NULL), v4(0., 0., 0., 0.));
    }
    
    {
        SCOPED_TRACE("invskew * skew = I");
        m4 m3_identity = m4_identity;
        m3_identity[3][3] = 0.;
        test_m4_equal(m3_identity, invskew3_jacobian * skew3_jacobian);
    }
    
    {
        SCOPED_TRACE("generic vector");
        test_rotation(rotvec);
    }
    {
        SCOPED_TRACE("zero vector");
        test_rotation(v4(0.));
    }
    {
        SCOPED_TRACE("+pi vector");
        test_rotation(v4(0., M_PI, 0., 0.));
    }
    {
        SCOPED_TRACE("-pi vector");
        test_rotation(v4(0., 0., -M_PI, 0.));
    }
    {
        SCOPED_TRACE("pi-offaxis vector");
        test_rotation(rotvec / norm(rotvec) * M_PI);
        //(rotvec / norm(rotvec) * M_PI).print();
        //fprintf(stderr, "norm is %f\n", norm(        (rotvec / norm(rotvec) * M_PI)));
    }
}

TEST(Vector4, Cross) {
    v4 vec2(.08, 1.2, -.23, 0.);
    {
        SCOPED_TRACE("a x b = skew(a) * b");
        test_v4_equal(cross(vec, vec2), skew3(vec) * vec2);
    }
    {
        SCOPED_TRACE("a x b = skew(b)^T * a");
        test_v4_equal(cross(vec, vec2), transpose(skew3(vec2)) * vec);
    }
}

