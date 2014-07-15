#include "gtest/gtest.h"
#include "stereo.h"

const static m4 result = { {
    v4(4.7050659665283991e-06, -3.608920748657601e-04, 0.034825532535250781, 0),
    v4(3.6443240696172532e-04, 3.3651208688602836e-06, -0.093723138797686997, 0),
    v4(-0.042549276793071709, 0.09930259761635167, 0.98910642230355439, 0),
    v4(0, 0, 0, 1)
}};

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

void test_stereo_m4_equal(const m4 &a, const m4 &b)
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

TEST(Stereo, EightPointF) {
    m4 F = eight_point_F(p1, p2, npts);
    test_stereo_m4_equal(F, result);
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
