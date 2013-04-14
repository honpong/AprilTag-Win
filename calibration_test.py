sys.path.extend(['calibration'])
import calibration

cal1 = calibration.camera_calibration();
cal2 = calibration.camera_calibration();

#sam iphone 5 back far focus
cal1.F.x = 584.48
cal1.F.y = 584.48
cal1.C.x = 319.5
cal1.C.y = 239.5
cal1.p.x = 0.
cal1.p.y = 0.
cal1.K[0] = 2.187e-2
cal1.K[1] = 5.115e-1
cal1.K[2] = -1.4472
cal1.in_width = 640
cal1.in_height = 480
cal1.invF.x = 1./ cal1.F.x
cal1.invF.y = 1./ cal1.F.y
cal1.niter = 10;
cal1.maxerr = 1.e-6

#sam iphone 5 back near focus
cal2.F.x = 596.21
cal2.F.y = 596.21
cal2.C.x = 319.5
cal2.C.y = 239.5
cal2.p.x = 0.
cal2.p.y =  0.
cal2.K[0] = -6.881e-3
cal2.K[1] = 7.0442e-1
cal2.K[2] = -1.95699
cal2.in_width = 640
cal2.in_height = 480
cal2.invF.x = 1./ cal2.F.x
cal2.invF.y = 1./ cal2.F.y
cal2.niter = 10;
cal2.maxerr = 1.e-6


#sam iphone 5 back auto 2
cal1.F.x = 582.1
cal1.F.y = 582.1
cal1.C.x = 319.5
cal1.C.y = 239.5
cal1.p.x = 0.
cal1.p.y = 0.
cal1.K = [-1.696e-3, 6.1548e-1, -1.6666]
cal1.in_width = 640
cal1.in_height = 480
cal1.invF.x = 1./ cal1.F.x
cal1.invF.y = 1./ cal1.F.y
cal1.niter = 10;
cal1.maxerr = 1.e-6

#sam iphone 5 back auto outlier
cal2.F.x = 589.23
cal2.F.y = 589.23
cal2.C.x = 319.5
cal2.C.y = 239.5
cal2.p.x = 0.
cal2.p.y =  0.
cal2.K[0] = -3.137e-2
cal2.K[1] = 7.69015e-1
cal2.K[2] = -1.86427
cal2.in_width = 640
cal2.in_height = 480
cal2.invF.x = 1./ cal2.F.x
cal2.invF.y = 1./ cal2.F.y
cal2.niter = 10;
cal2.maxerr = 1.e-6

#sam front 2
cal1.F.x = 624.38
cal1.F.y = 623.38
cal1.C.x = 319.4
cal1.C.y = 239.7
cal1.p.x = 1.052e-3
cal1.p.y = 1.609e-3
cal1.K = [-2.755e-2, 1.4224e-1, -7.077e-1]
cal1.in_width = 640
cal1.in_height = 480
cal1.invF.x = 1./ cal1.F.x
cal1.invF.y = 1./ cal1.F.y
cal1.niter = 10;
cal1.maxerr = 1.e-6

#jordan iphone 5 front try 3
cal2.F.x = 624.13
cal2.F.y = 624.07
cal2.C.x = 320.58
cal2.C.y = 239.27
cal2.p.x = -2.35e-3
cal2.p.y =  1.193e-3
cal2.K[0] = -7.213e-2
cal2.K[1] = 8.133e-1
cal2.K[2] = -3.176
cal2.in_width = 640
cal2.in_height = 480
cal2.invF.x = 1./ cal2.F.x
cal2.invF.y = 1./ cal2.F.y
cal2.niter = 10;
cal2.maxerr = 1.e-6

calibration.calibration_compare(cal1, cal2)
