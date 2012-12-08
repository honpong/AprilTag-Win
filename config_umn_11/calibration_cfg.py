# Copyright (c) 2008-2012, Eagle Jones
# All rights reserved.
#
# This file is a part of the corvis framework, and is made available
# under the BSD license; please see LICENSE file for full text

sys.path.extend(['calibration'])
import calibration

cal = calibration.camera_calibration()
cal.F.x = 531.38622
cal.F.y = 530.04069
cal.out_F.x = cal.F.x
cal.out_F.y = cal.F.y
cal.C.x = 336.15370
cal.C.y = 248.07873
cal.out_C.x = cal.C.x
cal.out_C.y = cal.C.y
cal.alpha_c = 0.
cal.p.x = 0.00012
cal.p.y = 0.00417
cal.K = [-0.16852, 0.13257, 0.] 
cal.rotation = array([1., 0., 0., 1.])
cal.in_width = 640
cal.in_height = 480
cal.out_width = 640
cal.out_height = 480
cal.feature_sink = calibdata
cal.image_sink = calibdata
cal.niter = 10
cal.maxerr = 1.e-6
calibration.calibration_init(cal)
