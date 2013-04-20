#include "filter_setup.h"

void filter_setup::calibration_config()
{
    cal.F.x = device.Fx;
    cal.F.y = device.Fy;
    cal.C.x = device.Cx;
    cal.C.y = device.Cy;
    cal.p.x = device.px;
    cal.p.y = device.py;
    cal.K[0] = device.K[0];
    cal.K[1] = device.K[1];
    cal.K[2] = device.K[2];

    cal.out_F.x = cal.F.x;
    cal.out_F.y = cal.F.y;
    cal.out_C.x = cal.C.x;
    cal.out_C.y = cal.C.y;
    cal.alpha_c = 0.;
    cal.rotation[0] = 1.;
    cal.rotation[1] = 0.;
    cal.rotation[2] = 0.;
    cal.rotation[3] = 1.;
    cal.in_width = device.image_width;
    cal.in_height = device.image_height;
    cal.out_width = device.image_width;
    cal.out_height = device.image_height;
    cal.niter = 10;
    cal.maxerr = .1 / cal.F.x; //0.1 pixel
}

void filter_setup::tracker_config()
{
    sfm.track.groupsize = 24;
    sfm.track.maxgroupsize = 40;
    sfm.track.maxfeats = 90;
}

void filter_setup::filter_config()
{
    sfm.s.T.variance = 1.e-7;
    sfm.s.W.variance = v4(1., 1., 1.e-7, 0.);
    sfm.s.V.variance = 1. * 1.;
    sfm.s.w.variance = .5 * .5;
    sfm.s.dw.variance = 2. * 2.; //observed range of variances in sequences is 1-6
    sfm.s.a.variance = 2. * 2.;
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
    sfm.s.dw.process_noise = 20. * 20.; // this stabilizes dw.stdev around 2
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

    calibration_config();
    cal.feature_sink = &calibdata;
    cal.image_sink = &calibdata;
    calibration_init(&cal);

    tracker_config();
    sfm.track.sink = &trackdata;

    filter_config();
    sfm.calibration = &cal;
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
    return device;
}

filter_setup::~filter_setup()
{
    delete trackdata.dispatch;
    delete solution.dispatch;
}
