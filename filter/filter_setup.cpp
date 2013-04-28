#include "filter_setup.h"

void filter_setup::tracker_config()
{
    sfm.track.groupsize = 24;
    sfm.track.maxgroupsize = 40;
    sfm.track.maxfeats = 90;
}

void filter_setup::filter_config()
{
    sfm.s.T.variance = 1.e-7;
    sfm.s.W.variance = v4(10., 10., 1.e-7, 0.);
    sfm.s.V.variance = 1. * 1.;
    sfm.s.w.variance = .5 * .5;
    sfm.s.dw.variance = 2. * 2.; //observed range of variances in sequences is 1-6
    sfm.s.a.variance = .1 * .1;
    sfm.s.da.variance = 50. * 50.; //observed range of variances in sequences is 10-50
    sfm.s.g.variance = 1.e-7;
    sfm.s.Wc.variance = v4(device.Wc_var[0], device.Wc_var[1], device.Wc_var[2], 0.);
    sfm.s.Tc.variance = v4(device.Tc_var[0], device.Tc_var[1], device.Tc_var[2], 0.);
    sfm.s.a_bias.v = v4(device.a_bias[0], device.a_bias[1], device.a_bias[2], 0.);
    sfm.s.a_bias.variance = v4(device.a_bias_var[0], device.a_bias_var[1], device.a_bias_var[2], 0.);
    sfm.s.w_bias.v = v4(device.w_bias[0], device.w_bias[1], device.w_bias[2], 0.);
    sfm.s.w_bias.variance = v4(device.w_bias_var[0], device.w_bias_var[1], device.w_bias_var[2], 0.);
    sfm.s.focal_length.variance = 10.;
    sfm.s.center_x.variance = 4.;
    sfm.s.center_y.variance = 4.;
    sfm.s.k1.variance = .1;
    sfm.s.k2.variance = .1;
    sfm.s.k3.variance = .1;

    sfm.init_vis_cov = 4.;
    sfm.max_add_vis_cov = 2.;
    sfm.min_add_vis_cov = .5;

    sfm.s.T.process_noise = 0.;
    sfm.s.W.process_noise = 0.;
    sfm.s.V.process_noise = 0.;
    sfm.s.w.process_noise = 0.;
    sfm.s.dw.process_noise = 40. * 40.; // this stabilizes dw.stdev around 5-6
    sfm.s.a.process_noise = 0.;
    sfm.s.da.process_noise = 400. * 400.; //this stabilizes da.stdev around 45-50
    sfm.s.g.process_noise = 1.e-7;
    sfm.s.Wc.process_noise = 1.e-7;
    sfm.s.Tc.process_noise = 1.e-7;
    sfm.s.a_bias.process_noise = 1.e-7;
    sfm.s.w_bias.process_noise = 1.e-7;
    sfm.s.focal_length.process_noise = 1.e-7;
    sfm.s.center_x.process_noise = 1.e-7;
    sfm.s.center_y.process_noise = 1.e-7;
    sfm.s.k1.process_noise = 1.e-7;
    sfm.s.k2.process_noise = 1.e-7;
    sfm.s.k3.process_noise = 1.e-7;

    sfm.vis_ref_noise = 1.e-7;
    sfm.vis_noise = 1.e-7;

    sfm.vis_cov = 2. * 2.;
    sfm.w_variance = device.w_meas_var;
    sfm.a_variance = device.a_meas_var;

    sfm.min_feats_per_group = 6;
    sfm.min_group_add = 16;
    sfm.max_group_add = 40;
    sfm.max_features = 80;
    sfm.active = false;
    sfm.max_state_size = 128;
    sfm.frame = 0;
    sfm.skip = 1;
    sfm.min_group_health = 10.;
    sfm.max_feature_std_percent = .10;
    sfm.outlier_thresh = 1.5;
    sfm.outlier_reject = 10.;

    sfm.s.focal_length.v = device.Fx;
    sfm.s.center_x.v = device.Cx;
    sfm.s.center_y.v = device.Cy;
    sfm.s.k1.v = device.K[0];
    sfm.s.k2.v = device.K[1];
    sfm.s.k3.v = device.K[2];

    sfm.s.Tc.v = v4(device.Tc[0], device.Tc[1], device.Tc[2], 0.);
    sfm.s.Wc.v = v4(device.Wc[0], device.Wc[1], device.Wc[2], 0.);

    sfm.shutter_delay = device.shutter_delay;
    sfm.shutter_period = device.shutter_period;
    sfm.image_height = device.image_height;
}

filter_setup::filter_setup(dispatch_t *input, const char *outfn, struct corvis_device_parameters *device_params): sfm(true)
{
    device = *device_params;

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

    tracker_config();
    sfm.track.sink = &trackdata;

    filter_config();
    filter_init(&sfm);

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
