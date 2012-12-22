# Copyright (c) 2008-2012, Eagle Jones
# All rights reserved.
#
# This file is a part of the corvis framework, and is made available
# under the BSD license; please see LICENSE file for full text

sys.path.extend(["filter/", "filter/.libs"])
import filter
sfm = filter.filter(False)
sfm.s.T.variance = 1.e-20
sfm.s.W.variance = array([1., 1., 1.e-4, 0.])
sfm.s.V.variance = 1.e-2
sfm.s.w.variance = 1.e-1
sfm.s.dw.variance = 1.e0
sfm.s.a.variance = 1.e-1
sfm.s.da.variance = 1.e0
sfm.s.g.variance = 1.e-3**2
sfm.s.Wc.variance = 1.e-4**2
sfm.s.Tc.variance = 1.e-3**2
sfm.s.a_bias.v = array([3.6e-3, -1.8e-3, -5.5e-3, 0.])
sfm.s.a_bias.variance = 4.e-3**2
sfm.s.w_bias.v = array([3.9e-5, 3.4e-5, 2.e-4, 0.])
sfm.s.w_bias.variance = 1.e-3**2 #9.7e-5**2

sfm.init_vis_cov = 1.e1**2

sfm.s.T.process_noise = 0.
sfm.s.W.process_noise = 0.
sfm.s.V.process_noise = 0.
sfm.s.w.process_noise = 0.
sfm.s.dw.process_noise = 1.e1**2
sfm.s.a.process_noise = 0.
sfm.s.da.process_noise = 1.e1**2
sfm.s.g.process_noise = 1.e-6**2
sfm.s.Wc.process_noise = 1.e-10**2
sfm.s.Tc.process_noise = 1.e-10**2
sfm.s.a_bias.process_noise = 1.e-10**2 #3.3e-6**2
sfm.s.w_bias.process_noise = 1.e-10**2 #0. #2.4e-8**2

sfm.vis_ref_noise = 1.e-10**2
sfm.vis_noise = 1.e-10**2

sfm.vis_cov = 5.e-3**2
sfm.w_variance = 4.0e-4**2
sfm.a_variance = 1.8e-1**2

sfm.min_feats_per_group = 3
sfm.min_group_add = 16
sfm.max_group_add = 40
sfm.max_features = 80
sfm.active = False
sfm.feature_count = 0
sfm.max_state_size = 256;
sfm.frame = 0
sfm.skip = 30
sfm.min_group_health = 10.
sfm.max_feature_std_percent = .25
sfm.outlier_thresh = 1.5
sfm.outlier_reject = 10.

sfm.s.Tc.v = array([ 0.138,  -0.043,  0.386, 0.])
sfm.s.Wc.v = array([-0.322, 2.051, -2.039, 0.])

#cor.dispatch_add_rewrite(capturedispatch, cor.packet_imu, 72000)

filter.filter_init(sfm)
