# Created by Eagle Jones
# Copyright (c) 2012. RealityCap, Inc.
# All Rights Reserved.

sys.path.extend(['calibration'])
import calibration

cal = calibration.camera_calibration()
cal.F.x = 604.241 #610.03326
cal.F.y = 604.028 #609.15142
cal.out_F.x = cal.F.x
cal.out_F.y = cal.F.y
cal.C.x = 317.576
cal.C.y = 237.755
cal.out_C.x = cal.C.x
cal.out_C.y = cal.C.y
cal.alpha_c = 0.
cal.p.x = -2.65791e-3 #-1.4990584e-3
cal.p.y = 6.48762e-4 #6.8647908e-4
cal.K = [.2774956, -1.0795446, 1.14524733 ] #[ .30716515, -.13639028, 1.931391] 
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
