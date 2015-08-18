#ifndef __ARCBALL__
#define __ARCBALL__

#define _USE_MATH_DEFINES
#include <cmath>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

class arcball
{
private:
    float last_x, last_y;
    float last_view_rotation;
    void rotate_quaternion_with_delta(float dx, float dy);

public:
    void reset();

    // Rotates by clicking and dragging
    void start_rotation(float x, float y);
    void continue_rotation(float x, float y);
    // Rotates in the image plane of the current view
    void start_view_rotation(float rotation);
    void continue_view_rotation(float rotation);
    // Gets the current rotation
    void get_rotation(float * rotation);
};

#endif
