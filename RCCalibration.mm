//
//  RCCalibration.m
//  RCCore
//
//  Created by Ben Hirashima on 4/9/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCCalibration.h"

@implementation RCCalibration

+ (corvis_device_parameters) getCalibrationData
{
    NSValue* params = [[NSUserDefaults standardUserDefaults] objectForKey:PREF_DEVICE_PARAMS];
    if (params) return [params getDeviceParams]; //TODO: what if this app is restored from itunes on a different device?
    
    switch ([RCDeviceInfo getDeviceType]) {
        case DeviceTypeiPad3:
            return [self getDefaultsForiPad3];
            break;
            
        default:
            return [self getDefaultsForiPad3]; //temp
//            return nil; //production
            break;
    }
}

+ (corvis_device_parameters) getDefaultsForiPad3
{
    corvis_device_parameters dc;
    //parameters for eagle's ipad 3
    dc.Fx = 604.241;
    dc.Fy = 604.028;
    dc.Cx = 317.576;
    dc.Cy = 237.755;
    dc.px = -2.65791e-3;
    dc.py = 6.48762e-4;
    dc.K[0] = .2774956;
    dc.K[1] = -1.0795446;
    dc.K[2] = 1.14524733;
    dc.a_bias[0] = .0367;
    dc.a_bias[1] = -.0112;
    dc.a_bias[2] = -.187;
    dc.w_bias[0] = .0113;
    dc.w_bias[1] = -.0183;
    dc.w_bias[2] = .0119;
    dc.Tc[0] = -.0940;
    dc.Tc[1] = .0396;
    dc.Tc[2] = .0115;
    dc.Wc[0] = .000808;
    dc.Wc[1] = .00355;
    dc.Wc[2] = -1.575;
    dc.a_bias_var[0] = 1.e-4;
    dc.a_bias_var[1] = 1.e-4;
    dc.a_bias_var[2] = 1.e-4;
    dc.w_bias_var[0] = 1.e-7;
    dc.w_bias_var[1] = 1.e-7;
    dc.w_bias_var[2] = 1.e-7;
    dc.Tc_var[0] = 1.e-6;
    dc.Tc_var[1] = 1.e-6;
    dc.Tc_var[2] = 1.e-6;
    dc.Wc_var[0] = 1.e-7;
    dc.Wc_var[1] = 1.e-7;
    dc.Wc_var[2] = 1.e-7;
    float w_stdev = .03 * sqrt(50.) / 180. * M_PI; //.03 dps / sqrt(hz) at 50 hz
    dc.w_meas_var = w_stdev * w_stdev;
    float a_stdev = .000218 * sqrt(50.) * 9.8; //218 ug / sqrt(hz) at 50 hz
    dc.a_meas_var = a_stdev * a_stdev;
    dc.image_width = 640;
    dc.image_height = 480;
    dc.shutter_delay = 0;
    dc. shutter_period = 31000;
    return dc;
}

@end
