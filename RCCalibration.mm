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
    
    [[NSUserDefaults standardUserDefaults] setObject:[NSNumber numberWithFloat:params.Fx] forKey:KEY_FX];
    [[NSUserDefaults standardUserDefaults] setObject:[NSNumber numberWithFloat:params.Fy] forKey:KEY_FY];
    [[NSUserDefaults standardUserDefaults] setObject:[NSNumber numberWithFloat:params.Cx] forKey:KEY_CX];
    [[NSUserDefaults standardUserDefaults] setObject:[NSNumber numberWithFloat:params.Cy] forKey:KEY_CY];
    [[NSUserDefaults standardUserDefaults] setObject:[NSNumber numberWithFloat:params.px] forKey:KEY_PX];
    [[NSUserDefaults standardUserDefaults] setObject:[NSNumber numberWithFloat:params.py] forKey:KEY_PY];
    [[NSUserDefaults standardUserDefaults] setObject:[NSNumber numberWithFloat:params.K[0]] forKey:KEY_K0];
    [[NSUserDefaults standardUserDefaults] setObject:[NSNumber numberWithFloat:params.K[1]] forKey:KEY_K1];
    [[NSUserDefaults standardUserDefaults] setObject:[NSNumber numberWithFloat:params.K[2]] forKey:KEY_K2];
    [[NSUserDefaults standardUserDefaults] setObject:[NSNumber numberWithFloat:params.a_bias[0]] forKey:KEY_ABIAS0];
    [[NSUserDefaults standardUserDefaults] setObject:[NSNumber numberWithFloat:params.a_bias[1]] forKey:KEY_ABIAS1];
    [[NSUserDefaults standardUserDefaults] setObject:[NSNumber numberWithFloat:params.a_bias[2]] forKey:KEY_ABIAS2];
    [[NSUserDefaults standardUserDefaults] setObject:[NSNumber numberWithFloat:params.w_bias[0]] forKey:KEY_WBIAS0];
    [[NSUserDefaults standardUserDefaults] setObject:[NSNumber numberWithFloat:params.w_bias[1]] forKey:KEY_WBIAS1];
    [[NSUserDefaults standardUserDefaults] setObject:[NSNumber numberWithFloat:params.w_bias[2]] forKey:KEY_WBIAS2];
    [[NSUserDefaults standardUserDefaults] setObject:[NSNumber numberWithFloat:params.Tc[0]] forKey:KEY_TC0];
    [[NSUserDefaults standardUserDefaults] setObject:[NSNumber numberWithFloat:params.Tc[1]] forKey:KEY_TC1];
    [[NSUserDefaults standardUserDefaults] setObject:[NSNumber numberWithFloat:params.Tc[2]] forKey:KEY_TC2];
    [[NSUserDefaults standardUserDefaults] setObject:[NSNumber numberWithFloat:params.Wc[0]] forKey:KEY_WC0];
    [[NSUserDefaults standardUserDefaults] setObject:[NSNumber numberWithFloat:params.Wc[1]] forKey:KEY_WC1];
    [[NSUserDefaults standardUserDefaults] setObject:[NSNumber numberWithFloat:params.Wc[2]] forKey:KEY_WC2];
    [[NSUserDefaults standardUserDefaults] setObject:[NSNumber numberWithFloat:params.a_bias_var[0]] forKey:KEY_ABIASVAR0];
    [[NSUserDefaults standardUserDefaults] setObject:[NSNumber numberWithFloat:params.a_bias_var[1]] forKey:KEY_ABIASVAR1];
    [[NSUserDefaults standardUserDefaults] setObject:[NSNumber numberWithFloat:params.a_bias_var[2]] forKey:KEY_ABIASVAR2];
    [[NSUserDefaults standardUserDefaults] setObject:[NSNumber numberWithFloat:params.w_bias_var[0]] forKey:KEY_WBIASVAR0];
    [[NSUserDefaults standardUserDefaults] setObject:[NSNumber numberWithFloat:params.w_bias_var[1]] forKey:KEY_WBIASVAR1];
    [[NSUserDefaults standardUserDefaults] setObject:[NSNumber numberWithFloat:params.w_bias_var[2]] forKey:KEY_WBIASVAR2];
    [[NSUserDefaults standardUserDefaults] setObject:[NSNumber numberWithFloat:params.Tc_var[0]] forKey:KEY_TCVAR0];
    [[NSUserDefaults standardUserDefaults] setObject:[NSNumber numberWithFloat:params.Tc_var[1]] forKey:KEY_TCVAR1];
    [[NSUserDefaults standardUserDefaults] setObject:[NSNumber numberWithFloat:params.Tc_var[2]] forKey:KEY_TCVAR2];
    [[NSUserDefaults standardUserDefaults] setObject:[NSNumber numberWithFloat:params.Wc_var[0]] forKey:KEY_WCVAR0];
    [[NSUserDefaults standardUserDefaults] setObject:[NSNumber numberWithFloat:params.Wc_var[1]] forKey:KEY_WCVAR1];
    [[NSUserDefaults standardUserDefaults] setObject:[NSNumber numberWithFloat:params.Wc_var[2]] forKey:KEY_WCVAR2];
    [[NSUserDefaults standardUserDefaults] setObject:[NSNumber numberWithFloat:params.w_meas_var] forKey:KEY_WMEASVAR];
    [[NSUserDefaults standardUserDefaults] setObject:[NSNumber numberWithFloat:params.a_meas_var] forKey:KEY_AMEASVAR];
    [[NSUserDefaults standardUserDefaults] setObject:[NSNumber numberWithInt:params.image_width] forKey:KEY_IMAGE_WIDTH];
    [[NSUserDefaults standardUserDefaults] setObject:[NSNumber numberWithInt:params.image_height] forKey:KEY_IMAGE_HEIGHT];
    [[NSUserDefaults standardUserDefaults] setObject:[NSNumber numberWithInt:params.shutter_delay] forKey:KEY_SHUTTER_DELAY];
    [[NSUserDefaults standardUserDefaults] setObject:[NSNumber numberWithInt:params.shutter_period] forKey:KEY_SHUTTER_PERIOD];
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
            
        default:
            return [self getDefaultsForiPad3]; //temp
