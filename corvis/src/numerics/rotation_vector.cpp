#include "rotation_vector.h"
#include "vec4.h"
#include "sinc.h"

// returns I*m4_identity + S * skew3(v) + C * skew3(v) * skew3(v);
static m3 skew_series(const v3 &v, f_t I, f_t S, f_t C)
{
    f_t x=v[0], y=v[1], z=v[2];
    
    //Need explicit naming of return type to work around compiler bug
    //https://connect.microsoft.com/VisualStudio/Feedback/Details/1515546
    return m3 {
        {I - C*(y*y+z*z), C*x*y - S*z, C*x*z + S*y},
        {C*y*x + S*z, I - C*(x*x+z*z), C*y*z - S*x},
        {C*z*x - S*y, C*z*y + S*x, I - C*(x*x+y*y)}
    };
}

// e^\hat{v}
m3 to_rotation_matrix(const rotation_vector &v)
{
    f_t th2, th = sqrt(th2=v.norm2());
    return skew_series(v3(v.x(), v.y(), v.z()), 1, sinc(th,th2), cosc(th,th2));
}

// returns d_spatial(v) defined by d_spatial(v) dv = \unhat{(d e^\hat{v})e^\hat{-v}}}
m3 to_spatial_jacobian(const rotation_vector &v)
{
    f_t th2, th = sqrt(th2=v.norm2());
    return skew_series(v3(v.x(), v.y(), v.z()), 1, cosc(th,th2), sincc(th,th2));
}

// returns d_body(v) defined by d_body(v) dv = \unhat{e^\hat{-v} d e^\hat{v}}
m3 to_body_jacobian(const rotation_vector &v)
{
    return to_spatial_jacobian(-v);
}


