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
    x[0] = v4(-.5 * width, -.5 * width, 1., 1.);
    x[1] = v4(-.5 * width, .5 * width, 1., 1.);
    x[2] = v4(.5 * width, .5 * width, 1., 1.);
    x[3] = v4(.5 * width, -.5 * width, 1., 1.);
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
