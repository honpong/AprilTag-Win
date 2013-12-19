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