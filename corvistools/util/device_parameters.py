from numpy import *
import sys
sys.path.extend(['../../'])
from corvis import filter

def set_initialized(dc):
    for i in range(3):
        #dc.a_bias[i] = 0.
        #dc.w_bias[i] = 0.
        dc.a_bias_var[i] = 1.e-4 #5.e-3
        dc.w_bias_var[i] = 1.e-4 #5.e-5

def set_device_parameters(dc, config_name):
    if config_name == 'iphone4s':
        filter.get_parameters_for_device(filter.DEVICE_TYPE_IPHONE4S, dc)

    elif config_name == 'iphone4s_camelia':
        filter.get_parameters_for_device(filter.DEVICE_TYPE_IPHONE4S, dc)
        dc.a_bias[0] = -0.1241252
        dc.a_bias[1] = 0.01688302
        dc.a_bias[2] = 0.3423786
        dc.w_bias[0] = -0.03693274
        dc.w_bias[1] = 0.004188713
        dc.w_bias[2] = -0.02701478
        set_initialized(dc)

    elif config_name == 'iphone4s_eagle': # still not estimated well
        #        dc.Tc[0] = 0.015;
        #dc.Tc[1] = 0.022;
        #dc.Tc[2] = 0.001;
        filter.get_parameters_for_device(filter.DEVICE_TYPE_IPHONE4S, dc)
        dc.a_bias[0] = -0.169
        dc.a_bias[1] = -0.072
        dc.a_bias[2] = 0.064
        dc.w_bias[0] = -0.018
        dc.w_bias[1] = 0.009
        dc.w_bias[2] = -0.018
        set_initialized(dc)


    elif config_name == 'iphone5':
        filter.get_parameters_for_device(filter.DEVICE_TYPE_IPHONE5, dc)

    elif config_name == 'iphone5_jordan':
        filter.get_parameters_for_device(filter.DEVICE_TYPE_IPHONE5, dc)
        dc.a_bias[0] = -0.00145;
        dc.a_bias[1] = -0.143;
        dc.a_bias[2] = 0.027;
        dc.w_bias[0] = 0.0036;
        dc.w_bias[1] = 0.0207;
        dc.w_bias[2] = -0.0437;
        set_initialized(dc)

    elif config_name == 'iphone5_sam':
        filter.get_parameters_for_device(filter.DEVICE_TYPE_IPHONE5, dc)
        dc.a_bias[0] = -0.127;
        dc.a_bias[1] = -0.139;
        dc.a_bias[2] = -0.003;
        dc.w_bias[0] = 0.005;
        dc.w_bias[1] = 0.007;
        dc.w_bias[2] = -0.003;
