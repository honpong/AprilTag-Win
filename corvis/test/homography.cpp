#include "gtest/gtest.h"
#include "homography.h"
#include "rotation_vector.h"
#include "util.h"
#include "matrix.h"

const static rotation_vector W(-.13, 1.3, -.1);
const static v4 T(.12, .79, 4., 0.);

static void homography_set_correspondence_one_sided(matrix &X, int n, float x1, float y1, float z1, float x2, float y2);
static void homography_set_correspondence(matrix &X, int n, float x1, float y1, float x2, float y2);
m4 homography_estimate_from_constraints(const matrix &X);
void homography_factorize(const m4 &H, m4 Rs[4], v4 Ts[4], v4 Ns[4]);
bool homography_pick_solution(const m4 Rs[4], const v4 Ts[4], const v4 Ns[4], const feature_t p1[4], const feature_t p2[4], m4 &R, v4 &T);

TEST(Homography, Qr_one_sided)
{
    // transform our QR code away from the origin and compute an image
    // of it to use with compute_qr_homography
    v4 x[4];
    f_t width = 0.5;
    x[0] = v4(-.5 * width, .5 * width, 1., 1.);
    x[1] = v4(-.5 * width, -.5 * width, 1., 1.);
    x[2] = v4(.5 * width, -.5 * width, 1., 1.);
    x[3] = v4(.5 * width, .5 * width, 1., 1.);
    m4 R = to_rotation_matrix(W);
    v4 cam[4];
    feature_t image[4];
    for(int i = 0; i < 4; ++i)
    {
        cam[i] = R * x[i] + T;
        image[i].x = cam[i][0] / cam[i][2];
        image[i].y = cam[i][1] / cam[i][2];
        x[i].print(); cam[i].print(); fprintf(stderr, "%f %f\n", image[i].x, image[i].y);
    }
    R.print();
    T.print();
    fprintf(stderr,"\n");
    m4 Rres;
    v4 Tres;
    homography_compute_one_sided(x, image, Rres, Tres);
    
    test_m4_near(Rres, R, 1.e-6);
    test_v4_near(Tres, T, 1.e-6);
}

TEST(Homography, Qr)
{
    // transform our QR code away from the origin and compute an image
    // of it to use with compute_qr_homography
    v4 x[4];
    f_t width = 0.5;
    x[0] = v4(-.5 * width, .5 * width, 0., 1.);
    x[1] = v4(-.5 * width, -.5 * width, 0., 1.);
    x[2] = v4(.5 * width, -.5 * width, 0., 1.);
    x[3] = v4(.5 * width, .5 * width, 0., 1.);
    m4 R = to_rotation_matrix(W);
    v4 cam[4];
    feature_t image[4];
    for(int i = 0; i < 4; ++i)
    {
        cam[i] = R * x[i] + T;
        image[i].x = cam[i][0] / cam[i][2];
        image[i].y = cam[i][1] / cam[i][2];
        x[i].print(); cam[i].print(); fprintf(stderr, "%f %f\n", image[i].x, image[i].y);
    }
    R.print();
    T.print();
    fprintf(stderr,"\n");
    m4 Rres;
    v4 Tres;
    homography_solve_qr(image, width, Rres, Tres);

    test_m4_near(Rres, R, 1.e-6);
    test_v4_near(Tres, T, 1.e-6);
}

TEST(Homography, SyntheticImage)
{
    const float real_size = 0.05;
    const float f = 500;
    const float center_x = 640/2;
    const float center_y = 480/2;

    feature_t image[4];
    image[0] = (feature_t){.x = 10, .y = 10};
    image[1] = (feature_t){.x = 10, .y = 60};
    image[2] = (feature_t){.x = 60, .y = 60};
    image[3] = (feature_t){.x = 60, .y = 10};
    
    feature_t calibrated[4];
    for(int i = 0; i < 4; i++)
    {
        calibrated[i].x = (image[i].x - center_x)/f;
        calibrated[i].y = (image[i].y - center_y)/f;
    }

    feature_t qr_center = (feature_t){.x = image[0].x + (image[2].x - image[0].x)/2.,
                                      .y = image[0].y + (image[2].y - image[0].y)/2.};
    float scale_by = real_size / 0.1;
    m4 R = m4_identity;
    v4 T = scale_by * v4( (qr_center.x - center_x)/f, (qr_center.y - center_y)/f, -.5/scale_by, 0);
    fprintf(stderr, "T_expected = ");
    T.print();
    fprintf(stderr, "\n");

    m4 Rres;
    v4 Tres;
    homography_solve_qr(calibrated, real_size, Rres, Tres);
    Rres.print();
    fprintf(stderr, "\n");
    Tres.print();
    fprintf(stderr, "\n");
    test_m4_near(Rres, R, 1.e-6);
    test_v4_near(Tres, T, 1.e-6);
}

