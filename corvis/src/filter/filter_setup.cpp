#include "filter_setup.h"

filter_setup::filter_setup(device_parameters *device_params)
{
    device = *device_params;
    filter_initialize(&sfm, device_params);
}

//TODO: Make it so speed error doesn't cause reset?
RCSensorFusionErrorCode filter_setup::get_error()
{
    RCSensorFusionErrorCode errorCode = RCSensorFusionErrorCodeNone;
    if (get_other_failure())
        errorCode = RCSensorFusionErrorCodeOther;
    else if(get_speed_failure())
        errorCode = RCSensorFusionErrorCodeTooFast;
    else if (get_vision_failure())
        errorCode = RCSensorFusionErrorCodeVision;

    RCSensorFusionRunState state = sfm.run_state;
    if(errorCode == RCSensorFusionErrorCodeVision && state != RCSensorFusionRunStateRunning)
    {
        sfm.detector_failed = false; //Clear to avoid repeating error before we try detection again
    }

    if(errorCode == RCSensorFusionErrorCodeTooFast || errorCode == RCSensorFusionErrorCodeOther) {
        // Do a full filter reset
        filter_initialize(&sfm, &device);
        switch(state)
        {
            case RCSensorFusionRunStateInactive:
                //This should never happen.
#ifdef DEBUG
                assert(0);
#endif
            case RCSensorFusionRunStateRunning:
                //OK, just stop and report it to the user.
                break;

            //All others get handled silently
            case RCSensorFusionRunStateSteadyInitialization:
                errorCode = RCSensorFusionErrorCodeNone;
                filter_start_hold_steady(&sfm);
                break;
            case RCSensorFusionRunStateDynamicInitialization:
                errorCode = RCSensorFusionErrorCodeNone;
                filter_start_dynamic(&sfm);
                break;
            case RCSensorFusionRunStateLandscapeCalibration:
            case RCSensorFusionRunStatePortraitCalibration:
            case RCSensorFusionRunStateStaticCalibration:
                errorCode = RCSensorFusionErrorCodeNone;
                filter_start_static_calibration(&sfm);
                break;
        }
    }
    return errorCode;
}

device_parameters filter_setup::get_device_parameters()
{
    device_parameters dc = device;
    dc.K0 = (float)sfm.s.k1.v;
    dc.K1 = (float)sfm.s.k2.v;
    dc.K2 = (float)sfm.s.k3.v;
    dc.distortionModel = sfm.s.fisheye;
    dc.Fx = dc.Fy = (float)(sfm.s.focal_length.v * sfm.s.image_height);
    dc.Cx = (float)(sfm.s.center_x.v * sfm.s.image_height + sfm.s.image_width / 2. - .5);
    dc.Cy = (float)(sfm.s.center_y.v * sfm.s.image_height + sfm.s.image_height / 2. - .5);
    dc.px = dc.py = 0.;
    for(int i = 0; i < 3; ++i) {
        dc.a_bias[i] = (float)sfm.s.a_bias.v[i];
        dc.a_bias_var[i] = (float)sfm.s.a_bias.variance()[i];
        dc.w_bias[i] = (float)sfm.s.w_bias.v[i];
        dc.w_bias_var[i] = (float)sfm.s.w_bias.variance()[i];
        dc.Tc[i] = (float)sfm.s.Tc.v[i];
        dc.Tc_var[i] = (float)sfm.s.Tc.variance()[i];
        dc.Wc[i] = (float)to_rotation_vector(sfm.s.Qc.v).raw_vector()[i];
        dc.Wc_var[i] = (float)sfm.s.Qc.variance()[i];
    }
    dc.a_meas_var = (float)sfm.a_variance;
    dc.w_meas_var = (float)sfm.w_variance;
    device = dc;
    return dc;
}

#define FAILURE_TRACKER 0x02
#define FAILURE_DETECTOR 0x04
#define FAILURE_USER_SPEED 0x08
#define FAILURE_ACCELEROMETER_SATURATION 0x10
#define FAILURE_GYROSCOPE_SATURATION 0x20
#define FAILURE_INNOVATION_STDEV 0x40
#define FAILURE_INNOVATION_THRESH 0x80
#define FAILURE_NUMERIC 0x100
#define FAILURE_MAPBUFFER 0x200

int filter_setup::get_failure_code()
{
    int reason = 0;

    if(sfm.detector_failed) {
        reason |= FAILURE_DETECTOR;
    }

    if(sfm.tracker_failed) {
        reason |= FAILURE_TRACKER;
    }

    if(sfm.speed_failed) {
        reason |= FAILURE_USER_SPEED;
    }

    if(sfm.numeric_failed) {
        reason |= FAILURE_NUMERIC;
    }

    return reason;
}

bool filter_setup::get_speed_warning()
{
    return sfm.speed_warning;
}

bool filter_setup::get_vision_failure()
{
    return sfm.detector_failed;
}

bool filter_setup::get_speed_failure()
{
    return (sfm.speed_failed) || sfm.tracker_failed;
}

bool filter_setup::get_other_failure()
{
    return sfm.numeric_failed;
}

float filter_setup::get_filter_converged()
{
    return filter_converged(&sfm);
}

bool filter_setup::get_device_steady()
{
    return filter_is_steady(&sfm);
}

bool filter_setup::get_vision_warning()
{
    return sfm.tracker_warned;
}
