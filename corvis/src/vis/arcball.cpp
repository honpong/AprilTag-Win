#include "arcball.h"

void arcball::reset()
{
    q = quaternion();
}

void arcball::rotate_quaternion_with_delta(float dx, float dy)
{
    v4 up(0,1,0,0);
    v4 right(-1,0,0,0);

    // up and right now rotation_vectors
    up = quaternion_rotate(conjugate(q), up)*dx*radians_per_pixel;
    right = quaternion_rotate(conjugate(q), right)*dy*radians_per_pixel;

    quaternion dqx = to_quaternion(rotation_vector(up[0], up[1], up[2]));
    quaternion dqy = to_quaternion(rotation_vector(right[0], right[1], right[2]));
    q = quaternion_product(q, quaternion_product(dqx, dqy));
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
    v4 axis(0, 0, -1, 0);
    axis = quaternion_rotate(conjugate(q), axis);

    float delta = rotation - last_view_rotation;
    // axis is now a rotation vector
    axis = axis*delta;
    quaternion delta_q = to_quaternion(rotation_vector(axis[0], axis[1], axis[2]));
    q = quaternion_product(q, delta_q);

    last_view_rotation = rotation;
}