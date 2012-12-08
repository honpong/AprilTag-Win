# Copyright (c) 2008-2012, Eagle Jones
# All rights reserved.
#
# This file is a part of the corvis framework, and is made available
# under the BSD license; please see LICENSE file for full text

solution = cor.mapbuffer()
solution.mem_writable = True
solution.filename = replay_file + "_solution"
solution.file_writable = True
solution.size = 100*MB
solution.dispatch = cor.dispatch_t()
cor.plugins_register(cor.mapbuffer_open(solution))

sys.path.extend(["filter/", "filter/.libs"])
import filter
sfm = filter.filter()
sfm.s.T.variance = 0.
sfm.s.W.variance = array([1., 1., 0., 0.])
sfm.s.V.variance = 1.
sfm.s.w.variance = 1.
sfm.s.dw.variance = 1.
sfm.s.a.variance = 1.
sfm.s.da.variance = 1.
sfm.s.g.variance = 1.e0**2
sfm.s.Wc.variance = 1.e-2**2
sfm.s.Tc.variance = 1.e-1**2
sfm.s.a_bias.variance = 2.e-2**2
sfm.s.w_bias.variance = 0. #9.7e-5**2

sfm.s.a_bias.v = array([4.e-3, -1.7e-1, -3.e-3, 0.])
sfm.s.w_bias.v = array([-3.4e-3, -1.5e-3, -3.8e-3,  0.])

sfm.init_vis_cov = 1.e0**2

sfm.s.T.process_noise = 0.
sfm.s.W.process_noise = 0.
sfm.s.V.process_noise = 0.
sfm.s.w.process_noise = 0.
sfm.s.dw.process_noise = 1.e-0**2
sfm.s.a.process_noise = 0.
sfm.s.da.process_noise = 1.e2**2
sfm.s.g.process_noise = 1.e-4**2
sfm.s.Wc.process_noise = 1.e-10**2
sfm.s.Tc.process_noise = 1.e-10**2
sfm.s.a_bias.process_noise = 8.4e-5**2
sfm.s.w_bias.process_noise = 0. #1.6e-7**2

sfm.vis_ref_noise = 1.e-4**2
sfm.vis_noise = 1.e-4**2

sfm.vis_cov = 2.e-3**2
sfm.w_variance = 1.4e-3**2
sfm.a_variance = 5.25e-2**2

sfm.min_feats_per_group = 3
sfm.min_group_add = 16
sfm.max_group_add = 40
sfm.max_features = 180
sfm.active = False
sfm.feature_count = 0
sfm.max_state_size = 512;
sfm.frame = 0
sfm.skip = 1
sfm.min_group_health = 10.
sfm.max_feature_std_percent = .25
sfm.outlier_thresh = 4.
sfm.outlier_reject = 10.
sfm.output = solution

sfm.s.Wc.v = -array([1.1841, -1.1994, 1.2285, 0.])
sfm.s.Tc.v = array([3.e-2, 8.e-2, 8.5e-2, 0.])
#-array([.0195, -.0842, .0324, 0.])

cor.dispatch_add_rewrite(capturedispatch, cor.packet_imu, 50000)

filter.filter_init(sfm)

