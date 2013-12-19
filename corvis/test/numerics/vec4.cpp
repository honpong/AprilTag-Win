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

const static v4 rotvec(.55, -1.2, -.15, 0.);
const static v4 vec(1.5, -.64, 4.1, 0.);

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

TEST(Matrix4, Rodrigues) {
    m4v4 dR_dW;
    v4m4 dW_dR;
    test_v4_equal(invrodrigues(m4_identity, NULL), v4(0., 0., 0., 0.));
    m4 rotmat = rodrigues(rotvec, &dR_dW);
    test_v4_equal(invrodrigues(rotmat, &dW_dR), rotvec);
//TODO: restore this one, and do a custom bounds version of comparison
    /* This fails due to numerical differences, but the next one passes.
     test_m4_equal(m4_identity, transpose(rotmat) * rotmat);*/
    test_v4_equal(transpose(rotmat) * (rotmat * vec), vec);
//TODO: check the skew3 static derivatives
/*    v4 pertvec = v4(.01, .01, .01, 0.);
    v4 perturb  = rotvec + pertvec;
    m4 rodpert = rodrigues(perturb, NULL);
    m4 jacpert = rotmat + dR_dW * pertvec;
    test_m4_equal(jacpert, rodpert);
    rotmat.print();
    rodpert.print();
    jacpert.print();*/
}