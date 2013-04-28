#include "filter_setup.h"

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

    dispatch_addclient(input, &sfm, sfm_imu_measurement);
    dispatch_addclient(input, &sfm, sfm_accelerometer_measurement);
    dispatch_addclient(input, &sfm, sfm_gyroscope_measurement);
    dispatch_addclient(input, &sfm, sfm_image_measurement);
    dispatch_addclient(input, &sfm, sfm_control);
    dispatch_add_rewrite(input, packet_camera, 16667);
}

struct corvis_device_parameters filter_setup::get_device_parameters()
{
    corvis_device_parameters dc = device;
    dc.K[0] = sfm.s.k1.v;
    dc.K[1] = sfm.s.k2.v;
    dc.K[2] = sfm.s.k3.v;
    dc.Fx = sfm.s.focal_length.v;
    dc.Cx = sfm.s.center_x.v;
    dc.Cy = sfm.s.center_y.v;
    for(int i = 0; i < 3; ++i) {
        dc.a_bias[i] = sfm.s.a_bias.v[i];
        dc.a_bias_var[i] = sfm.s.a_bias.variance[i];
        dc.w_bias[i] = sfm.s.w_bias.v[i];
        dc.w_bias_var[i] = sfm.s.w_bias.variance[i];
        dc.Tc[i] = sfm.s.Tc.v[i];
        dc.Tc_var[i] = sfm.s.Tc.variance[i];
        dc.Wc[i] = sfm.s.Wc.v[i];
        dc.Wc_var[i] = sfm.s.Wc.variance[i];
    }
    return dc;
}

filter_setup::~filter_setup()
{
    delete trackdata.dispatch;
    delete solution.dispatch;
}

#define FAILURE_MAPBUFFER 0x01
#define FAILURE_TRACKER 0x02
#define FAILURE_DETECTOR 0x04
#define FAILURE_USER_SPEED 0x08
#define FAILURE_ACCELEROMETER_SATURATION 0x10
#define FAILURE_GYROSCOPE_SATURATION 0x20
#define FAILURE_INNOVATION_STDEV 0x40
#define FAILURE_INNOVATION_THRESH 0x80
#define FAILURE_NUMERIC 0x100

const f_t accelerometer_saturation = 1.9 * 9.8;
const f_t gyroscope_saturation = 230. / 180. * M_PI;

int filter_setup::check_health()
{
    int reason = 0;
    if(input->mb->has_blocked) {
        reason |= FAILURE_MAPBUFFER;
    }

    if(sfm.detector_failed) {
        reason |= FAILURE_DETECTOR;
    }

    if(sfm.tracker_failed) {
        reason |= FAILURE_TRACKER;
    }

    if(sfm.accelerometer_max > accelerometer_saturation) {
        reason |= FAILURE_ACCELEROMETER_SATURATION;
    }

    if(sfm.gyroscope_max > gyroscope_saturation) {
        reason |= FAILURE_GYROSCOPE_SATURATION;
    }

    if(sfm.speed_failed) {
        reason |= FAILURE_USER_SPEED;
    }

    if(sfm.numeric_failed) {
        reason |= FAILURE_NUMERIC;
    }

    return reason;
}
