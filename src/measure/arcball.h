#ifndef __CORVIS_ARCBALL__
#define __CORVIS_ARCBALL__

#include <cmath>
#include "vec4.h"
#include "quaternion.h"

class arcball
{
private:
    float last_x, last_y;
    float last_view_rotation;
    quaternion q;
    void rotate_quaternion_with_delta(float dx, float dy);

public:
    float radians_per_pixel = (float)(M_PI/640.);
    void reset();

    // Rotates by clicking and dragging
    void start_rotation(float x, float y);
    void continue_rotation(float x, float y);
    // Rotates in the image plane of the current view
    void start_view_rotation(float rotation);
    void continue_view_rotation(float rotation);
    // Gets the current rotation
    quaternion get_quaternion() { return q; };
};

#endif
