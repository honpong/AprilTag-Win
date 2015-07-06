#include "gtest/gtest.h"
#include "stereo.h"
#include "fundamental.h"
#include "util.h"

const static m4 result = {
    {4.7050659665283991e-06, -3.608920748657601e-04, 0.034825532535250781, 0},
    {3.6443240696172532e-04, 3.3651208688602836e-06, -0.093723138797686997, 0},
    {-0.042549276793071709, 0.09930259761635167, 0.98910642230355439, 0},
    {0, 0, 0, 1}
};

const static int npts = 10;
static v4 p1[] = {
    v4(93.4400,    28.4400,0,0),
    v4(8.4400,    215.4400,0,0),
    v4(128.5600,  112.4400,0,0),
    v4(129.5600,  183.4400,0,0),
    v4(183.5600,   69.5600,0,0),
    v4(222.4400,   69.4400,0,0),
    v4(223.0600,  120.0600,0,0),
    v4(336.1900,  270.0600,0,0),
    v4(278.9400,  126.0600,0,0),
    v4(299.4400,   11.5600,0,0)
};

static v4 p2[] = {
    v4(115.9700,   45.9400,0,0),
    v4(32.7300,   229.3100,0,0),
    v4(149.7700,  127.1900,0,0),
    v4(150.5500,  197.4400,0,0),
    v4(203.9900,   84.4400,0,0),
    v4(243.1800,   84.4400,0,0),
    v4(241.7200,  134.5600,0,0),
    v4(361.1100,  287.6900,0,0),
    v4(300.3200,  140.1900,0,0),
    v4(318.3000,   25.8100,0,0)
};

TEST(Stereo, EightPointF) {
    m4 F = eight_point_F(p1, p2, npts);
    EXPECT_M4_NEAR(F, result, 1.e-13);
}

TEST(Stereo, LineEndpoints) {
    v4 line = v4(2, 1, -300, 0);
    float endpoints[4];
    line_endpoints(line, 256, 256, endpoints);

    EXPECT_FLOAT_EQ(endpoints[0], 150);
    EXPECT_FLOAT_EQ(endpoints[1], 0);
    EXPECT_FLOAT_EQ(endpoints[2], 22.5);
    EXPECT_FLOAT_EQ(endpoints[3], 255);

    line_endpoints(line, 640, 480, endpoints);

    EXPECT_FLOAT_EQ(endpoints[0], 0);
    EXPECT_FLOAT_EQ(endpoints[1], 300);
    EXPECT_FLOAT_EQ(endpoints[2], 150);
    EXPECT_FLOAT_EQ(endpoints[3], 0);

}

TEST(Stereo, DecomposeF) {

    const m4 F = {
        {1.78401e-06,   1.80498e-05,  0.00438624,  0},
        {-1.58894e-05,  1.76122e-06,  0.00939954,  0},
        {-0.00715472,  -0.0100469,    0.99987,     0},
        {0, 0, 0, 1}
    };

    float focal_length = 525.851;
    float center_x = 326.299;
    float center_y = 248.752;
    const v4 p1 = v4(464.379425, 51.303638, 1.000000, 0.000000);
    const v4 p2 = v4(432.259125, 85.563332, 1.000000, 0.000000);

    const v4 T_gt = v4(-0.30957282,0.73110586,-0.60798758,0);
    const m4 R_gt = {
        {0.99746948, -0.060900524, -0.03668436, 0},
        {0.057197798, 0.99386221,  -0.09469077, 0},
        {0.042225916, 0.092352889, 0.99483061, 0},
        {0, 0, 0, 1}
    };
    const m4 R2_gt = R_gt.transpose();
    const v4 T2_gt = -R_gt.transpose()*T_gt;

    m4 R, R2;
    v4 T, T2;

    // R and T are from p1 to p2
    decompose_F(F, focal_length, center_x, center_y, p1, p2, R, T);

    // Check the opposite direction
    decompose_F(F.transpose(), focal_length, center_x, center_y, p2, p1, R2, T2);

    for(int i = 0; i < 4; i++)
        EXPECT_FLOAT_EQ(T_gt[i], T[i]);

    for(int r = 0; r < 4; r++)
        for(int c = 0; c < 4; c++)
            EXPECT_FLOAT_EQ(R_gt(r, c), R(r, c));

    for(int i = 0; i < 4; i++)
        EXPECT_FLOAT_EQ(T2_gt[i], T2[i]);

    for(int r = 0; r < 4; r++)
        for(int c = 0; c < 4; c++)
            EXPECT_FLOAT_EQ(R2_gt(r, c), R2(r, c));

}
