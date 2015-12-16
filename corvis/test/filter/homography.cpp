#include "gtest/gtest.h"
#include "homography.h"
#include "rotation_vector.h"
#include "util.h"
#include "matrix.h"

void fill_qr_ideal(v4 ideal[4], float qr_size, bool use_markers, int modules)
{
    if(use_markers) {
        // Ideal points are the QR code viewed from the bottom with
        // +z going out of the code toward the viewer
        // libzxing detects the center of the three markers and the center of the
        // lower right marker (LR) which is closer to the center of the code than
        // the rest
        float offset = 0.5;
        ideal[0] = v4(3.5,   3.5, 1, 0);                   // UL
        ideal[1] = v4(3.5, modules - 3.5, 1, 0);           // LL
        ideal[2] = v4(modules - 6.5, modules - 6.5, 1, 0); // LR
        ideal[3] = v4(modules - 3.5,  3.5, 1, 0);          // UR
        for(int i = 0; i < 4; i++) {
            ideal[i][0] = (ideal[i][0]/modules - offset)*qr_size;
            ideal[i][1] = (ideal[i][1]/modules - offset)*qr_size;
        }
    }
    else {
        ideal[0] = v4(-0.5*qr_size, -0.5*qr_size, 1, 0); // UL
        ideal[1] = v4(-0.5*qr_size,  0.5*qr_size, 1, 0); // LL
        ideal[2] = v4( 0.5*qr_size,  0.5*qr_size, 1, 0); // LR
        ideal[3] = v4( 0.5*qr_size, -0.5*qr_size, 1, 0); // UR
    }
    /*
    for(int i = 0; i < 4; i++) {
        fprintf(stderr, "ideal%d = ", i); ideal[i].print(); fprintf(stderr, "\n");
    }
    */
}

void project_points(const v4 points[4], feature_t image[4])
{
    for(int i = 0; i < 4; i++) {
        image[i].x() = points[i][0] / points[i][2];
        image[i].y() = points[i][1] / points[i][2];
        //fprintf(stderr, "image %d (%f %f)\n", i, image[i].x(), image[i].y());
    }
}

void test_qr_with_parameters(const m4 & R, const v4 & T, float qr_size, bool use_markers)
{
    v4 qr[4];
    feature_t qr_image[4];
    int modules = 21;

    fill_qr_ideal(qr, qr_size, use_markers, modules);

    for(int i = 0; i < 4; i++)
        qr[i] = R*qr[i] + T;

    project_points(qr, qr_image);

    homography_decomposition result;
    bool success = homography_align_qr_ideal(qr_image, qr_size, use_markers, modules, result);

    EXPECT_EQ(success, true);

    EXPECT_M4_NEAR(result.R, R, 1.e-3);
    EXPECT_V4_NEAR(result.T, T, 1.e-3);
}

TEST(Homography, I)
{
    float qr_size = 0.15;
    bool use_markers = false;
    
    test_qr_with_parameters(m4::Identity(), v4(0,0,0,0), qr_size, use_markers);
}

// This test case is degenerate with MaSKS homography decomposition
TEST(Homography, R)
{
    float qr_size = 0.38;
    bool use_markers = true;
    float theta = M_PI/2;
    m4 R = m4::Identity();
    v4 T = v4(0, 0, 0, 0);
    R(0, 0) = cos(theta); R(0, 1) = -sin(theta);
    R(1, 0) = sin(theta); R(1, 1) =  cos(theta);

    test_qr_with_parameters(R, T, qr_size, use_markers);
}

TEST(Homography, SimpleT)
{
    float qr_size = 0.15;
    bool use_markers = true;
    m4 R = m4::Identity();
    v4 T = v4(0.4, -0.33, -0.8, 0);

    test_qr_with_parameters(R, T, qr_size, use_markers);
}

TEST(Homography, SimpleRT)
{
    float qr_size = 0.15;
    bool use_markers = true;
    float theta = M_PI/3;
    m4 R = m4::Identity();
    v4 T = v4(0.1, 0.2, 0, 0);
    R(0, 0) = cos(theta); R(0, 1) = -sin(theta);
    R(1, 0) = sin(theta); R(1, 1) =  cos(theta);

    test_qr_with_parameters(R, T, qr_size, use_markers);
}

