//
//  RCCalibration.m
//  RCCore
//
//  Created by Ben Hirashima on 4/9/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCCalibration.h"

#define KEY_FX @"DeviceCalibration_Fx"
#define KEY_FY @"DeviceCalibration_Fy"
#define KEY_CX @"DeviceCalibration_Cx"
#define KEY_CY @"DeviceCalibration_Cy"
#define KEY_PX @"DeviceCalibration_px"
#define KEY_PY @"DeviceCalibration_py"
#define KEY_K0 @"DeviceCalibration_K0"
#define KEY_K1 @"DeviceCalibration_K1"
#define KEY_K2 @"DeviceCalibration_K2"
#define KEY_ABIAS0 @"DeviceCalibration_abias0"
#define KEY_ABIAS1 @"DeviceCalibration_abias1"
#define KEY_ABIAS2 @"DeviceCalibration_abias2"
#define KEY_WBIAS0 @"DeviceCalibration_wbias0"
#define KEY_WBIAS1 @"DeviceCalibration_wbais1"
#define KEY_WBIAS2 @"DeviceCalibration_wbais2"
#define KEY_TC0 @"DeviceCalibration_Tc0"
#define KEY_TC1 @"DeviceCalibration_Tc1"
#define KEY_TC2 @"DeviceCalibration_Tc2"
#define KEY_WC0 @"DeviceCalibration_Wc0"
#define KEY_WC1 @"DeviceCalibration_Wc1"
#define KEY_WC2 @"DeviceCalibration_Wc2"
#define KEY_ABIASVAR0 @"DeviceCalibration_abiasvar0"
#define KEY_ABIASVAR1 @"DeviceCalibration_abiasvar1"
#define KEY_ABIASVAR2 @"DeviceCalibration_abiasvar2"
#define KEY_WBIASVAR0 @"DeviceCalibration_wbiasvar0"
#define KEY_WBIASVAR1 @"DeviceCalibration_wbiasvar1"
#define KEY_WBIASVAR2 @"DeviceCalibration_wbiasvar2"
#define KEY_TCVAR0 @"DeviceCalibration_TcVar0"
#define KEY_TCVAR1 @"DeviceCalibration_TcVar1"
#define KEY_TCVAR2 @"DeviceCalibration_TcVar2"
#define KEY_WCVAR0 @"DeviceCalibration_WcVar0"
#define KEY_WCVAR1 @"DeviceCalibration_WcVar1"
#define KEY_WCVAR2 @"DeviceCalibration_WcVar2"
#define KEY_WMEASVAR @"DeviceCalibration_wMeasVar"
#define KEY_AMEASVAR @"DeviceCalibration_aMeasVar"
#define KEY_IMAGE_WIDTH @"DeviceCalibration_imageWidth"
#define KEY_IMAGE_HEIGHT @"DeviceCalibration_imageHeight"
#define KEY_SHUTTER_DELAY @"DeviceCalibration_shutterDelay"
#define KEY_SHUTTER_PERIOD @"DeviceCalibration_shutterPeriod"

@implementation RCCalibration

