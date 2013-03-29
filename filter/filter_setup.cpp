#include "filter_setup.h"

void filter_setup::calibration_config()
{
    cal.F.x = 604.241;
    cal.F.y = 604.028;
    cal.out_F.x = cal.F.x;
    cal.out_F.y = cal.F.y;
    cal.C.x = 317.576;
    cal.C.y = 237.755;
    cal.out_C.x = cal.C.x;
    cal.out_C.y = cal.C.y;
    cal.alpha_c = 0.;
    cal.p.x = -2.65791e-3;
    cal.p.y = 6.48762e-4;
    cal.K[0] = .2774956;
    cal.K[1] = -1.0795446;
    cal.K[2] = 1.14524733;
    cal.rotation[0] = 1.;
    cal.rotation[1] = 0.;
    cal.rotation[2] = 0.;
    cal.rotation[3] = 1.;
    cal.in_width = 640;
    cal.in_height = 480;
    cal.out_width = 640;
    cal.out_height = 480;
    cal.niter = 10;
    cal.maxerr = 1.e-6;
}

void filter_setup::tracker_config()
{
    track.spacing = 7;
    track.levels = 5;
    track.thresh = 0.1;
    track.iter = 25;
    track.eps = .3;
    track.blocksize = 5;
    track.harris = false;
    track.harrisk = .04;
    track.groupsize = 32;
    track.maxgroupsize = 40;
    track.maxfeats = 120;
    track.max_tracking_error = 1.0;
}

void filter_setup::filter_config()
{
    sfm.s.T.variance = 1.e-10;
    sfm.s.W.variance = v4(1., 1., 1.e-4, 0.);
    sfm.s.V.variance = 1.;
    sfm.s.w.variance = 1.;
    sfm.s.dw.variance = 10.;
    sfm.s.a.variance = 10.;
    sfm.s.da.variance = 10.;
    sfm.s.g.variance = 1.e-3 * 1.e-3;
    sfm.s.Wc.variance = 1.e-5 * 1.e-5;
    sfm.s.Tc.variance = 1.e-3 * 1.e-3;
    sfm.s.a_bias.v = v4(0.0367, -0.0112, -0.187, 0.);
    sfm.s.a_bias.variance = 1.e-2 * 1.e-2;
    sfm.s.w_bias.v = v4(0.0113, -0.0183, 0.0119, 0.);
    sfm.s.w_bias.variance = 1.e-3 * 1.e-3;

    sfm.init_vis_cov = 4.;
    sfm.max_add_vis_cov = 2.;
    sfm.min_add_vis_cov = .5;

    sfm.s.T.process_noise = 0.;
    sfm.s.W.process_noise = 0.;
    sfm.s.V.process_noise = 0.;
    sfm.s.w.process_noise = 0.;
    sfm.s.dw.process_noise = 1.e1 * 1.e1;
    sfm.s.a.process_noise = 0.;
    sfm.s.da.process_noise = 1.e1 * 1.e1;
    sfm.s.g.process_noise = 1.e-6 * 1.e-6;
    sfm.s.Wc.process_noise = 1.e-20;
    sfm.s.Tc.process_noise = 1.e-20;
    sfm.s.a_bias.process_noise = 1.e-20;
    sfm.s.w_bias.process_noise = 1.e-20;

    sfm.vis_ref_noise = 1.e-20;
    sfm.vis_noise = 1.e-20;

    sfm.vis_cov = 2. * 2.;
    sfm.w_variance = .000008;
    sfm.a_variance = .0002;

    sfm.min_feats_per_group = 3;
    sfm.min_group_add = 16;
    sfm.max_group_add = 40;
    sfm.max_features = 80;
    sfm.active = false;
    sfm.max_state_size = 256;
    sfm.frame = 0;
    sfm.skip = 1;
    sfm.min_group_health = 10.;
    sfm.max_feature_std_percent = .10;
    sfm.outlier_thresh = 1.5;
    sfm.outlier_reject = 10.;

    sfm.s.Tc.v = v4(-.0940, 0.0396, 0.0115, 0.);
    sfm.s.Wc.v = v4(.000808, .00355, -1.575, 0.);

    sfm.shutter_delay = 0;
    sfm.shutter_period = 31000;
    sfm.image_height = 480;
}

filter_setup::filter_setup(dispatch_t *input, const char *outfn): sfm(true)
{
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
    track.sink = &trackdata;
    init(&track);

    filter_config();
    sfm.calibration = &cal;
    sfm.track = &track;
    filter_init(&sfm);

    sfm.output = &solution;

    dispatch_addclient(input, &sfm, sfm_imu_measurement);
    dispatch_addclient(input, &sfm, sfm_accelerometer_measurement);
    dispatch_addclient(input, &sfm, sfm_gyroscope_measurement);
    dispatch_addclient(input, &sfm, sfm_image_measurement);
    dispatch_add_rewrite(input, packet_camera, 16667);
}

filter_setup::~filter_setup()
{
    delete trackdata.dispatch;
    delete solution.dispatch;
}