TEST(Homography, RTFull)
{
    float qr_size = 0.15;
    bool use_markers = true;
    const static rotation_vector W(-.13, 1.3, -.1);
    m4 R = to_rotation_matrix(W);
    v4 T = v4(.12, .79, 4., 0.);

    test_qr_with_parameters(R, T, qr_size, use_markers);
}

// The calculated homography needs its sign flipped or will not give a
// valid decomposition
TEST(Homography, HSignFlip)
{
    float qr_size = 0.15;
    bool use_markers = true;
    float theta = -M_PI/2;
    m4 R = m4::Identity();
    v4 T = v4(0, 0, -0.2, 0);
    R(0, 0) = cos(theta); R(0, 1) = -sin(theta);
    R(1, 0) = sin(theta); R(1, 1) =  cos(theta);

    test_qr_with_parameters(R, T, qr_size, use_markers);
}

// These real points also H to need a sign flip in order to succeed
// This test will fail with non-simple decomposition due to marker
// localization error
TEST(Homography, Real)
{
    // Rest = ~90 degrees ccw
    // Test = ~.3-.4m in Z
    float qr_size = 0.1825;
    int modules = 25;
    feature_t calibrated[4];
	calibrated[0] = feature_t { -0.351846f,  0.206401f };
	calibrated[1] = feature_t {  0.085983f,  0.207428f };
	calibrated[2] = feature_t {  0.016154f, -0.156903f };
	calibrated[3] = feature_t { -0.348012f, -0.233697f };

    const m4 Rexpected = {
        {7.240773e-03, -9.991911e-01, 2.042948e-02, 0.000000e+00},
        {-1.003615e+00, -6.556204e-03, -4.141437e-03, 0.000000e+00},
        {4.292099e-03, -2.032784e-02, -1.002851e+00, 0.000000e+00},
        {0.000000e+00, 0.000000e+00, 0.000000e+00, 1.000000e+00}
    };
    const v4 Texpected = v4(-3.920247e-02, -3.720683e-03, 3.009674e-01, 0.000000e+00);

    m4 R; v4 T;
    bool success = homography_align_to_qr(calibrated, qr_size, modules, R, T);

    EXPECT_EQ(success, true);
    EXPECT_M4_NEAR(R, Rexpected, 1e-3);
    EXPECT_V4_NEAR(T, Texpected, 1e-3);
}

TEST(Homography, AlignToQR)
{
    v4 qr[4];
    v4 qr_expected[4];
    feature_t qr_image[4];
    float qr_size = 0.50;
    // use_markers must be true for homography_align_to_qr
    bool use_markers = true; 
    int modules = 25;

    fill_qr_ideal(qr, qr_size, use_markers, modules);

    // Expected points have +z pointing out of the qr code
    // Since ideal points are 1m away, there is also a composited
    // translation
    m4 Rq = m4::Identity(); Rq(1, 1) = -1; Rq(2, 2) = -1;
    v4 Tq = v4(0, 0, 1, 0);
    for(int i = 0; i < 4; i++) {
        qr_expected[i] = Rq*qr[i] + Tq;
    }

    const static rotation_vector W(-.13, 1.3, -.1);
    m4 R = to_rotation_matrix(W);
    v4 T = v4(.12, .79, 4., 0.);
    for(int i = 0; i < 4; i++)
        qr[i] = R*qr[i] + T;

    project_points(qr, qr_image);

    m4 Raligned; v4 Taligned;
    bool success = homography_align_to_qr(qr_image, qr_size, modules, Raligned, Taligned);
    EXPECT_EQ(success, true);

    m4 Ri = Raligned.transpose();
    v4 Ti = -Raligned.transpose()*Taligned;
    for(int i = 0; i < 4; i++) {
        EXPECT_V4_NEAR(Ri*qr[i] + Ti, qr_expected[i], 1e-3);
    }
}

void homography_factorize(const m4 &H, m4 Rs[4], v4 Ts[4], v4 Ns[4]);