TEST(Homography, ImageReal)
{
    feature_t image[4];
    image[0] = (feature_t){.x = 291.030426, .y = 256.791859};
    image[1] = (feature_t){.x = 289.511223, .y = 318.284512};
    image[2] = (feature_t){.x = 349.993210, .y = 317.415562};
    image[3] = (feature_t){.x = 350.646400, .y = 258.538685};
    const float center_x = 311.292933;
    const float center_y = 247.884155;
    const float f = 535.735909;
    float real_size = 0.0735;

    feature_t calibrated[4];
    for(int i = 0; i < 4; i++)
    {
        calibrated[i].x = (image[i].x - center_x)/f;
        calibrated[i].y = (image[i].y - center_y)/f;
    }

    m4 Rres;
    v4 Tres;
    bool success = homography_solve_qr(calibrated, real_size, Rres, Tres);
    EXPECT_EQ(success, true);
}

/*
 2014-11-19 17:03:45.351 RC3DKSampleAR[1272:460158] Code detected Test QR code. Yay!!! at 147319326698, corners (325.768280, 163.891997), (245.812798, 216.301961), (292.727489, 300.962191), (376.211052, 248.810692)
 calibrated[0] = (feature_t){.x = 0.021575, .y = -0.141629};
 calibrated[1] = (feature_t){.x = -0.128005, .y = -0.043603};
 calibrated[2] = (feature_t){.x = -0.040246, .y = 0.114815};
 calibrated[3] = (feature_t){.x = 0.115991, .y = 0.017222};
 T,W coming from 3dk:
 [ -4.869276e-04, -1.388849e-02, 8.402545e-01, 0.000000e+00 ][-0.165943 0.852419 0.471536 -0.153291]
 origin location in camera frame:
 [ 1.002108e-01, 3.526036e-01, 7.562072e-01, 0.000000e+00 ]
 
 2014-11-19 17:03:45.399 RC3DKSampleAR[1272:460158] Code detected Test QR code. Yay!!! at 147319368318, corners (325.234489, 162.669396), (245.726395, 215.430551), (292.434769, 299.861641), (376.146088, 247.944660)
 calibrated[0] = (feature_t){.x = 0.020573, .y = -0.144037};
 calibrated[1] = (feature_t){.x = -0.128384, .y = -0.045216};
 calibrated[2] = (feature_t){.x = -0.040887, .y = 0.113005};
 calibrated[3] = (feature_t){.x = 0.116005, .y = 0.015706};
 T,W coming from 3dk:
 [ -6.088693e-03, -1.308967e-02, 8.149995e-01, 0.000000e+00 ][-0.118254 0.858022 0.477430 -0.147903]
 origin location in camera frame:
 [ 1.290553e-01, 2.785050e-01, 7.551241e-01, 0.000000e+00 ]
 
 
 2014-11-19 22:21:39.239 RC3DKSampleAR[1552:511316] Code detected Test QR code. Yay!!! at 163503947971, corners (320.408249, 171.481519), (283.825340, 253.772392), (358.216667, 299.043674), (399.801292, 211.599283)
 calibrated[0] = (feature_t){.x = 0.012237, .y = -0.129332};
 calibrated[1] = (feature_t){.x = -0.056257, .y = 0.024655};
 calibrated[2] = (feature_t){.x = 0.082966, .y = 0.109323};
 calibrated[3] = (feature_t){.x = 0.160663, .y = -0.054232};
 T qr is:[ -2.037080e-01, 4.227303e-02, -1.386526e+00, 0.000000e+00 ]
 new origin translation is:[ 4.638916e-02, 4.214264e-01, -1.334260e+00, 0.000000e+00 ]
 
 2014-11-19 22:21:39.279 RC3DKSampleAR[1552:511316] Code detected Test QR code. Yay!!! at 163503989592, corners (320.408249, 172.265825), (283.825340, 254.556684), (358.216667, 299.827995), (399.801292, 212.383604)
 calibrated[0] = (feature_t){.x = 0.012233, .y = -0.127867};
 calibrated[1] = (feature_t){.x = -0.056263, .y = 0.026128};
 calibrated[2] = (feature_t){.x = 0.082963, .y = 0.110795};
 calibrated[3] = (feature_t){.x = 0.160665, .y = -0.052764};
 T qr is:[ 4.133630e-02, -9.919677e-03, 8.527199e-01, 0.000000e+00 ]
 new origin translation is:[ 9.023930e-03, -4.399321e-01, 7.473491e-01, 0.000000e+00 ]
 */
