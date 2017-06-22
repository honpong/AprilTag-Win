#include "gtest/gtest.h"
#include "util.h"
#include "matrix.h"

static f_t A_padded[] = {0.00651856,0.0075159,7.65636e-06,0,
             0.0075159,0.0326057,-2.33408e-05,0,
             7.65636e-06,-2.33408e-05,0.0345768,0,
             0,0,0,1};

static f_t B_padded[] = {-7.97337e-06,-2.53351e-05,1.92629e-05,0,
             7.7408e-06,-3.65932e-06,-2.00035e-06,0,
             3.83459e-07,6.25474e-08,-2.42663e-06,0,
            -7.31693e-07,1.05141e-06,-0.00098288,0,

            -6.65329e-08,4.70554e-07,-0.000261044,0,
             0.000261848,0.000983777,2.05282e-07,0,
            -0.000137619,0.000285825,-0.356927,0,
            -2.44528e-05,5.87314e-05,-0.0951044,0
};


static f_t R_padded[] = {-0.00044727,-0.000673516,0.000556749,0,
                0.00179376,-0.000525748,-5.86045e-05,0,
                7.72983e-05,-1.59499e-05,-7.02086e-05,0,
                -0.000126089,4.09623e-05,-0.028426,0,

                -1.60001e-05,1.27154e-05,-0.00754967,0,
                0.00732931,0.0284825,2.3541e-05,0,
                -0.0144023,0.00469643,-10.3227,0,
                -0.000445738,-6.4957e-05,-2.75053,0
};

TEST(Matrix, SimplePadded)
{
    matrix A(A_padded, 4, 4, 4, 4);
    matrix B(B_padded, 8, 4, 8, 4);
    matrix R(R_padded, 8, 4, 8, 4);
    matrix_solve(A, B);

    for(int i = 0; i < 8; i++)
        for(int j = 0; j < 4; j++)
            if (std::fabs( B(i,j) - R(i,j) )          > 25*F_T_EPS &&
                std::fabs((B(i,j) - R(i,j)) / R(i,j)) > 25*F_T_EPS)
                EXPECT_NEAR(B(i,j), R(i,j),             25*F_T_EPS);
}

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