#set_initialized(dc)


    elif config_name == 'iphone5s':
        filter.get_parameters_for_device(filter.DEVICE_TYPE_IPHONE5S, dc)

    elif config_name == 'iphone5s_eagle':
        filter.get_parameters_for_device(filter.DEVICE_TYPE_IPHONE5S, dc)
        dc.a_bias[0] = 0.036;
        dc.a_bias[1] = -0.012;
        dc.a_bias[2] = -0.096;
        dc.w_bias[0] = 0.017;
        dc.w_bias[1] = 0.004;
        dc.w_bias[2] = -0.010;
        set_initialized(dc)

    elif config_name == 'iphone5s_sam':
        filter.get_parameters_for_device(filter.DEVICE_TYPE_IPHONE5S, dc)
        dc.a_bias[0] = 0.047;
        dc.a_bias[1] = 0.028;
        dc.a_bias[2] = -0.027;
        dc.w_bias[0] = 0.058;
        dc.w_bias[1] = -0.029;
        dc.w_bias[2] = -0.002;
        set_initialized(dc)

    elif config_name == 'iphone5s_brian':
        filter.get_parameters_for_device(filter.DEVICE_TYPE_IPHONE5S, dc)
        dc.a_bias[0] = -0.036;
        dc.a_bias[1] = 0.015;
        dc.a_bias[2] = -0.07;
        dc.w_bias[0] = 0.03;
        dc.w_bias[1] = 0.008;
        dc.w_bias[2] = -0.017;
        set_initialized(dc)


    elif config_name == 'ipad2':
        filter.get_parameters_for_device(filter.DEVICE_TYPE_IPAD2, dc)

    elif config_name == 'ipad2_ben':
        filter.get_parameters_for_device(filter.DEVICE_TYPE_IPAD2, dc)
        dc.a_bias[0] = -0.114
        dc.a_bias[1] = -0.171
        dc.a_bias[2] = 0.226
        dc.w_bias[0] = 0.007
        dc.w_bias[1] = 0.011
        dc.w_bias[2] = 0.010
        set_initialized(dc)

    elif config_name == 'ipad2_brian':
        filter.get_parameters_for_device(filter.DEVICE_TYPE_IPAD2, dc)
        dc.a_bias[0] = 0.1417
        dc.a_bias[1] = -0.0874
        dc.a_bias[2] = 0.2256
        dc.w_bias[0] = -0.0014
        dc.w_bias[1] = 0.0055
        dc.w_bias[2] = -0.0076
        set_initialized(dc)


    elif config_name == 'ipad3':
        filter.get_parameters_for_device(filter.DEVICE_TYPE_IPAD3, dc)

    elif config_name == 'ipad3_eagle':
        filter.get_parameters_for_device(filter.DEVICE_TYPE_IPAD3, dc)
        dc.a_bias[0] = 0.001;
        dc.a_bias[1] = -0.225;
        dc.a_bias[2] = -0.306;
        dc.w_bias[0] = 0.016;
        dc.w_bias[1] = -0.015;
        dc.w_bias[2] = 0.011;
        set_initialized(dc)


    elif config_name == 'ipad4':
        filter.get_parameters_for_device(filter.DEVICE_TYPE_IPAD4, dc)

    elif config_name == 'ipad4_jordan':
        filter.get_parameters_for_device(filter.DEVICE_TYPE_IPAD4, dc)
        dc.a_bias[0] = 0.089;
        dc.a_bias[1] = -0.176;
        dc.a_bias[2] = -0.129;
        dc.w_bias[0] = -0.023;
        dc.w_bias[1] = -0.003;
        dc.w_bias[2] = 0.015;
        set_initialized(dc)


    elif config_name == 'ipadair':
        filter.get_parameters_for_device(filter.DEVICE_TYPE_IPADAIR, dc)

    elif config_name == 'ipadair_eagle':
        filter.get_parameters_for_device(filter.DEVICE_TYPE_IPADAIR, dc)
        dc.a_bias[0] = 0.002;
        dc.a_bias[1] = -0.0013;
        dc.a_bias[2] = 0.07;
        dc.w_bias[0] = 0.012;
        dc.w_bias[1] = 0.017;
        dc.w_bias[2] = 0.004;
        set_initialized(dc)


    elif config_name == 'ipadmini':
        filter.get_parameters_for_device(filter.DEVICE_TYPE_IPADMINI, dc)

    elif config_name == 'ipadmini_ben':
        filter.get_parameters_for_device(filter.DEVICE_TYPE_IPADMINI, dc)
        dc.a_bias[0] = 0.075
        dc.a_bias[1] = 0.07
        dc.a_bias[2] = 0.015
        dc.w_bias[0] = 0.004
        dc.w_bias[1] = -0.044
        dc.w_bias[2] = .0088
        set_initialized(dc)


    elif config_name == 'ipadminiretina':
        filter.get_parameters_for_device(filter.DEVICE_TYPE_IPADMINIRETINA, dc)

    elif config_name == 'ipadminiretina_ben':
        filter.get_parameters_for_device(filter.DEVICE_TYPE_IPADMINIRETINA, dc)
        dc.a_bias[0] = 0.108
        dc.a_bias[1] = -.082
        dc.a_bias[2] = 0.006
        dc.w_bias[0] = 0.013
        dc.w_bias[1] = -0.015
        dc.w_bias[2] = -0.026
        set_initialized(dc)


    elif config_name == 'ipod5' or config_name == 'ipodtouch':
        filter.get_parameters_for_device(filter.DEVICE_TYPE_IPOD5, dc)

    elif config_name == 'ipodtouch_ben':
        filter.get_parameters_for_device(filter.DEVICE_TYPE_IPOD5, dc)
        dc.a_bias[0] = -0.092
        dc.a_bias[1] = 0.053
        dc.a_bias[2] = 0.104
        dc.w_bias[0] = 0.021
        dc.w_bias[1] = 0.021
        dc.w_bias[2] = -0.013
        set_initialized(dc)


    elif config_name == 'ipad3-front':
        #parameters for generic ipad 3 - front cam
        dc.Fx = 604.;
        dc.Fy = 604.;
        dc.Cx = 319.5;
        dc.Cy = 239.5;
        dc.px = 0.
        dc.py = 0.
        dc.K[0] = 0.; #.2774956;
        dc.K[1] = 0.; #-1.0795446;
        dc.K[2] = 0.; #1.14524733;
        dc.a_bias[0] = 0.;
        dc.a_bias[1] = 0.;
        dc.a_bias[2] = 0.;
        dc.w_bias[0] = 0.;
        dc.w_bias[1] = 0.;
        dc.w_bias[2] = 0.;
        dc.Tc[0] = 0.#-.0940;
        dc.Tc[1] = 0.#.0396;
        dc.Tc[2] = 0.#.0115;
        dc.Wc[0] = 0.;
        dc.Wc[1] = 0.;
        dc.Wc[2] = -pi/2.;
        #dc.a_bias_var[0] = 1.e-4;
        #dc.a_bias_var[1] = 1.e-4;
        #dc.a_bias_var[2] = 1.e-4;
        #dc.w_bias_var[0] = 1.e-7;
        #dc.w_bias_var[1] = 1.e-7;
        #dc.w_bias_var[2] = 1.e-7;
        a_bias_stdev = .02 * 9.8; #20 mg
        dc.a_bias_var = a_bias_stdev * a_bias_stdev;
        w_bias_stdev = 10. / 180. * pi; #10 dps
        dc.w_bias_var = w_bias_stdev * w_bias_stdev;
        dc.Tc_var[0] = 1.e-6;
        dc.Tc_var[1] = 1.e-6;
        dc.Tc_var[2] = 1.e-6;
        dc.Wc_var[0] = 1.e-7;
        dc.Wc_var[1] = 1.e-7;
        dc.Wc_var[2] = 1.e-7;
        w_stdev = .03 * sqrt(50.) / 180. * pi; #.03 dps / sqrt(hz) at 50 hz
        dc.w_meas_var = w_stdev * w_stdev;
        a_stdev = .000218 * sqrt(50.) * 9.8; #218 ug / sqrt(hz) at 50 hz
        dc.a_meas_var = a_stdev * a_stdev;
        dc.image_width = 640;
        dc.image_height = 480;
        dc.shutter_delay = 0;
        dc.shutter_period = 31000;

    elif config_name == 'nexus7-front':
        #parameters for sam's nexus 7 - front cam
        dc.Fx = 604.;
        dc.Fy = 604.;
        dc.Cx = 319.5;
        dc.Cy = 239.5;
        dc.px = 0.
        dc.py = 0.
        dc.K[0] = 0.; #.2774956;
        dc.K[1] = 0.; #-1.0795446;
        dc.K[2] = 0.; #1.14524733;
        dc.a_bias[0] = 0.126;
        dc.a_bias[1] = 0.024;
        dc.a_bias[2] = 0.110;
        dc.w_bias[0] = 1.e-5;
        dc.w_bias[1] = -3.e-6;
        dc.w_bias[2] = -8.e-6;
        dc.Tc[0] = 0.#-.0940;
        dc.Tc[1] = 0.#.0396;
        dc.Tc[2] = 0.#.0115;
        dc.Wc[0] = 0.;
        dc.Wc[1] = 0.;
        dc.Wc[2] = -pi/2.;
        #dc.a_bias_var[0] = 1.e-4;
        #dc.a_bias_var[1] = 1.e-4;
        #dc.a_bias_var[2] = 1.e-4;
        #dc.w_bias_var[0] = 1.e-7;
        #dc.w_bias_var[1] = 1.e-7;
        #dc.w_bias_var[2] = 1.e-7;
        a_bias_stdev = .02 * 9.8; #20 mg
        dc.a_bias_var = a_bias_stdev * a_bias_stdev;
        w_bias_stdev = 10. / 180. * pi; #10 dps
        dc.w_bias_var = w_bias_stdev * w_bias_stdev;
        dc.Tc_var[0] = 1.e-6;
        dc.Tc_var[1] = 1.e-6;
        dc.Tc_var[2] = 1.e-6;
        dc.Wc_var[0] = 1.e-7;
        dc.Wc_var[1] = 1.e-7;
        dc.Wc_var[2] = 1.e-7;
        w_stdev = .03 * sqrt(50.) / 180. * pi; #.03 dps / sqrt(hz) at 50 hz
        dc.w_meas_var = 1.e-3 * 1.e-3 #w_stdev * w_stdev;
        a_stdev = .000218 * sqrt(50.) * 9.8; #218 ug / sqrt(hz) at 50 hz
        dc.a_meas_var = 2.e-2 * 2.e-2 #a_stdev * a_stdev;
        dc.image_width = 640;
        dc.image_height = 480;
        dc.shutter_delay = 0;
        dc.shutter_period = 31000;

    elif config_name == 'simulator':
        dc.Fx = 585.;
        dc.Fy = 585.;
        dc.Cx = 319.5;
        dc.Cy = 239.5;
        dc.px = 0.;
        dc.py = 0.;
        dc.K[0] = .10;
        dc.K[1] = -.10;
        dc.K[2] = 0.;
        dc.Tc[0] = 0.000;
        dc.Tc[1] = 0.000;
        dc.Tc[2] = -0.008;
        dc.Wc[0] = sqrt(2.)/2. * pi;
        dc.Wc[1] = -sqrt(2.)/2. * pi;
        dc.Wc[2] = 0.;
        a_bias_stdev = .02 * 9.8; # 20 mg
        w_bias_stdev = 10. / 180. * pi; #10 dps
        for i in range(3):
            dc.a_bias[i] = 0.;
            dc.w_bias[i] = 0.;
            dc.a_bias_var[i] = 1.e-4; #a_bias_stdev * a_bias_stdev;
            dc.w_bias_var[i] = 1.e-4; #w_bias_stdev * w_bias_stdev;
            dc.Tc_var[i] = 1.e-7;
            dc.Wc_var[i] = 1.e-7;
        w_stdev = .03 * sqrt(50.) / 180. * pi; #.03 dps / sqrt(hz) at 50 hz
        dc.w_meas_var = w_stdev * w_stdev;
        a_stdev = .000218 * sqrt(50.) * 9.8; # 218 ug / sqrt(hz) at 50 hz
        dc.a_meas_var = a_stdev * a_stdev;
        dc.image_width = 640;
        dc.image_height = 480;
        dc.shutter_delay = 0;
        dc.shutter_period = 31000;

    elif config_name == 'intel':
        dc.Fx = 577.
        dc.Fy = 577.
        dc.Cx = 319.5;
        dc.Cy = 239.5;
        dc.px = 0.;
        dc.py = 0.;
        dc.K[0] = .14
        dc.K[1] = -.27
        dc.K[2] = 0.;
        dc.Tc[0] = 0.008;
        dc.Tc[1] = 0.002;
        dc.Tc[2] = 0.;
        dc.Wc[0] = sqrt(2.)/2. * pi;
        dc.Wc[1] = -sqrt(2.)/2. * pi;
        dc.Wc[2] = 0.;
        dc.a_bias[0] = 0.0 #0.10
        dc.a_bias[1] = .20 #-0.09
        dc.a_bias[2] = -.30  #-.6
        dc.w_bias[0] = .021 #.034 #-.029
        dc.w_bias[1] = -.005 #-.001 #.012
        dc.w_bias[2] = .030 #.004 #.014
        a_bias_stdev = 0.02 * 9.8; # 20 mg
        w_bias_stdev = 10. / 180. * pi; # 10 dps
        for i in range(3):
            dc.a_bias_var[i] = 1.e-4 #a_bias_stdev * a_bias_stdev;
            dc.w_bias_var[i] = 1.e-6 #w_bias_stdev * w_bias_stdev;
            dc.Tc_var[i] = 1.e-6;
            dc.Wc_var[i] = 1.e-7;
        w_stdev = 3.e-2 #.03 * sqrt(50.) / 180. * pi; #.03 dps / sqrt(hz) at 50 hz
        dc.w_meas_var = w_stdev * w_stdev;
        a_stdev = 3.e-2 #.000220 * sqrt(50.) * 9.8; # 220 ug / sqrt(hz) at 50 hz
        dc.a_meas_var = a_stdev * a_stdev;
        dc.image_width = 640;
        dc.image_height = 480;
        dc.shutter_delay = 0;
        dc.shutter_period = 31000;

    elif config_name == 'intel-blank':
        dc.Fx = 580.;
        dc.Fy = 580.;
        dc.Cx = 319.5;
        dc.Cy = 239.5;
        dc.px = 0.;
        dc.py = 0.;
        dc.K[0] = .16;
        dc.K[1] = -.29;
        dc.K[2] = 0.;
        dc.Tc[0] = 0.008;
        dc.Tc[1] = 0.002;
        dc.Tc[2] = 0.;
        dc.Wc[0] = sqrt(2.)/2. * pi;
        dc.Wc[1] = -sqrt(2.)/2. * pi;
        dc.Wc[2] = 0.;
        dc.a_bias[0] = 0
        dc.a_bias[1] = 0
        dc.a_bias[2] = 0
        dc.w_bias[0] = 0
        dc.w_bias[1] = 0
        dc.w_bias[2] = 0
        a_bias_stdev = 0.02 * 9.8; # 20 mg
        w_bias_stdev = 10. / 180. * pi; # 10 dps
        for i in range(3):
            dc.a_bias_var[i] = a_bias_stdev * a_bias_stdev;
            dc.w_bias_var[i] = w_bias_stdev * w_bias_stdev;
            dc.Tc_var[i] = 1.e-6;
            dc.Wc_var[i] = 1.e-7;
        w_stdev = 3.e-2 #.03 * sqrt(50.) / 180. * pi; #.03 dps / sqrt(hz) at 50 hz
        dc.w_meas_var = w_stdev * w_stdev;
        a_stdev = 3.e-2 #.000220 * sqrt(50.) * 9.8; # 220 ug / sqrt(hz) at 50 hz
        dc.a_meas_var = a_stdev * a_stdev;
        dc.image_width = 640;
        dc.image_height = 480;
        dc.shutter_delay = 0;
        dc.shutter_period = 31000;


    elif config_name == 'intel-old': #good for 11-21-12-31-18
        dc.Fx = 610.;
        dc.Fy = 610.;
        dc.Cx = 319.5;
        dc.Cy = 239.5;
        dc.px = 0.;
        dc.py = 0.;
        dc.K[0] = .20;
        dc.K[1] = -.55;
        dc.K[2] = 0.;
        dc.Tc[0] = 0.008;
        dc.Tc[1] = 0.002;
        dc.Tc[2] = 0.;
        dc.Wc[0] = sqrt(2.)/2. * pi;
        dc.Wc[1] = -sqrt(2.)/2. * pi;
        dc.Wc[2] = 0.;
        dc.a_bias[0] = 0.10
        dc.a_bias[1] = -0.09
        dc.a_bias[2] = -.6
        dc.w_bias[0] = .034 #-.029
        dc.w_bias[1] = -.001 #.012
        dc.w_bias[2] = .004 #.014
        a_bias_stdev = 0.02 * 9.8; # 20 mg
        w_bias_stdev = 10. / 180. * pi; # 10 dps
        for i in range(3):
            dc.a_bias_var[i] = 1.e-4 #a_bias_stdev * a_bias_stdev;
            dc.w_bias_var[i] = 1.e-4 #w_bias_stdev * w_bias_stdev;
            dc.Tc_var[i] = 1.e-6;
            dc.Wc_var[i] = 1.e-7;
        w_stdev = 3.e-2 #.03 * sqrt(50.) / 180. * pi; #.03 dps / sqrt(hz) at 50 hz
        dc.w_meas_var = w_stdev * w_stdev;
        a_stdev = 3.e-2 #.000220 * sqrt(50.) * 9.8; # 220 ug / sqrt(hz) at 50 hz
        dc.a_meas_var = a_stdev * a_stdev;
        dc.image_width = 640;
        dc.image_height = 480;
        dc.shutter_delay = 0;
        dc.shutter_period = 31000;

    else:
       print "WARNING: Unrecognized configuration"
