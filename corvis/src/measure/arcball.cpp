#include "arcball.h"

void arcball::reset()
{
    q = quaternion::Identity();
}

void arcball::rotate_quaternion_with_delta(float dx, float dy)
{
    v3 up(0,1,0);
    v3 right(-1,0,0);

    // up and right now rotation_vectors
    up = q.conjugate()*up*dx*radians_per_pixel;
    right = q.conjugate()*right*dy*radians_per_pixel;

    quaternion dqx = to_quaternion(rotation_vector(up));
    quaternion dqy = to_quaternion(rotation_vector(right));
    q = q*dqx*dqy;
}

// Rotates by clicking and dragging
void arcball::start_rotation(float x, float y)
{
    last_x = x;
    last_y = y;
}

void arcball::continue_rotation(float x, float y)
{
    // image to GL coordinates
    rotate_quaternion_with_delta(x - last_x, -(y - last_y));
    last_x = x;
    last_y = y;
}

// Rotates in the image plane of the current view
void arcball::start_view_rotation(float rotation)
{
    last_view_rotation = rotation;
}

void arcball::continue_view_rotation(float rotation)
{
    v3 axis(0, 0, -1);
    axis = q.conjugate()*axis;

    float delta = rotation - last_view_rotation;
    // axis is now a rotation vector
    axis = axis*delta;
    quaternion delta_q = to_quaternion(rotation_vector(axis));
    q = q*delta_q;

    last_view_rotation = rotation;
}
