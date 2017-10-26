#include "gtest/gtest.h"
#include "rc_compat.h"
#include "util.h"

#include <random>
#include <algorithm>

TEST(Prediction, QuaternionMultiply)
{
    std::default_random_engine gen(100);
    std::uniform_real_distribution<float> r;
    for (int i=0; i<100; i++) {
        transformation t1(quaternion(r(gen), r(gen), r(gen), r(gen)).normalized(), v3(0,0,0));
        transformation t2(quaternion(r(gen), r(gen), r(gen), r(gen)).normalized(), v3(0,0,0));
        transformation t12(t1.Q * t2.Q, v3(0,0,0));
        rc_Pose p1 = to_rc_Pose(t1);
        rc_Pose p2 = to_rc_Pose(t2);
        rc_Pose p12 = { rc_quaternionMultiply(p1.Q, p2.Q) };
        EXPECT_QUATERNION_NEAR(t12.Q, to_transformation(p12).Q, F_T_EPS);
    }
}

TEST(Prediction, QuaternionExp)
{
    std::default_random_engine gen(100);
    std::uniform_real_distribution<float> r;
    for (int i=0; i<100; i++) {
        rc_Vector w = {{r(gen), r(gen), r(gen)}};
        rotation_vector W(w.x,w.y,w.z);
        rc_Pose p = { rc_quaternionExp(w), rc_Vector{{0,0,0}} };
        transformation P = to_transformation(p);
        EXPECT_QUATERNION_NEAR(to_quaternion(W), P.Q, F_T_EPS);
    }
}

#include "sensor_fusion.h"

TEST(Prediction, Predict)
{
    std::unique_ptr<rc_Tracker, decltype(rc_destroy)*> rc{rc_create(), rc_destroy};
    rc_Extrinsics e = {rc_POSE_IDENTITY}; rc_AccelerometerIntrinsics ai = {}; rc_GyroscopeIntrinsics gi = {};
    rc_configureAccelerometer(rc.get(), 0, &e, &ai);
    rc_configureGyroscope(rc.get(), 0, &e, &gi);
    rc_startTracker(rc.get(), rc_RUN_SYNCHRONOUS);
    state_motion &s = ((sensor_fusion *)rc.get())->sfm.s;
    s.remap();
    std::default_random_engine gen(100);
    std::uniform_real_distribution<float> r;
    for (int i=0; i<100; i++) {
        s.Q.v = quaternion(r(gen), r(gen), r(gen), r(gen)).normalized();
        s.T.v = v3(r(gen), r(gen), r(gen));
        s.w.v = v3(r(gen), r(gen), r(gen));
        s.V.v = v3(r(gen), r(gen), r(gen));
        s.dw.v = v3(r(gen), r(gen), r(gen));
        s.a.v = v3(r(gen), r(gen), r(gen));

        transformation t0_(s.Q.v, s.T.v);
        rc_PoseVelocity v; rc_PoseAcceleration a;
        rc_Pose p0 = rc_getPose(rc.get(), &v, &a, rc_DATA_PATH_SLOW).pose_m;
        transformation t0 = to_transformation(p0);
        EXPECT_TRANSFORMATION_NEAR(t0, t0_, 8*F_T_EPS) << "t=0";

        f_t dt_s = .100 * r(gen); rc_Timestamp dt_us = dt_s * 1e6;
        rc_Pose p1 = rc_predictPose(dt_us, p0, v, a);
        transformation t1 = to_transformation(p1);
        s.evolve(dt_s);
        transformation t1_(s.Q.v, s.T.v);
        EXPECT_TRANSFORMATION_NEAR(t1, t1_, 9*F_T_EPS) << "t=1";;
    }
}
