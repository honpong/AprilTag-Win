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
