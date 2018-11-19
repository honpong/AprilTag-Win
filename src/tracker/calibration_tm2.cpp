#include "rc_tracker.h"
#include "tm2_config_tables.h"
#include <string.h>
#include <initializer_list>

#define set(a,b) static_assert(sizeof(a)==sizeof(b), "sizeof("#a")==sizeof("#b")"); memcpy(&a,&b,sizeof(a));

bool rc_setCalibrationTM2(rc_Tracker *tracker, const void *calibration, size_t size)
{
    if (size < sizeof(tm2_calibration_table))
        return false;
    tm2_calibration_table tm2 = {};

    memcpy(&tm2.header.header_size, calibration, sizeof(tm2.header.header_size));

    if (tm2.header.header_size != sizeof(tm2.header))
        return false;

    memcpy(&tm2.header, calibration, sizeof(tm2.header));

    if (tm2.header.header_major != HEADER_MAJOR)
        return false;
    if (tm2.header.table_major != QS_CAL_MAJOR)
        return false;
    if (tm2.header.header_size + tm2.header.table_size != sizeof(tm2))
        return false;

    memcpy(&tm2, calibration, sizeof(tm2));

    if (tm2.header.table_major == QS_CAL_MAJOR_VR && tm2.header.table_minor < QS_CAL_MINOR_VR)
        rc_configureWorld(tracker, rc_Vector{{0,0,1}}, rc_Vector{{0,1,0}}, rc_Vector{{0,0,1}}); // Old non-VR coordinates

    {
        int i=0;
        for (const auto &cam : {tm2.cam0, tm2.cam1}) {
            rc_Extrinsics ex = {}; set(ex,cam.Extrinsics);
            rc_CameraIntrinsics ci = {};
            ci.type          = (rc_CalibrationType)cam.Intrinsics.type;
            ci.height_px     = cam.Intrinsics.height_px;
            ci.width_px      = cam.Intrinsics.width_px;
            ci.f_x_px        = cam.Intrinsics.f_x_px;
            ci.f_y_px        = cam.Intrinsics.f_y_px;
            ci.c_x_px        = cam.Intrinsics.c_x_px;
            ci.c_y_px        = cam.Intrinsics.c_y_px;
            ci.distortion[0] = cam.Intrinsics.distortion[0];
            ci.distortion[1] = cam.Intrinsics.distortion[1];
            ci.distortion[2] = cam.Intrinsics.distortion[2];
            ci.distortion[3] = cam.Intrinsics.distortion[3];
            if (!rc_configureCamera(tracker, i++, rc_FORMAT_GRAY8, &ex, &ci))
                return false;
        }
    }

    {
        rc_Extrinsics ex = {}; set(ex, tm2.imu.Extrinsics);

        rc_GyroscopeIntrinsics gi = {};
        set(gi.scale_and_alignment,         tm2.imu.GyroIntrinsics.scale_and_alignment);
        set(gi.bias_rad__s,                 tm2.imu.GyroIntrinsics.bias_rad__s);
        set(gi.bias_variance_rad2__s2,      tm2.imu.GyroIntrinsics.bias_variance_rad2__s2);
        gi.measurement_variance_rad2__s2  = tm2.imu.GyroIntrinsics.measurement_variance_rad2__s2;
        gi.decimate_by                    = tm2.imu.GyroIntrinsics.decimate_by;

        if (!rc_configureGyroscope(tracker, 0, &ex, &gi))
            return false;

        rc_AccelerometerIntrinsics ai = {};
        set(ai.scale_and_alignment,         tm2.imu.AccIntrinsics.scale_and_alignment);
        set(ai.bias_m__s2,                  tm2.imu.AccIntrinsics.bias_m__s2);
        set(ai.bias_variance_m2__s4,        tm2.imu.AccIntrinsics.bias_variance_m2__s4);
        ai.measurement_variance_m2__s4    = tm2.imu.AccIntrinsics.measurement_variance_m2__s4;
        ai.decimate_by                    = tm2.imu.AccIntrinsics.decimate_by;
        if (!rc_configureAccelerometer(tracker, 0, &ex, &ai))
            return false;

        rc_ThermometerIntrinsics ti = {};
        ti.measurement_variance_C2   = tm2.imu.ThermometerIntrinsics.measurement_variance_C2;
        //ti.calibraiton_temperature_C = tm2.imu.ThermometerIntrinsics.calibraiton_temperature_C;
        if (!rc_configureThermometer(tracker, 0, &ex, &ti))
            return false;

    }

    return true;
}
