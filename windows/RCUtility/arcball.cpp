#include "arcball.h"

static const float radians_per_pixel = (float)(M_PI/640.);

typedef struct _v4 {
    _v4() {v[0] = 0; v[1] = 0; v[2] = 0; v[3] = 0;};
    _v4(float x, float y, float z, float w) {v[0] = x; v[1] = y; v[2] = z; v[3] = w;};
    union {
        float v[4];
        struct { float x, y, z, w; };
    };
} v4;
typedef v4 quaternion;

static quaternion q(0,0,0,1);

static v4 scale(const v4 & vec, float amount)
{
    return v4(vec.v[0]*amount, vec.v[1]*amount, vec.v[2]*amount, vec.v[3]*amount);
}

static float norm(const v4 & vec)
{
    return sqrt(vec.v[0]*vec.v[0] + vec.v[1]*vec.v[1] + vec.v[2]*vec.v[2] + vec.v[3]*vec.v[3]);
}

static quaternion quaternion_multiply(const quaternion & q1, const quaternion & q2)
{
    quaternion result;
    result.w = q1.w*q2.w - q1.x*q2.x - q1.y*q2.y - q1.z*q2.z;
    result.x = q1.x*q2.w + q1.w*q2.x - q1.z*q2.y + q1.y*q2.z;
    result.y = q1.y*q2.w + q1.z*q2.x + q1.w*q2.y - q1.x*q2.z;
    result.z = q1.z*q2.w - q1.y*q2.x + q1.x*q2.y + q1.w*q2.z;
    return result;
}

static quaternion quaternion_invert(const quaternion & qin)
{
    quaternion qi(-qin.x, -qin.y, -qin.z, qin.w);
    return qi;
}

#include <stdio.h>
static quaternion axis_angle_to_quaternion(const v4 & v)
{
    v4 w(.5f * v.x, .5f * v.y, .5f * v.z, 0.f);

    float th2 = w.x*w.x + w.y*w.y + w.z*w.z;
    float th = sqrt(th2), C = cos(th);
    float S;
    if(th < sqrt(120.f * 1e-5))
        S = 1.f - th2/6.f + th2*th2/120.f;
    else
        S = sin(th)/th;
    return quaternion(S * w.x, S * w.y, S * w.z, C); // e^(v/2)
}

// R should be 3x4
static void quaternion_to_rotation(const quaternion & qin, float * R)
{
    float
    xx = qin.x*qin.x,
    yy = qin.y*qin.y,
    zz = qin.z*qin.z,
    wx = qin.w*qin.x,
    wy = qin.w*qin.y,
    wz = qin.w*qin.z,
    xy = qin.x*qin.y,
    xz = qin.x*qin.z,
    yz = qin.y*qin.z;

    R[0] = 1.f - 2.f * (yy+zz); R[1] = 2.f * (xy-wz); R[2] = 2.f * (xz+wy); R[3] = 0.f;
    R[4] = 2.f * (xy+wz); R[5] = 1.f - 2.f * (xx+zz); R[6] = 2.f * (yz-wx); R[7] = 0.f;
    R[8] = 2.f * (xz-wy); R[9] = 2.f * (yz+wx); R[10] = 1.f - 2.f * (xx+yy); R[11] = 0.f;
}

static v4 matrix_times_v4(float * M, const v4 & v)
{
    v4 result;
    result.x = v.v[0]*M[0] + v.v[1]*M[1] + v.v[2]*M[2]  + v.v[3]*M[3];
    result.y = v.v[0]*M[4] + v.v[1]*M[5] + v.v[2]*M[6]  + v.v[3]*M[7];
    result.z = v.v[0]*M[8] + v.v[1]*M[9] + v.v[2]*M[10] + v.v[3]*M[11];
    result.w = 1;

    return result;
}

static v4 quaternion_rotate(const quaternion & qin, const v4 & v)
{
    float rotation[12];
    quaternion_to_rotation(qin, rotation);
    return matrix_times_v4(rotation, v);
}

void arcball::reset()
{
    q = quaternion(0, 0, 0, 1);
}

void arcball::rotate_quaternion_with_delta(float dx, float dy)
{
    v4 up(0,1,0,0);
    v4 right(-1,0,0,0);

    // up and right now rotation_vectors
    up = quaternion_rotate(quaternion_invert(q), scale(up, dx*radians_per_pixel));
    right = quaternion_rotate(quaternion_invert(q), scale(right, dy*radians_per_pixel));

    quaternion dqx = axis_angle_to_quaternion(up);
    quaternion dqy = axis_angle_to_quaternion(right);
    q = quaternion_multiply(q, quaternion_multiply(dqx, dqy));
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
    axis = quaternion_rotate(quaternion_invert(q), axis);

    float delta = rotation - last_view_rotation;
    // axis is now a rotation vector
    axis = scale(axis, delta);
    quaternion delta_q = axis_angle_to_quaternion(axis);
    q = quaternion_multiply(q, delta_q);

    last_view_rotation = rotation;
}

#include <stdio.h>
void arcball::get_rotation(float * R)
{
    quaternion_to_rotation(quaternion_invert(q), R);
}