+ (void) saveCalibrationData: (corvis_device_parameters)params
{
    NSLog(@"RCCalibration.saveCalibrationData");
    
    NSDictionary* data = [NSDictionary dictionaryWithObjectsAndKeys:
        [NSNumber numberWithFloat:params.Fx], KEY_FX,
        [NSNumber numberWithFloat:params.Fy], KEY_FY,
        [NSNumber numberWithFloat:params.Cx], KEY_CX,
        [NSNumber numberWithFloat:params.Cy], KEY_CY,
        [NSNumber numberWithFloat:params.px], KEY_PX,
        [NSNumber numberWithFloat:params.py], KEY_PY,
        [NSNumber numberWithFloat:params.K[0]], KEY_K0,
        [NSNumber numberWithFloat:params.K[1]], KEY_K1,
        [NSNumber numberWithFloat:params.K[2]], KEY_K2,
        [NSNumber numberWithFloat:params.a_bias[0]], KEY_ABIAS0,
        [NSNumber numberWithFloat:params.a_bias[1]], KEY_ABIAS1,
        [NSNumber numberWithFloat:params.a_bias[2]], KEY_ABIAS2,
        [NSNumber numberWithFloat:params.w_bias[0]], KEY_WBIAS0,
        [NSNumber numberWithFloat:params.w_bias[1]], KEY_WBIAS1,
        [NSNumber numberWithFloat:params.w_bias[2]], KEY_WBIAS2,
        [NSNumber numberWithFloat:params.Tc[0]], KEY_TC0,
        [NSNumber numberWithFloat:params.Tc[1]], KEY_TC1,
        [NSNumber numberWithFloat:params.Tc[2]], KEY_TC2,
        [NSNumber numberWithFloat:params.Wc[0]], KEY_WC0,
        [NSNumber numberWithFloat:params.Wc[1]], KEY_WC1,
        [NSNumber numberWithFloat:params.Wc[2]], KEY_WC2,
        [NSNumber numberWithFloat:params.a_bias_var[0]], KEY_ABIASVAR0,
        [NSNumber numberWithFloat:params.a_bias_var[1]], KEY_ABIASVAR1,
        [NSNumber numberWithFloat:params.a_bias_var[2]], KEY_ABIASVAR2,
        [NSNumber numberWithFloat:params.w_bias_var[0]], KEY_WBIASVAR0,
        [NSNumber numberWithFloat:params.w_bias_var[1]], KEY_WBIASVAR1,
        [NSNumber numberWithFloat:params.w_bias_var[2]], KEY_WBIASVAR2,
        [NSNumber numberWithFloat:params.Tc_var[0]], KEY_TCVAR0,
        [NSNumber numberWithFloat:params.Tc_var[1]], KEY_TCVAR1,
        [NSNumber numberWithFloat:params.Tc_var[2]], KEY_TCVAR2,
        [NSNumber numberWithFloat:params.Wc_var[0]], KEY_WCVAR0,
        [NSNumber numberWithFloat:params.Wc_var[1]], KEY_WCVAR1,
        [NSNumber numberWithFloat:params.Wc_var[2]], KEY_WCVAR2,
        [NSNumber numberWithFloat:params.w_meas_var], KEY_WMEASVAR,
        [NSNumber numberWithFloat:params.a_meas_var], KEY_AMEASVAR,
        [NSNumber numberWithInt:params.image_width], KEY_IMAGE_WIDTH,
        [NSNumber numberWithInt:params.image_height], KEY_IMAGE_HEIGHT,
        [NSNumber numberWithInt:params.shutter_delay], KEY_SHUTTER_DELAY,
        [NSNumber numberWithInt:params.shutter_period], KEY_SHUTTER_PERIOD,
        nil
    ];
    
    [[NSUserDefaults standardUserDefaults] setObject:data forKey:PREF_DEVICE_PARAMS];
    [[NSUserDefaults standardUserDefaults] synchronize];
}

+ (corvis_device_parameters) getCalibrationData
{
    NSLog(@"RCCalibration.getCalibrationData");
    
    corvis_device_parameters params;
    if ([RCCalibration copySavedCalibrationData:&params]) return params; //TODO: what if this app is restored from itunes on a different device?
   
    switch ([RCDeviceInfo getDeviceType]) {
        case DeviceTypeiPad3:
            return [self getDefaultsForiPad3];
            break;

        case DeviceTypeiPad2:
            return [self getDefaultsForiPad2];
            break;

        case DeviceTypeiPhone5:
            return [self getDefaultsForiPhone5];
            break;
            
        default:
            return [self getDefaultsForiPad3]; //temp
//            return nil; //production TODO: change
            break;
    }
}

