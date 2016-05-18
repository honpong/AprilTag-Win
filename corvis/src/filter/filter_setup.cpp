#include "filter_setup.h"

filter_setup::filter_setup(device_parameters *device_parameters)
{
    device = *device_parameters;
    filter_initialize(&sfm, device_parameters);
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
    auto &cam = device.color;
    auto &imu = device.imu;
    if (sfm.s.camera_intrinsics.fisheye) {
        cam.intrinsics.type = rc_CALIBRATION_TYPE_FISHEYE;
        cam.intrinsics.w = sfm.s.camera_intrinsics.k1.v;
        cam.intrinsics.k2 = 0;
        cam.intrinsics.k3 = 0;
    } else {
        cam.intrinsics.type = rc_CALIBRATION_TYPE_POLYNOMIAL3;
        cam.intrinsics.k1 = sfm.s.camera_intrinsics.k1.v;
        cam.intrinsics.k2 = sfm.s.camera_intrinsics.k2.v;
        cam.intrinsics.k3 = sfm.s.camera_intrinsics.k3.v;
    }
    cam.intrinsics.f_x_px = cam.intrinsics.f_y_px = (sfm.s.camera_intrinsics.focal_length.v * sfm.s.camera_intrinsics.image_height);
    cam.intrinsics.c_x_px = (sfm.s.camera_intrinsics.center_x.v * sfm.s.camera_intrinsics.image_height + sfm.s.camera_intrinsics.image_width  / 2. - .5);
    cam.intrinsics.c_y_px = (sfm.s.camera_intrinsics.center_y.v * sfm.s.camera_intrinsics.image_height + sfm.s.camera_intrinsics.image_height / 2. - .5);

    cam.extrinsics_wrt_imu_m.T     = sfm.s.extrinsics.Tc.v;
    cam.extrinsics_var_wrt_imu_m.T = sfm.s.extrinsics.Tc.variance();
    cam.extrinsics_wrt_imu_m.Q     = sfm.s.extrinsics.Qc.v;
    cam.extrinsics_var_wrt_imu_m.W = sfm.s.extrinsics.Qc.variance();

    imu.a_bias_m__s2         = sfm.s.imu_intrinsics.a_bias.v;
    imu.a_bias_var_m2__s4    = sfm.s.imu_intrinsics.a_bias.variance();
    imu.w_bias_rad__s        = sfm.s.imu_intrinsics.w_bias.v;
    imu.w_bias_var_rad2__s2  = sfm.s.imu_intrinsics.w_bias.variance();
    imu.a_noise_var_m2__s4   = sfm.a_variance;
    imu.w_noise_var_rad2__s2 = sfm.w_variance;
    return device;
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
