#include "gtest/gtest.h"
#include "util.h"
#include "matrix.h"

static f_t chol[] = {  4,  12, -16, 0,
                      12,  37, -43, 0,
                     -16, -43,  98, 0,
                       0,   0,   0, 0};

TEST(Matrix, PositiveDefinite)
{
    matrix A(chol, 3, 3, 4, 4);

    EXPECT_TRUE(test_posdef(A));

    A(1,2) = -44; // not symmetric
    EXPECT_FALSE(test_posdef(A));
    A(1,2) = -43;

    A.resize(4,4); // pos semi-definite
    EXPECT_FALSE(test_posdef(A));
}