+ (BOOL) copySavedCalibrationData:(struct corvis_device_parameters*)dc
{
    NSDictionary* data = [RCCalibration getCalibrationAsDictionary];
    if (data == nil) return NO;
    
    @try {
        dc->Fx = [((NSNumber*)[data objectForKey:KEY_FX]) floatValue];
        dc->Fy = [((NSNumber*)[data objectForKey:KEY_FY]) floatValue];
        dc->Cx = [((NSNumber*)[data objectForKey:KEY_CX]) floatValue];
        dc->Cy = [((NSNumber*)[data objectForKey:KEY_CY]) floatValue];
        dc->px = [((NSNumber*)[data objectForKey:KEY_PX]) floatValue];
        dc->py = [((NSNumber*)[data objectForKey:KEY_PY]) floatValue];
        dc->K[0] = [((NSNumber*)[data objectForKey:KEY_K0]) floatValue];
        dc->K[1] = [((NSNumber*)[data objectForKey:KEY_K1]) floatValue];
        dc->K[2] = [((NSNumber*)[data objectForKey:KEY_K2]) floatValue];
        dc->a_bias[0] = [((NSNumber*)[data objectForKey:KEY_ABIAS0]) floatValue];
        dc->a_bias[1] = [((NSNumber*)[data objectForKey:KEY_ABIAS1]) floatValue];
        dc->a_bias[2] = [((NSNumber*)[data objectForKey:KEY_ABIAS2]) floatValue];
        dc->w_bias[0] = [((NSNumber*)[data objectForKey:KEY_WBIAS0]) floatValue];
        dc->w_bias[1] = [((NSNumber*)[data objectForKey:KEY_WBIAS1]) floatValue];
        dc->w_bias[2] = [((NSNumber*)[data objectForKey:KEY_WBIAS2]) floatValue];
        dc->Tc[0] = [((NSNumber*)[data objectForKey:KEY_TC0]) floatValue];
        dc->Tc[1] = [((NSNumber*)[data objectForKey:KEY_TC1]) floatValue];
        dc->Tc[2] = [((NSNumber*)[data objectForKey:KEY_TC2]) floatValue];
        dc->Wc[0] = [((NSNumber*)[data objectForKey:KEY_WC0]) floatValue];
        dc->Wc[1] = [((NSNumber*)[data objectForKey:KEY_WC1]) floatValue];
        dc->Wc[2] = [((NSNumber*)[data objectForKey:KEY_WC2]) floatValue];
        dc->a_bias_var[0] = [((NSNumber*)[data objectForKey:KEY_ABIASVAR0]) floatValue];
        dc->a_bias_var[1] = [((NSNumber*)[data objectForKey:KEY_ABIASVAR1]) floatValue];
        dc->a_bias_var[2] = [((NSNumber*)[data objectForKey:KEY_ABIASVAR2]) floatValue];
        dc->w_bias_var[0] = [((NSNumber*)[data objectForKey:KEY_WBIASVAR0]) floatValue];
        dc->w_bias_var[1] = [((NSNumber*)[data objectForKey:KEY_WBIASVAR1]) floatValue];
        dc->w_bias_var[2] = [((NSNumber*)[data objectForKey:KEY_WBIASVAR2]) floatValue];
        dc->Tc_var[0] = [((NSNumber*)[data objectForKey:KEY_TCVAR0]) floatValue];
        dc->Tc_var[1] = [((NSNumber*)[data objectForKey:KEY_TCVAR1]) floatValue];
        dc->Tc_var[2] = [((NSNumber*)[data objectForKey:KEY_TCVAR2]) floatValue];
        dc->Wc_var[0] = [((NSNumber*)[data objectForKey:KEY_WCVAR0]) floatValue];
        dc->Wc_var[1] = [((NSNumber*)[data objectForKey:KEY_WCVAR1]) floatValue];
        dc->Wc_var[2] = [((NSNumber*)[data objectForKey:KEY_WCVAR2]) floatValue];
        dc->w_meas_var = [((NSNumber*)[data objectForKey:KEY_WMEASVAR]) floatValue];
        dc->a_meas_var = [((NSNumber*)[data objectForKey:KEY_AMEASVAR]) floatValue];
        dc->image_width = [((NSNumber*)[data objectForKey:KEY_IMAGE_WIDTH]) intValue];
        dc->image_height = [((NSNumber*)[data objectForKey:KEY_IMAGE_WIDTH]) intValue];
        dc->shutter_delay = [((NSNumber*)[data objectForKey:KEY_SHUTTER_DELAY]) intValue];
        dc->shutter_period = [((NSNumber*)[data objectForKey:KEY_SHUTTER_PERIOD]) intValue];
    }
    @catch (NSException *exception) {
        NSLog(@"Failed to get saved calibration data: %@", exception.debugDescription);
        return NO;
    }
        
    return YES;
}

+ (NSDictionary*) getCalibrationAsDictionary
{
    return [[NSUserDefaults standardUserDefaults] objectForKey:PREF_DEVICE_PARAMS];
}

