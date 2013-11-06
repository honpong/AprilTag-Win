//
//  RCCalibration.m
//  RCCore
//
//  Created by Ben Hirashima on 4/9/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCCalibration.h"
#import "RCPrivateHTTPClient.h"

@implementation RCCalibration

+ (void) saveCalibrationData: (corvis_device_parameters)params
{
    LOGME
    
    NSDictionary* data = [NSDictionary dictionaryWithObjectsAndKeys:
        [NSNumber numberWithFloat:CALIBRATION_VERSION], KEY_CALIBRATION_VERSION,
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

+ (corvis_device_parameters) getDefaultsForCurrentDevice
{
    switch ([RCDeviceInfo getDeviceType]) {
        case DeviceTypeiPadAir:
            return [self getDefaultsForiPadAir];

        case DeviceTypeiPadMini:
            return [self getDefaultsForiPadMini];
            
        case DeviceTypeiPad4:
            return [self getDefaultsForiPad4];
            
        case DeviceTypeiPad3:
            return [self getDefaultsForiPad3];
            
        case DeviceTypeiPad2:
            return [self getDefaultsForiPad2];
            
        case DeviceTypeiPhone5s:
            return [self getDefaultsForiPhone5s];

        case DeviceTypeiPhone5:
        case DeviceTypeiPhone5c:
            return [self getDefaultsForiPhone5];

        case DeviceTypeiPhone4s:
            return [self getDefaultsForiPhone4s];
            
        case DeviceTypeiPod5:
            return [self getDefaultsForiPod5];
            
        default:
            return [self getDefaultsForiPad3]; //TODO: need to prevent this - can't run on unsupported devices
    }
}

+ (corvis_device_parameters) getCalibrationData
{
    LOGME
    
    corvis_device_parameters defaults = [self getDefaultsForCurrentDevice], params;

#warning Tc is currently restored from defaults for every device.
    if ([RCCalibration copySavedCalibrationData:&params]) { //TODO: what if this app is restored from itunes on a different device?
        params.Tc[0] = defaults.Tc[0];
        params.Tc[1] = defaults.Tc[1];
        params.Tc[2] = defaults.Tc[2];
   /*     params.Fx = defaults.Fx;
        params.Fy = defaults.Fy;
        params.Cx = defaults.Cx;
        params.Cy = defaults.Cy;
        params.px = defaults.px;
        params.py = defaults.py;
        params.K[0] = defaults.K[0];
        params.K[1] = defaults.K[1];
        params.K[2] = defaults.K[2];*/
    } else {
        params = defaults;
    }
    
    //DLog(@"%@", [self stringFromCalibration:params]);
    return params;
}

+ (BOOL) copySavedCalibrationData:(struct corvis_device_parameters*)dc
{
    NSDictionary* data = [RCCalibration getCalibrationAsDictionary];
    if (data == nil || ![RCCalibration isCalibrationDataValid:data]) return NO;

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
        dc->image_height = [((NSNumber*)[data objectForKey:KEY_IMAGE_HEIGHT]) intValue];
        dc->shutter_delay = [((NSNumber*)[data objectForKey:KEY_SHUTTER_DELAY]) intValue];
        dc->shutter_period = [((NSNumber*)[data objectForKey:KEY_SHUTTER_PERIOD]) intValue];
    }
    @catch (NSException *exception) {
        DLog(@"Failed to get saved calibration data: %@", exception.debugDescription);
        return NO;
    }
        
    return YES;
}

+ (NSDictionary*) getCalibrationAsDictionary
{
    return [[NSUserDefaults standardUserDefaults] objectForKey:PREF_DEVICE_PARAMS];
}

+ (NSString*) stringFromCalibration:(struct corvis_device_parameters)dc
{
    return [NSString stringWithFormat:
            @"F % .1f % .1f\n"
            "C % .1f % .1f\n"
            "p % e % e\n"
            "K % .4f % .4f % .4f\n\n"
            
            "abias % .4f % .4f % .4f %.1e %.1e %.1e\n"
            "wbias % .4f % .4f % .4f %.1e %.1e %.1e\n\n"
            
            "Tc % .4f % .4f % .4f %.1e %.1e %.1e\n"
            "Wc % .4f % .4f % .4f %.1e %.1e %.1e\n\n"
            
            "wm %e am %e width %d height %d delay %d period %d\n",
            dc.Fx, dc.Fy,
            dc.Cx, dc.Cy,
            dc.px, dc.py,
            dc.K[0], dc.K[1], dc.K[2],
            dc.a_bias[0], dc.a_bias[1], dc.a_bias[2], dc.a_bias_var[0], dc.a_bias_var[1], dc.a_bias_var[2],
            dc.w_bias[0], dc.w_bias[1], dc.w_bias[2], dc.w_bias_var[0], dc.w_bias_var[1], dc.w_bias_var[2],
            dc.Tc[0], dc.Tc[1], dc.Tc[2], dc.Tc_var[0], dc.Tc_var[1], dc.Tc_var[2],
            dc.Wc[0], dc.Wc[1], dc.Wc[2], dc.Wc_var[0], dc.Wc_var[1], dc.Wc_var[2],
            dc.w_meas_var, dc.a_meas_var, dc.image_width, dc.image_height, dc.shutter_period, dc.shutter_delay];
}

+ (NSString*) getCalibrationAsString
{
    struct corvis_device_parameters dc;
    if(![self copySavedCalibrationData:&dc]) return @"";
    return [self stringFromCalibration:dc];
}

+ (NSString*) getCalibrationAsJsonWithVendorId
{
    NSString* vendorId = [[[UIDevice currentDevice] identifierForVendor] UUIDString];
    NSString* calibrationString = [self getCalibrationAsString];
    NSDictionary* dict = @{ @"id": vendorId, @"calibration": calibrationString };
    
    NSError *error;
    NSData *jsonData = [NSJSONSerialization dataWithJSONObject:dict
                                                       options:NSJSONWritingPrettyPrinted
                                                         error:&error];
    if (! jsonData) {
        NSLog(@"Got an error: %@", error);
        return @"";
    } else {
        return [[NSString alloc] initWithData:jsonData encoding:NSUTF8StringEncoding];
    }
}

+ (BOOL) hasCalibrationData
{
    NSDictionary* data = [[NSUserDefaults standardUserDefaults] objectForKey:PREF_DEVICE_PARAMS];
    return [RCCalibration isCalibrationDataValid:data];
}

+ (BOOL) isCalibrationDataValid:(NSDictionary*)data
{
    BOOL result = NO;
    if (data)
    {
        NSNumber* calibrationVersion = [data objectForKey:KEY_CALIBRATION_VERSION];
        if (calibrationVersion && [calibrationVersion intValue] == CALIBRATION_VERSION) result = YES;
        corvis_device_parameters defaults = [self getDefaultsForCurrentDevice];
        //check if biases are within 5 sigma
        float a = [((NSNumber*)[data objectForKey:KEY_ABIAS0]) floatValue];
        if(a * a > 5. * 5. * defaults.a_bias_var[0]) result = NO;
        a = [((NSNumber*)[data objectForKey:KEY_ABIAS1]) floatValue];
        if(a * a > 5. * 5. * defaults.a_bias_var[1]) result = NO;
        a = [((NSNumber*)[data objectForKey:KEY_ABIAS0]) floatValue];
        if(a * a > 5. * 5. * defaults.a_bias_var[2]) result = NO;
        a = [((NSNumber*)[data objectForKey:KEY_WBIAS0]) floatValue];
        if(a * a > 5. * 5. * defaults.w_bias_var[0]) result = NO;
        a = [((NSNumber*)[data objectForKey:KEY_WBIAS1]) floatValue];
        if(a * a > 5. * 5. * defaults.w_bias_var[1]) result = NO;
        a = [((NSNumber*)[data objectForKey:KEY_WBIAS2]) floatValue];
        if(a * a > 5. * 5. * defaults.w_bias_var[2]) result = NO;
    }
    return result;
}

+ (corvis_device_parameters) getDefaultsForiPhone4s
{
    LOGME
    corvis_device_parameters dc;
    dc.Fx = 610.;
    dc.Fy = 610.;
    dc.Cx = 319.5;
    dc.Cy = 239.5;
    dc.px = 0.;
    dc.py = 0.;
    dc.K[0] = .20;
    dc.K[1] = -.55;
    dc.K[2] = 0.;
    dc.a_bias[0] = 0.;
    dc.a_bias[1] = 0.;
    dc.a_bias[2] = 0.;
    dc.w_bias[0] = 0.;
    dc.w_bias[1] = 0.;
    dc.w_bias[2] = 0.;
    dc.Tc[0] = 0.;
    dc.Tc[1] = 0.015;
    dc.Tc[2] = 0.;
    dc.Wc[0] = sqrt(2.)/2. * M_PI;
    dc.Wc[1] = -sqrt(2.)/2. * M_PI;
    dc.Wc[2] = 0.;
    double a_bias_stdev = .02 * 9.8 / 2.; //20 mg "typical", assuming that means two-sigma
    for(int i = 0; i < 3; ++i) dc.a_bias_var[i] = a_bias_stdev * a_bias_stdev;
    double w_bias_stdev = 1. / 180. * M_PI; //10 dps typical according to specs, but not in practice - factory or apple calibration?
    for(int i = 0; i < 3; ++i) dc.w_bias_var[i] = w_bias_stdev * w_bias_stdev;
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

+ (corvis_device_parameters) getDefaultsForiPhone5
{
    LOGME
    corvis_device_parameters dc;
    dc.Fx = 585.;
    dc.Fy = 585.;
    dc.Cx = 319.5;
    dc.Cy = 239.5;
    dc.px = 0.;
    dc.py = 0.;
    dc.K[0] = .10;
    dc.K[1] = -.10;
    dc.K[2] = 0.;
    dc.a_bias[0] = 0.;
    dc.a_bias[1] = 0.;
    dc.a_bias[2] = 0.;
    dc.w_bias[0] = 0.;
    dc.w_bias[1] = 0.;
    dc.w_bias[2] = 0.;
    dc.Tc[0] = 0.000;
    dc.Tc[1] = 0.000;
    dc.Tc[2] = -0.008;
    dc.Wc[0] = sqrt(2.)/2. * M_PI;
    dc.Wc[1] = -sqrt(2.)/2. * M_PI;
    dc.Wc[2] = 0.;
    double a_bias_stdev = .02 * 9.8 / 2.; //20 mg "typical", assuming that means two-sigma
    for(int i = 0; i < 3; ++i) dc.a_bias_var[i] = a_bias_stdev * a_bias_stdev;
    double w_bias_stdev = 1. / 180. * M_PI; //10 dps typical according to specs, but not in practice - factory or apple calibration?
    for(int i = 0; i < 3; ++i) dc.w_bias_var[i] = w_bias_stdev * w_bias_stdev;
    dc.Tc_var[0] = 1.e-7;
    dc.Tc_var[1] = 1.e-7;
    dc.Tc_var[2] = 1.e-7;
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

+ (corvis_device_parameters) getDefaultsForiPhone5s
{
    LOGME
    corvis_device_parameters dc;
    dc.Fx = 525.;
    dc.Fy = 525.;
    dc.Cx = 319.5;
    dc.Cy = 239.5;
    dc.px = 0.;
    dc.py = 0.;
    dc.K[0] = .05;
    dc.K[1] = -.06;
    dc.K[2] = 0.;
    dc.a_bias[0] = 0.;
    dc.a_bias[1] = 0.;
    dc.a_bias[2] = 0.;
    dc.w_bias[0] = 0.;
    dc.w_bias[1] = 0.;
    dc.w_bias[2] = 0.;
    dc.Tc[0] = -0.005;
    dc.Tc[1] = 0.030;
    dc.Tc[2] = 0.000;
    dc.Wc[0] = sqrt(2.)/2. * M_PI;
    dc.Wc[1] = -sqrt(2.)/2. * M_PI;
    dc.Wc[2] = 0.;
    double a_bias_stdev = .02 * 9.8 / 2.; //20 mg "typical", assuming that means two-sigma
    for(int i = 0; i < 3; ++i) dc.a_bias_var[i] = a_bias_stdev * a_bias_stdev;
    double w_bias_stdev = 1. / 180. * M_PI; //10 dps typical according to specs, but not in practice - factory or apple calibration?
    for(int i = 0; i < 3; ++i) dc.w_bias_var[i] = w_bias_stdev * w_bias_stdev;
    dc.Tc_var[0] = 1.e-7;
    dc.Tc_var[1] = 1.e-7;
    dc.Tc_var[2] = 1.e-7;
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

+ (corvis_device_parameters) getDefaultsForiPod5
{
    LOGME
    corvis_device_parameters dc;
    dc.Fx = 586.;
    dc.Fy = 586.;
    dc.Cx = 319.5;
    dc.Cy = 239.5;
    dc.px = 0.;
    dc.py = 0.;
    dc.K[0] = .22;
    dc.K[1] = -.56;
    dc.K[2] = 0.;
    dc.a_bias[0] = 0.;
    dc.a_bias[1] = 0.;
    dc.a_bias[2] = 0.;
    dc.w_bias[0] = 0.;
    dc.w_bias[1] = 0.;
    dc.w_bias[2] = 0.;
    dc.Tc[0] = -0.03;
    dc.Tc[1] = -0.03;
    dc.Tc[2] = 0.;
    dc.Wc[0] = sqrt(2.)/2. * M_PI;
    dc.Wc[1] = -sqrt(2.)/2. * M_PI;
    dc.Wc[2] = 0.;
    double a_bias_stdev = .02 * 9.8 / 2.; //20 mg "typical", assuming that means two-sigma
    for(int i = 0; i < 3; ++i) dc.a_bias_var[i] = a_bias_stdev * a_bias_stdev;
    double w_bias_stdev = 1. / 180. * M_PI; //10 dps typical according to specs, but not in practice - factory or apple calibration?
    for(int i = 0; i < 3; ++i) dc.w_bias_var[i] = w_bias_stdev * w_bias_stdev;
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
    LOGME
    corvis_device_parameters dc;
    dc.Fx = 795.;
    dc.Fy = 795.;
    dc.Cx = 319.5;
    dc.Cy = 239.5;
    dc.px = 0.;
    dc.py = 0.;
    dc.K[0] = -.06;
    dc.K[1] = .19;
    dc.K[2] = 0.;
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
    double a_bias_stdev = .02 * 9.8 / 2.; //20 mg "typical", assuming that means two-sigma
    for(int i = 0; i < 3; ++i) dc.a_bias_var[i] = a_bias_stdev * a_bias_stdev;
    double w_bias_stdev = 1. / 180. * M_PI; //10 dps typical according to specs, but not in practice - factory or apple calibration?
    for(int i = 0; i < 3; ++i) dc.w_bias_var[i] = w_bias_stdev * w_bias_stdev;
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
    LOGME
    corvis_device_parameters dc;
    dc.Fx = 620.;
    dc.Fy = 620.;
    dc.Cx = 319.5;
    dc.Cy = 239.5;
    dc.px = 0.;
    dc.py = 0.;
    dc.K[0] = .17;
    dc.K[1] = -.38;
    dc.K[2] = 0.;
    dc.a_bias[0] = 0.;
    dc.a_bias[1] = 0.;
    dc.a_bias[2] = 0.;
    dc.w_bias[0] = 0.;
    dc.w_bias[1] = 0.;
    dc.w_bias[2] = 0.;
    dc.Tc[0] = .05;
    dc.Tc[1] = .005;
    dc.Tc[2] = -.010;
    dc.Wc[0] = sqrt(2.)/2. * M_PI;
    dc.Wc[1] = -sqrt(2.)/2. * M_PI;
    dc.Wc[2] = 0.;
    double a_bias_stdev = .02 * 9.8 / 2.; //20 mg "typical", assuming that means two-sigma
    for(int i = 0; i < 3; ++i) dc.a_bias_var[i] = a_bias_stdev * a_bias_stdev;
    double w_bias_stdev = 1. / 180. * M_PI; //10 dps typical according to specs, but not in practice - factory or apple calibration?
    for(int i = 0; i < 3; ++i) dc.w_bias_var[i] = w_bias_stdev * w_bias_stdev;
    dc.Tc_var[0] = 1.e-7;
    dc.Tc_var[1] = 1.e-7;
    dc.Tc_var[2] = 1.e-7;
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

+ (corvis_device_parameters) getDefaultsForiPad4
{
    LOGME
    corvis_device_parameters dc;
    dc.Fx = 618.;
    dc.Fy = 618.;
    dc.Cx = 319.5;
    dc.Cy = 239.5;
    dc.px = 0.;
    dc.py = 0.;
    dc.K[0] = .19;
    dc.K[1] = -.52;
    dc.K[2] = 0.;
    dc.a_bias[0] = 0.;
    dc.a_bias[1] = 0.;
    dc.a_bias[2] = 0.;
    dc.w_bias[0] = 0.;
    dc.w_bias[1] = 0.;
    dc.w_bias[2] = 0.;
    dc.Tc[0] = .05;
    dc.Tc[1] = .005;
    dc.Tc[2] = -.010;
    dc.Wc[0] = sqrt(2.)/2. * M_PI;
    dc.Wc[1] = -sqrt(2.)/2. * M_PI;
    dc.Wc[2] = 0.;
    double a_bias_stdev = .02 * 9.8 / 2.; //20 mg "typical", assuming that means two-sigma
    for(int i = 0; i < 3; ++i) dc.a_bias_var[i] = a_bias_stdev * a_bias_stdev;
    double w_bias_stdev = 1. / 180. * M_PI; //10 dps typical according to specs, but not in practice - factory or apple calibration?
    for(int i = 0; i < 3; ++i) dc.w_bias_var[i] = w_bias_stdev * w_bias_stdev;
    dc.Tc_var[0] = 1.e-7;
    dc.Tc_var[1] = 1.e-7;
    dc.Tc_var[2] = 1.e-7;
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

+ (corvis_device_parameters) getDefaultsForiPadAir
{
    LOGME
    corvis_device_parameters dc;
    dc.Fx = 584.;
    dc.Fy = 584.;
    dc.Cx = 319.5;
    dc.Cy = 239.5;
    dc.px = 0.;
    dc.py = 0.;
    dc.K[0] = .13;
    dc.K[1] = -.31;
    dc.K[2] = 0.;
    dc.a_bias[0] = 0.;
    dc.a_bias[1] = 0.;
    dc.a_bias[2] = 0.;
    dc.w_bias[0] = 0.;
    dc.w_bias[1] = 0.;
    dc.w_bias[2] = 0.;
    dc.Tc[0] = .05;
    dc.Tc[1] = .005;
    dc.Tc[2] = -.010;
    dc.Wc[0] = sqrt(2.)/2. * M_PI;
    dc.Wc[1] = -sqrt(2.)/2. * M_PI;
    dc.Wc[2] = 0.;
    double a_bias_stdev = .02 * 9.8 / 2.; //20 mg "typical", assuming that means two-sigma
    for(int i = 0; i < 3; ++i) dc.a_bias_var[i] = a_bias_stdev * a_bias_stdev;
    double w_bias_stdev = 1. / 180. * M_PI; //10 dps typical according to specs, but not in practice - factory or apple calibration?
    for(int i = 0; i < 3; ++i) dc.w_bias_var[i] = w_bias_stdev * w_bias_stdev;
    dc.Tc_var[0] = 1.e-7;
    dc.Tc_var[1] = 1.e-7;
    dc.Tc_var[2] = 1.e-7;
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

+ (corvis_device_parameters) getDefaultsForiPadMini
{
    LOGME
    corvis_device_parameters dc;
    dc.Fx = 590.;
    dc.Fy = 590.;
    dc.Cx = 319.5;
    dc.Cy = 239.5;
    dc.px = 0.;
    dc.py = 0.;
    dc.K[0] = .20;
    dc.K[1] = -.40;
    dc.K[2] = 0.;
    dc.a_bias[0] = 0.;
    dc.a_bias[1] = 0.;
    dc.a_bias[2] = 0.;
    dc.w_bias[0] = 0.;
    dc.w_bias[1] = 0.;
    dc.w_bias[2] = 0.;
    dc.Tc[0] = -.012;
    dc.Tc[1] = .047;
    dc.Tc[2] = .003;
    dc.Wc[0] = sqrt(2.)/2. * M_PI;
    dc.Wc[1] = -sqrt(2.)/2. * M_PI;
    dc.Wc[2] = 0.;
    double a_bias_stdev = .02 * 9.8 / 2.; //20 mg "typical", assuming that means two-sigma
    for(int i = 0; i < 3; ++i) dc.a_bias_var[i] = a_bias_stdev * a_bias_stdev;
    double w_bias_stdev = 1. / 180. * M_PI; //10 dps typical according to specs, but not in practice - factory or apple calibration?
    for(int i = 0; i < 3; ++i) dc.w_bias_var[i] = w_bias_stdev * w_bias_stdev;
    dc.Tc_var[0] = 1.e-7;
    dc.Tc_var[1] = 1.e-7;
    dc.Tc_var[2] = 1.e-7;
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

+ (void) postDeviceCalibration:(void (^)())successBlock onFailure:(void (^)(int statusCode))failureBlock
{
    LOGME;
    
    NSString *jsonString = [RCCalibration getCalibrationAsJsonWithVendorId];
    NSDictionary* postParams = @{ @"secret": @"BensTheDude", JSON_KEY_FLAG:[NSNumber numberWithInt: JsonBlobFlagCalibrationData], JSON_KEY_BLOB: jsonString };
    
    [HTTP_CLIENT
     postPath:API_DATUM_LOGGED
     parameters:postParams
     success:^(AFHTTPRequestOperation *operation, id JSON)
     {
         DLog(@"POST Response\n%@", operation.responseString);
         if (successBlock) successBlock();
     }
     failure:^(AFHTTPRequestOperation *operation, NSError *error)
     {
         if (operation.response.statusCode)
         {
             DLog(@"Failed to POST. Status: %i %@", operation.response.statusCode, operation.responseString);
             NSString *requestBody = [[NSString alloc] initWithData:operation.request.HTTPBody encoding:NSUTF8StringEncoding];
             DLog(@"Failed request body:\n%@", requestBody);
         }
         else
         {
             DLog(@"Failed to POST.\n%@", error);
         }
         if (failureBlock) failureBlock(operation.response.statusCode);
     }
     ];
}

@end