//            return nil; //production
            break;
    }
}

+ (BOOL) copySavedCalibrationData:(struct corvis_device_parameters*)dc
{
    if ([[NSUserDefaults standardUserDefaults] objectForKey:KEY_FX] == nil) return NO; //assume that if one doesn't exist, the others don't either
    
    @try {
        dc->Fx = [((NSNumber*)[[NSUserDefaults standardUserDefaults] objectForKey:KEY_FX]) floatValue];
        dc->Fy = [((NSNumber*)[[NSUserDefaults standardUserDefaults] objectForKey:KEY_FY]) floatValue];
        dc->Cx = [((NSNumber*)[[NSUserDefaults standardUserDefaults] objectForKey:KEY_CX]) floatValue];
        dc->Cy = [((NSNumber*)[[NSUserDefaults standardUserDefaults] objectForKey:KEY_CY]) floatValue];
        dc->px = [((NSNumber*)[[NSUserDefaults standardUserDefaults] objectForKey:KEY_PX]) floatValue];
        dc->py = [((NSNumber*)[[NSUserDefaults standardUserDefaults] objectForKey:KEY_PY]) floatValue];
        dc->K[0] = [((NSNumber*)[[NSUserDefaults standardUserDefaults] objectForKey:KEY_K0]) floatValue];
        dc->K[1] = [((NSNumber*)[[NSUserDefaults standardUserDefaults] objectForKey:KEY_K1]) floatValue];
        dc->K[2] = [((NSNumber*)[[NSUserDefaults standardUserDefaults] objectForKey:KEY_K2]) floatValue];
        dc->a_bias[0] = [((NSNumber*)[[NSUserDefaults standardUserDefaults] objectForKey:KEY_ABIAS0]) floatValue];
        dc->a_bias[1] = [((NSNumber*)[[NSUserDefaults standardUserDefaults] objectForKey:KEY_ABIAS1]) floatValue];
        dc->a_bias[2] = [((NSNumber*)[[NSUserDefaults standardUserDefaults] objectForKey:KEY_ABIAS2]) floatValue];
        dc->w_bias[0] = [((NSNumber*)[[NSUserDefaults standardUserDefaults] objectForKey:KEY_WBIAS0]) floatValue];
        dc->w_bias[1] = [((NSNumber*)[[NSUserDefaults standardUserDefaults] objectForKey:KEY_WBIAS1]) floatValue];
        dc->w_bias[2] = [((NSNumber*)[[NSUserDefaults standardUserDefaults] objectForKey:KEY_WBIAS2]) floatValue];
        dc->Tc[0] = [((NSNumber*)[[NSUserDefaults standardUserDefaults] objectForKey:KEY_TC0]) floatValue];
        dc->Tc[1] = [((NSNumber*)[[NSUserDefaults standardUserDefaults] objectForKey:KEY_TC1]) floatValue];
        dc->Tc[2] = [((NSNumber*)[[NSUserDefaults standardUserDefaults] objectForKey:KEY_TC2]) floatValue];
        dc->Wc[0] = [((NSNumber*)[[NSUserDefaults standardUserDefaults] objectForKey:KEY_WC0]) floatValue];
        dc->Wc[1] = [((NSNumber*)[[NSUserDefaults standardUserDefaults] objectForKey:KEY_WC1]) floatValue];
        dc->Wc[2] = [((NSNumber*)[[NSUserDefaults standardUserDefaults] objectForKey:KEY_WC2]) floatValue];
        dc->a_bias_var[0] = [((NSNumber*)[[NSUserDefaults standardUserDefaults] objectForKey:KEY_ABIASVAR0]) floatValue];
        dc->a_bias_var[1] = [((NSNumber*)[[NSUserDefaults standardUserDefaults] objectForKey:KEY_ABIASVAR1]) floatValue];
        dc->a_bias_var[2] = [((NSNumber*)[[NSUserDefaults standardUserDefaults] objectForKey:KEY_ABIASVAR2]) floatValue];
        dc->w_bias_var[0] = [((NSNumber*)[[NSUserDefaults standardUserDefaults] objectForKey:KEY_WBIASVAR0]) floatValue];
        dc->w_bias_var[1] = [((NSNumber*)[[NSUserDefaults standardUserDefaults] objectForKey:KEY_WBIASVAR1]) floatValue];
        dc->w_bias_var[2] = [((NSNumber*)[[NSUserDefaults standardUserDefaults] objectForKey:KEY_WBIASVAR2]) floatValue];
        dc->Tc_var[0] = [((NSNumber*)[[NSUserDefaults standardUserDefaults] objectForKey:KEY_TCVAR0]) floatValue];
        dc->Tc_var[1] = [((NSNumber*)[[NSUserDefaults standardUserDefaults] objectForKey:KEY_TCVAR1]) floatValue];
        dc->Tc_var[2] = [((NSNumber*)[[NSUserDefaults standardUserDefaults] objectForKey:KEY_TCVAR2]) floatValue];
        dc->Wc_var[0] = [((NSNumber*)[[NSUserDefaults standardUserDefaults] objectForKey:KEY_WCVAR0]) floatValue];
        dc->Wc_var[1] = [((NSNumber*)[[NSUserDefaults standardUserDefaults] objectForKey:KEY_WCVAR1]) floatValue];
        dc->Wc_var[2] = [((NSNumber*)[[NSUserDefaults standardUserDefaults] objectForKey:KEY_WCVAR2]) floatValue];
        dc->w_meas_var = [((NSNumber*)[[NSUserDefaults standardUserDefaults] objectForKey:KEY_WMEASVAR]) floatValue];
        dc->a_meas_var = [((NSNumber*)[[NSUserDefaults standardUserDefaults] objectForKey:KEY_AMEASVAR]) floatValue];
        dc->image_width = [((NSNumber*)[[NSUserDefaults standardUserDefaults] objectForKey:KEY_IMAGE_WIDTH]) intValue];
        dc->image_height = [((NSNumber*)[[NSUserDefaults standardUserDefaults] objectForKey:KEY_IMAGE_WIDTH]) intValue];
        dc->shutter_delay = [((NSNumber*)[[NSUserDefaults standardUserDefaults] objectForKey:KEY_SHUTTER_DELAY]) intValue];
        dc->shutter_period = [((NSNumber*)[[NSUserDefaults standardUserDefaults] objectForKey:KEY_SHUTTER_PERIOD]) intValue];
    }
    @catch (NSException *exception) {
        NSLog(@"Failed to get saved calibration data: %@", exception.debugDescription);
        return NO;
    }
        
    return YES;
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
    dc.Wc[0] = -sqrt(.5) * M_PI;
    dc.Wc[1] = sqrt(.5) * M_PI;
    dc.Wc[2] = 0.;
    dc.a_bias_var[0] = 1.e-2;
    dc.a_bias_var[1] = 1.e-2;
    dc.a_bias_var[2] = 1.e-2;
    dc.w_bias_var[0] = 1.e-2;
    dc.w_bias_var[1] = 1.e-2;
    dc.w_bias_var[2] = 1.e-2;
    dc.Tc_var[0] = 1.e-2;
    dc.Tc_var[1] = 1.e-2;
    dc.Tc_var[2] = 1.e-2;
    dc.Wc_var[0] = 1.e-4;
    dc.Wc_var[1] = 1.e-4;
    dc.Wc_var[2] = 1.e-4;
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
    dc.shutter_period = 31000;
    return dc;
}

@end