TEST(Homography, Factorize)
{
    m4 H;
    m4 Rs[4];
    v4 Ts[4];
    v4 Ns[4];
    // From MaSKS pg 138 example 5.20
    H.row(0) = 0.25*v4(5.404, 0, 4.436, 0);
    H.row(1) = 0.25*v4(0, 4, 0, 0);
    H.row(2) = 0.25*v4(-1.236, 0, 3.804, 0);
    H.row(3) = 0.25*v4(0, 0, 0, 1);

    homography_factorize(H, Rs, Ts, Ns);

    m4 Re[4];
    Re[0] = {{ {0.704, 0, 0.710, 0}, {0, 1, 0, 0}, {-0.710, 0, 0.704, 0}, {0, 0, 0, 1} }};
    Re[1] = {{ {0.951, 0, 0.309, 0}, {0, 1, 0, 0}, {-0.309, 0, 0.951, 0}, {0, 0, 0, 1} }};
    Re[2] = {{ {0.704, 0, 0.710, 0}, {0, 1, 0, 0}, {-0.710, 0, 0.704, 0}, {0, 0, 0, 1} }};
    Re[3] = {{ {0.951, 0, 0.309, 0}, {0, 1, 0, 0}, {-0.309, 0, 0.951, 0}, {0, 0, 0, 1} }};

    v4 Te[4];
    Te[0] = v4( 0.760, 0,  0.471, 0);
    Te[1] = v4(-0.894, 0,      0, 0);
    Te[2] = v4(-0.760, 0, -0.471, 0);
    Te[3] = v4( 0.894, 0,      0, 0);

    v4 Ne[4];
    Ne[0] = v4( 0.851, 0,  0.525, 0);
    Ne[1] = v4(-0.447, 0, -0.894, 0);
    Ne[2] = v4(-0.851, 0, -0.525, 0);
    Ne[3] = v4( 0.447, 0,  0.894, 0);

    for(int i = 0; i < 4; i++) {
        EXPECT_M4_NEAR(Rs[i], Re[i], 1e-3);
        EXPECT_V4_NEAR(Ts[i], Te[i], 1e-3);
        EXPECT_V4_NEAR(Ns[i], Ne[i], 1e-3);
    }

}

TEST(Homography, Decomposition)
{

    m4 He = {
        {1.134393e+00, -4.521915e-16, 6.756220e-01, 0.000000e+00},
        {-1.503590e-15, 1.000000e+00, 2.147986e-16, 0.000000e+00},
        {-3.089999e-01, 1.068114e-16, 9.510000e-01, 0.000000e+00},
        {0.000000e+00, 0.000000e+00, 0.000000e+00, 1.000000e+00}
    };
    m4 R = { {0.951, 0, 0.309, 0}, {0, 1, 0, 0}, {-0.309, 0, 0.951, 0}, {0, 0, 0, 1} };
    v4 T = v4( 2.05, 0,      0, 0);
    v4 N = v4( 0.4473, 0,  0.8942, 0);
    float d = 5;
    
    // four points in front of the image plane of both cameras, on
    // plane specified by N and d above
    v4 P1[4] = {v4(-1, 0, (d - (-1)*N[0])/N[2], 0),
                v4(-1, 3, (d - (-1)*N[0])/N[2], 0),
                v4((d - 5*N[2])/N[0], 3, 5, 0),
                v4((d - 5*N[2])/N[0], 0, 5, 0)};
    v4 P2[4];
    for(int i = 0; i < 4; i++) {
        P2[i] = R*P1[i] + T;
    }

    feature_t p1[4], p2[4];
    project_points(P1, p1);
    project_points(P2, p2);

    m4 H = homography_compute(p1, p2);
    EXPECT_M4_NEAR(H, He, 1e-3);
    vector<homography_decomposition> decompositions = homography_decompose(p1, p2, H);
    bool found = false;
    float margin2 = 1e-3 * 1e-3;
    // There are 2 physically possible solutions, check that it
    // contains the correct one.
    for(int i = 0; i < decompositions.size(); i++) {
        homography_decomposition result = decompositions[i];
        float dT2 = (result.T*d - T).dot(result.T*d - T);
        float dN2 = (result.N - N).dot(result.N - N);
        if(dN2 < margin2 && dT2 < margin2) {
            EXPECT_M4_NEAR(result.R, R, 1e-3);
            found = true;
        }

    }
    EXPECT_EQ(found, true);
}