TEST(Homography, BackProjectDual)
{
    feature_t calibrated[4];
    calibrated[0] = (feature_t){.x = 0.012237, .y = -0.129332};
    calibrated[1] = (feature_t){.x = -0.056257, .y = 0.024655};
    calibrated[2] = (feature_t){.x = 0.082966, .y = 0.109323};
    calibrated[3] = (feature_t){.x = 0.160663, .y = -0.054232};
    float qr_width = 0.15;
    
    m4 Rres;
    v4 Tres;
    bool success = homography_solve_qr(calibrated, qr_width, Rres, Tres);
    
    v4 ideal_points[4];
    ideal_points[0] = v4(-.5f*qr_width, .5f*qr_width, 0., 1.);
    ideal_points[1] = v4(-.5f*qr_width, -.5f*qr_width, 0., 1.);
    ideal_points[2] = v4(.5f*qr_width, -.5f*qr_width, 0., 1.);
    ideal_points[3] = v4(.5f*qr_width, .5f*qr_width, 0., 1.);
    
    feature_t back_projected[4], delta[4];
    
    for(int i = 0; i < 4; ++i)
    {
        v4 cam = Rres * ideal_points[i] + Tres;
        back_projected[i].x = cam[0] / cam[2];
        back_projected[i].y = cam[1] / cam[2];
        delta[i].x = fabs(calibrated[i].x - back_projected[i].x);
        delta[i].y = fabs(calibrated[i].y - back_projected[i].y);
        
        fprintf(stderr, "calibrated %f %f, backprojected %f %f, delta %f %f\n", calibrated[i].x, calibrated[i].y, back_projected[i].x, back_projected[i].y, delta[i].x, delta[i].y);
    }
    
    Rres.print(); Tres.print();
    EXPECT_EQ(success, true);
    
    
    
    calibrated[0] = (feature_t){.x = 0.012233, .y = -0.127867};
    calibrated[1] = (feature_t){.x = -0.056263, .y = 0.026128};
    calibrated[2] = (feature_t){.x = 0.082963, .y = 0.110795};
    calibrated[3] = (feature_t){.x = 0.160665, .y = -0.052764};
    
    success = homography_solve_qr(calibrated, qr_width, Rres, Tres);
    
    ideal_points[0] = v4(-.5f*qr_width, .5f*qr_width, 0., 1.);
    ideal_points[1] = v4(-.5f*qr_width, -.5f*qr_width, 0., 1.);
    ideal_points[2] = v4(.5f*qr_width, -.5f*qr_width, 0., 1.);
    ideal_points[3] = v4(.5f*qr_width, .5f*qr_width, 0., 1.);
    
    for(int i = 0; i < 4; ++i)
    {
        v4 cam = Rres * ideal_points[i] + Tres;
        back_projected[i].x = cam[0] / cam[2];
        back_projected[i].y = cam[1] / cam[2];
        delta[i].x = fabs(calibrated[i].x - back_projected[i].x);
        delta[i].y = fabs(calibrated[i].y - back_projected[i].y);
        
        fprintf(stderr, "calibrated %f %f, backprojected %f %f, delta %f %f\n", calibrated[i].x, calibrated[i].y, back_projected[i].x, back_projected[i].y, delta[i].x, delta[i].y);
    }
    
    Rres.print(); Tres.print();
    EXPECT_EQ(success, true);
}