+ (corvis_device_parameters) getDefaultsForiPhone5
{
    corvis_device_parameters dc;
    dc.Fx = 590.;
    dc.Fy = 590.;
    dc.Cx = 319.5;
    dc.Cy = 239.5;
    dc.px = 0.;
    dc.py = 0.;
    dc.K[0] = -0.06;
    dc.K[1] = 0.35;
    dc.K[2] = -0.70;
    dc.a_bias[0] = 0.;
    dc.a_bias[1] = 0.;
    dc.a_bias[2] = 0.;
    dc.w_bias[0] = 0.;
    dc.w_bias[1] = 0.;
    dc.w_bias[2] = 0.;
    dc.Tc[0] = 0.;
    dc.Tc[1] = 0.;
    dc.Tc[2] = 0.;
    dc.Wc[0] = sqrt(2.)/2. * M_PI;
    dc.Wc[1] = -sqrt(2.)/2. * M_PI;
    dc.Wc[2] = 0.;
    double a_bias_stdev = .02 * 9.8; //20 mg
    for(int i = 0; i < 3; ++i) dc.a_bias_var[i] = 1.e-4; //a_bias_stdev * a_bias_stdev;
    double w_bias_stdev = 10. / 180. * M_PI; //10 dps
    for(int i = 0; i < 3; ++i) dc.w_bias_var[i] = 1.e-4; //w_bias_stdev * w_bias_stdev;
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
    dc.shutter_period = 31000;
    return dc;
}

+ (corvis_device_parameters) getDefaultsForiPad2
{
    corvis_device_parameters dc;
    dc.Fx = 789.49;
    dc.Fy = 789.49;
    dc.Cx = 319.5;
    dc.Cy = 239.5;
    dc.px = 0.;
    dc.py = 0.;
    dc.K[0] = -1.2546e-1;
    dc.K[1] = 9.9923e-1;
    dc.K[2] = -2.9888;
    dc.a_bias[0] = 0.;
    dc.a_bias[1] = 0.;
    dc.a_bias[2] = 0.;
    dc.w_bias[0] = 0.;
    dc.w_bias[1] = 0.;
    dc.w_bias[2] = 0.;
    dc.Tc[0] = -.015;
    dc.Tc[1] = .100;
    dc.Tc[2] = 0.;
    dc.Wc[0] = sqrt(2.)/2. * M_PI;
    dc.Wc[1] = -sqrt(2.)/2. * M_PI;
    dc.Wc[2] = 0.;
    dc.a_bias_var[0] = 1.e-4;
    dc.a_bias_var[1] = 1.e-4;
    dc.a_bias_var[2] = 1.e-4;
    dc.w_bias_var[0] = 1.e-4;
    dc.w_bias_var[1] = 1.e-4;
    dc.w_bias_var[2] = 1.e-4;
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
    dc.shutter_period = 31000;
    return dc;
}

+ (corvis_device_parameters) getDefaultsForiPad3
{
    corvis_device_parameters dc;
    dc.Fx = 604.12;
    dc.Fy = 604.12;
    dc.Cx = 319.5;
    dc.Cy = 239.5;
    dc.px = 0.;
    dc.py = 0.;
    dc.K[0] = .2774956;
    dc.K[1] = -1.0795446;
    dc.K[2] = 1.14524733;
    dc.a_bias[0] = .0367;
    dc.a_bias[1] = -.0112;
    dc.a_bias[2] = -.187;
    dc.w_bias[0] = .0113;
    dc.w_bias[1] = -.0183;
    dc.w_bias[2] = .0119;
    dc.Tc[0] = 0.;
    dc.Tc[1] = .015;
    dc.Tc[2] = 0.;
    dc.Wc[0] = sqrt(2.)/2. * M_PI;
    dc.Wc[1] = -sqrt(2.)/2. * M_PI;
    dc.Wc[2] = 0.;
    double a_bias_stdev = .02 * 9.8; //20 mg
    for(int i = 0; i < 3; ++i) dc.a_bias_var[i] = 1.e-4; //a_bias_stdev * a_bias_stdev;
    double w_bias_stdev = 10. / 180. * M_PI; //10 dps
    for(int i = 0; i < 3; ++i) dc.w_bias_var[i] = 1.e-4; //w_bias_stdev * w_bias_stdev;
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
    dc.shutter_period = 31000;
    return dc;
}

@end
