#include "gtest/gtest.h"
#include "util.h"
#include "rotation_vector.h"

TEST(RotationVector, Exponential)
{
    rotation_vector w(.1,.2,.3);
    m4 R = {
            { (f_t)0.93575480327791880, -(f_t)0.28316496056507373, (f_t)0.21019170595074288, 0},
            { (f_t)0.30293271340263710,  (f_t)0.9505806179060914, -(f_t)0.06803131640494002, 0},
            {-(f_t)0.18054007669439776,  (f_t)0.12733457491763028, (f_t)0.97529030895304570, 0},
            {      0,                         0,                        0,                   1},
    };
    EXPECT_M4_NEAR(R, to_rotation_matrix(w), F_T_EPS);
    EXPECT_M4_NEAR(to_rotation_matrix(rotation_vector(0,0,0)), m4::Identity(), 0);
}

TEST(RotationVector, ExactJacobians)
{
    rotation_vector w(.1,.2,.3);
    m4 J = {
            { (f_t)0.97848449542621930,  (f_t)0.15156822390846117, -(f_t)0.093873647747713900, 0},
            {-(f_t)0.14494806865499016,  (f_t)0.98344961186632260,  (f_t)0.059349614974115006, 0},
            { (f_t)0.10380388062792034, -(f_t)0.03948914921370206,  (f_t)0.991724805933161300, 0},
            {      0,                         0,                         0,                    0},
    };
    EXPECT_M4_NEAR(J,    to_body_jacobian( w), F_T_EPS);
    EXPECT_M4_NEAR(J, to_spatial_jacobian(-w), F_T_EPS);
}

TEST(RotationVector, NumericalJacobians)
{
    for (f_t eps = 1; eps >= sqrt(F_T_EPS); eps /= 2) {
        rotation_vector w(.5,.2,-.7); v4 n(-.9,.3,-.003,0); // Random
        v4 dx = eps * n, wdx = w.raw_vector()+dx; rotation_vector w_dx(wdx[0], wdx[1], wdx[2]);
        m4 Rt_dR = to_rotation_matrix(-w) * ((to_rotation_matrix(w_dx) - to_rotation_matrix(w)) * (1 / eps));
        m4 dR_Rt = ((to_rotation_matrix(w_dx) - to_rotation_matrix(w)) * (1 / eps)) * to_rotation_matrix(-w);
        EXPECT_M4_NEAR(-Rt_dR.transpose(), Rt_dR, 2*eps);
        EXPECT_M4_NEAR(-dR_Rt.transpose(), dR_Rt, 2*eps);
        EXPECT_V4_NEAR(invskew3(Rt_dR),    to_body_jacobian(w) * n, eps);
        EXPECT_V4_NEAR(invskew3(dR_Rt), to_spatial_jacobian(w) * n, eps);
    }
}
