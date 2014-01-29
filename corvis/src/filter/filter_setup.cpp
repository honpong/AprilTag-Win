#include "filter_setup.h"

filter_setup::filter_setup(corvis_device_parameters *device_params): sfm(true)
{
    device = *device_params;
    input = NULL;
    filter_init(&sfm, device);
    trackdata.dispatch = 0;
    solution.dispatch = 0;
}

filter_setup::filter_setup(dispatch_t *_input, const char *outfn, struct corvis_device_parameters *device_params): sfm(true)
{
    device = *device_params;
    input = _input;

    trackdata.size = 512 * 1024;
    trackdata.filename = NULL;
    trackdata.replay = false;
    trackdata.dispatch = new dispatch_t();
    plugins_register(mapbuffer_open(&trackdata));

    solution.size = 1 * 1024 * 1024;
    solution.filename = NULL;
    solution.replay = false;
    //solution.filename = outfn;
    solution.dispatch =  new dispatch_t();
    plugins_register(mapbuffer_open(&solution));

    sfm.track.sink = &trackdata;

    filter_init(&sfm, device);

    sfm.output = &solution;

    dispatch_addclient(input, &sfm, filter_imu_packet);
    dispatch_addclient(input, &sfm, filter_accelerometer_packet);
    dispatch_addclient(input, &sfm, filter_gyroscope_packet);
    dispatch_addclient(input, &sfm, filter_image_packet);
    dispatch_addclient(input, &sfm, filter_control_packet);
    dispatch_add_rewrite(input, packet_camera, 16667);
}

struct corvis_device_parameters filter_setup::get_device_parameters()
{
    corvis_device_parameters dc = device;
    dc.K[0] = sfm.s.k1.v;
    dc.K[1] = sfm.s.k2.v;
    dc.K[2] = sfm.s.k3.v;
    dc.Fx = dc.Fy = sfm.s.focal_length.v;
    dc.Cx = sfm.s.center_x.v;
    dc.Cy = sfm.s.center_y.v;
    dc.px = dc.py = 0.;
    for(int i = 0; i < 3; ++i) {
        dc.a_bias[i] = sfm.s.a_bias.v[i];
        dc.a_bias_var[i] = sfm.s.a_bias.variance()[i];
        dc.w_bias[i] = sfm.s.w_bias.v[i];
        dc.w_bias_var[i] = sfm.s.w_bias.variance()[i];
        dc.Tc[i] = sfm.s.Tc.v[i];
        dc.Tc_var[i] = sfm.s.Tc.variance()[i];
        dc.Wc[i] = sfm.s.Wc.v.raw_vector()[i];
        dc.Wc_var[i] = sfm.s.Wc.variance()[i];
    }
    dc.a_meas_var = sfm.a_variance;
    dc.w_meas_var = sfm.w_variance;
    device = dc;
    sfm.device = dc;
    return dc;
}

filter_setup::~filter_setup()
{
    if(trackdata.dispatch) delete trackdata.dispatch;
    if(solution.dispatch) delete solution.dispatch;
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

const f_t accelerometer_saturation = 1.9 * 9.8;
const f_t gyroscope_saturation = 230. / 180. * M_PI;

int filter_setup::get_failure_code()
{
    int reason = 0;
    if(input && input->mb->has_blocked) {
        reason |= FAILURE_MAPBUFFER;
    }

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
    return (input && input->mb->has_blocked) || sfm.numeric_failed;
}

float filter_setup::get_filter_converged()
{
    return filter_converged(&sfm);
}

bool filter_setup::get_device_steady()
{
    return filter_is_steady(&sfm);
}

bool filter_setup::get_device_aligned()
{
    return filter_is_aligned(&sfm);
}

bool filter_setup::get_vision_warning()
{
    return sfm.tracker_warned;
}
