#pragma once

#include "rc_tracker.h"
#include "sensor.h"

static rc_Pose to_rc_Pose(const transformation &g)
{
    rc_Pose p;
    m_map(p.R.v) = g.Q.toRotationMatrix().cast<float>();
    v_map(p.Q.v) = g.Q.coeffs().cast<float>();
    v_map(p.T.v) = g.T.cast<float>();
    return p;
}

static transformation to_transformation(const rc_Pose p)
{
    quaternion Q(v_map(p.Q.v).cast<f_t>()); m3 R = m_map(p.R.v).cast<f_t>(); v3 T = v_map(p.T.v).cast<f_t>();
    return std::fabs(R.determinant() - 1) < std::fabs(Q.norm() - 1) ? transformation(R, T) : transformation(Q, T);
}

static struct sensor::extrinsics rc_Extrinsics_to_sensor_extrinsics(const rc_Extrinsics e)
{
    struct sensor::extrinsics extrinsics;
    extrinsics.mean = transformation(rotation_vector(e.W.x, e.W.y, e.W.z), v3(e.T.x, e.T.y, e.T.z));
    extrinsics.variance.Q = v_map(e.W_variance.v);
    extrinsics.variance.T = v_map(e.T_variance.v);
    return extrinsics;
}

static rc_Extrinsics rc_Extrinsics_from_sensor_extrinsics(struct sensor::extrinsics e)
{
    rc_Extrinsics extrinsics = {};
    v_map(extrinsics.W.v) = to_rotation_vector(e.mean.Q).raw_vector();
    v_map(extrinsics.T.v) = e.mean.T;
    v_map(extrinsics.W_variance.v) = e.variance.Q;
    v_map(extrinsics.T_variance.v) = e.variance.T;
    return extrinsics;
}
