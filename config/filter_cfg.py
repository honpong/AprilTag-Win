# Created by Eagle Jones
# Copyright (c) 2012. RealityCap, Inc.
# All Rights Reserved.

sys.path.extend(["filter/", "filter/.libs"])
import filter
sfm = filter.filter(True)
sfm.s.T.variance = 0.
sfm.s.W.variance = array([1., 1., 0., 0.])
sfm.s.V.variance = 0.
sfm.s.w.variance = 0.
sfm.s.dw.variance = 0.
sfm.s.a.variance = 0.
sfm.s.da.variance = 0.
sfm.s.g.variance = 1.e0**2
sfm.s.Wc.variance = 1.e-3**2
sfm.s.Tc.variance = 1.e-1**2
sfm.s.a_bias.v = array([0.0367, -0.0112, -0.187, 0.])
sfm.s.a_bias.variance = 4.e-3**2
sfm.s.w_bias.v = array([0.0113, -0.0183, 0.0119, 0.])
sfm.s.w_bias.variance = 0.

sfm.init_vis_cov = 1.e1**2

sfm.s.T.process_noise = 0.
sfm.s.W.process_noise = 0.
sfm.s.V.process_noise = 0.
sfm.s.w.process_noise = 0.
sfm.s.dw.process_noise = 1.e1**2
sfm.s.a.process_noise = 0.
sfm.s.da.process_noise = 1.e3**2
sfm.s.g.process_noise = 1.e-3**2
sfm.s.Wc.process_noise = 1.e-9**2
sfm.s.Tc.process_noise = 1.e-9**2
sfm.s.a_bias.process_noise = 3.3e-6**2
sfm.s.w_bias.process_noise = 0. #2.4e-8**2

sfm.vis_ref_noise = 1.e-7**2
sfm.vis_noise = 1.e-7**2

sfm.vis_cov = 5.e-3**2
sfm.w_variance = .000008 #measured at rest # 5.2e-4**2 #spec sheet for L3G4200D says .03 dps/sqrt(hz)
sfm.a_variance =  .0002 #measured at rest # 2.1e-3**2 #spec sheet for LIS331DLH says 218 micro-g / sqrt(hz)

sfm.min_feats_per_group = 3
sfm.min_group_add = 16
sfm.max_group_add = 40
sfm.max_features = 80
sfm.active = False
sfm.feature_count = 0
sfm.max_state_size = 256;
sfm.frame = 0
sfm.skip = 20
sfm.min_group_health = 10.
sfm.max_feature_std_percent = .25
sfm.outlier_thresh = 2.5
sfm.outlier_reject = 10.

sfm.s.Tc.v = array([ 0.047, 0.0092, 0., 0.])
sfm.s.Wc.v = array([.0017, .0031, -1.575, 0.])

cor.dispatch_add_rewrite(capturedispatch, cor.packet_camera, 15000) #i -think- based on initial tests that the frame timestamps indicate the start of the integration time, so add 15 ms to center it

filter.filter_init(sfm)
