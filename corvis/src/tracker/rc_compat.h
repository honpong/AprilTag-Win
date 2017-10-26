#pragma once

#include "rc_tracker.h"
#include "sensor.h"

static inline rc_Pose to_rc_Pose(const transformation &g)
{
    rc_Pose p;
    map(p.R.v) = g.Q.toRotationMatrix().cast<float>();
    map(p.Q.v) = g.Q.coeffs().cast<float>();
    map(p.T.v) = g.T.cast<float>();
    return p;
}

static inline transformation to_transformation(const rc_Pose p)
{
    quaternion Q(map(p.Q.v).cast<f_t>()); m3 R = map(p.R.v).cast<f_t>(); v3 T = map(p.T.v).cast<f_t>();
    return Q.norm() < F_T_EPS && R.determinant() < F_T_EPS ? transformation() :
        std::fabs(R.determinant() - 1) < std::fabs(Q.norm() - 1) ? transformation(R, T) : transformation(Q, T);
}

static inline struct sensor::extrinsics rc_Extrinsics_to_sensor_extrinsics(const rc_Extrinsics e)
{
    struct sensor::extrinsics extrinsics;
    extrinsics.mean = to_transformation(e.pose_m);
    extrinsics.variance.Q = map(e.variance_m2.W.v);
    extrinsics.variance.T = map(e.variance_m2.T.v);
    return extrinsics;
}

static inline rc_Extrinsics rc_Extrinsics_from_sensor_extrinsics(const struct sensor::extrinsics &e)
{
    rc_Extrinsics extrinsics = {};
    extrinsics.pose_m = to_rc_Pose(e.mean);
    map(extrinsics.variance_m2.W.v) = e.variance.Q;
    map(extrinsics.variance_m2.T.v) = e.variance.T;
    return extrinsics;
}
