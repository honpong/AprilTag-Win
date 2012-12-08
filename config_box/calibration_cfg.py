# Copyright (c) 2008-2012, Eagle Jones
# All rights reserved.
#
# This file is a part of the corvis framework, and is made available
# under the BSD license; please see LICENSE file for full text

sys.path.extend(['calibration'])
import calibration

cal = calibration.camera_calibration()
cal.F.x = 553.16888
cal.F.y = 551.75398
cal.out_F.x = 555.
cal.out_F.y = 555.
cal.C.x = 542.02475
cal.C.y = 375.65318
cal.out_C.x = cal.C.y
cal.out_C.y = cal.C.x
cal.alpha_c = -.00707
cal.p.x = -0.00003
cal.p.y =  0.00113
cal.K = [-0.35311, 0.14001, -0.02683]
cal.rotation = array([0., 1., -1., 0.])
cal.in_width = 640
cal.in_height = 768
cal.out_width = 768
cal.out_height = 640
cal.feature_sink = calibdata
cal.image_sink = calibdata
cal.niter = 10
cal.maxerr = 1.e-6
calibration.calibration_init(cal)
