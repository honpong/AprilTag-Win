#include "camera.h"

f_t estimate_kr(f_t x, f_t y, f_t k1, f_t k2, f_t k3)
{
    f_t r2 = x*x + y*y;
    f_t kr = 1. + r2 * (k1 + r2 * (k2 + r2 * k3));
    return kr;
}
v3 camera::project_image_point(f_t x, f_t y) const
{
    return v3((x - center_x)/focal_length, (y - center_y)/focal_length, 1);
}

v3 camera::calibrate_image_point(f_t x, f_t y) const
{
    feature_t undistorted = undistort_image_point(x, y);
    return project_image_point(undistorted.x(), undistorted.y());
}

feature_t camera::undistort_image_point(f_t x, f_t y) const
{
    // Project distorted point
    f_t projected_x = (x - center_x)/focal_length;
    f_t projected_y = (y - center_y)/focal_length;
    // Estimate kr
    f_t r2 = projected_x*projected_x + projected_y*projected_y;
    f_t kr = 1. + r2 * (k1 + r2 * (k2 + r2 * k3));

    // Adjust distorted projection by kr, and back project to image
    return feature_t {(float)(projected_x * focal_length / kr + center_x),
                      (float)(projected_y * focal_length / kr + center_y)};
}

#define interp(c0, c1, t) ((c0)*(1-(t)) + ((c1)*(t)))
float bilinear_interp(const uint8_t * image, int width, int height, float x, float y)
{
    float result = 0;
    int xi = (int)x;
    int yi = (int)y;
    float tx = x - xi;
    float ty = y - yi;
    if(xi < 0 || xi >= width-1 || yi < 0 || yi >= height-1)
        return 0;
    float c00 = image[yi*width + xi];
    float c01 = image[(yi+1)*width + xi];
    float c10 = image[yi*width + xi + 1];
    float c11 = image[(yi+1)*width + xi + 1];
    result = interp(interp(c00, c10, tx), interp(c01, c11, tx), ty);
    return result;
}

/* TODO: Extend to multiple channels */
void camera::undistort_image(const uint8_t * input, uint8_t * output, bool * valid) const
{
    for(int y = 0; y < height; y++)
        for(int x = 0; x < width; x++) {
            feature_t undistorted = undistort_image_point(x, y);
            if(undistorted.x() < 0 || undistorted.x() >= width-1 ||
               undistorted.y() < 0 || undistorted.y() >= height-1) {
                valid[y*width + x] = false;
                output[y*width + x] = 0;
            }
            else {
                valid[y*width + x] = true;
                output[y*width + x] = bilinear_interp(input, width, height, undistorted.x(), undistorted.y());
            }
        }
}


