#include "gtest/gtest.h"
#include "homography.h"
#include "rotation_vector.h"

const static rotation_vector W(-.13, 1.3, -.1);
const static v4 T(.12, .79, 4., 0.);

TEST(Homography, Qr)
{
    // ideal qr plane is 1m on a side, 1m away (same as we use in
    // compute_qr_homography)
    v4 x[4];
    x[0] = v4(-.5, .5, 1., 1.);
    x[1] = v4(-.5, -.5, 1., 1.);
    x[2] = v4(.5, -.5, 1., 1.);
    x[3] = v4(.5, .5, 1., 1.);
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
    compute_qr_homography(image, 1.);
}
