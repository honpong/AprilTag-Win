//
//  RCCalibration.m
//  RCCore
//
//  Created by Ben Hirashima on 4/9/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCCalibration.h"
#import "RCPrivateHTTPClient.h"
#include "device_parameters.h"

@implementation RCCalibration

+ (BOOL) saveCalibrationData: (device_parameters)params
{
    LOGME
    
    NSDictionary* data = @{KEY_CALIBRATION_VERSION: [NSNumber numberWithFloat:CALIBRATION_VERSION],
        KEY_FX: @(params.Fx),
        KEY_FY: @(params.Fy),
        KEY_CX: @(params.Cx),
        KEY_CY: @(params.Cy),
        KEY_PX: @(params.px),
        KEY_PY: @(params.py),
        KEY_K0: @(params.K0),
        KEY_K1: @(params.K1),
        KEY_K2: @(params.K2),
        KEY_ABIAS0: @(params.a_bias[0]),
        KEY_ABIAS1: @(params.a_bias[1]),
        KEY_ABIAS2: @(params.a_bias[2]),
        KEY_WBIAS0: @(params.w_bias[0]),
        KEY_WBIAS1: @(params.w_bias[1]),
        KEY_WBIAS2: @(params.w_bias[2]),
        KEY_TC0: @(params.Tc[0]),
        KEY_TC1: @(params.Tc[1]),
        KEY_TC2: @(params.Tc[2]),
        KEY_WC0: @(params.Wc[0]),
        KEY_WC1: @(params.Wc[1]),
        KEY_WC2: @(params.Wc[2]),
        KEY_ABIASVAR0: @(params.a_bias_var[0]),
        KEY_ABIASVAR1: @(params.a_bias_var[1]),
        KEY_ABIASVAR2: @(params.a_bias_var[2]),
        KEY_WBIASVAR0: @(params.w_bias_var[0]),
        KEY_WBIASVAR1: @(params.w_bias_var[1]),
        KEY_WBIASVAR2: @(params.w_bias_var[2]),
        KEY_TCVAR0: @(params.Tc_var[0]),
        KEY_TCVAR1: @(params.Tc_var[1]),
        KEY_TCVAR2: @(params.Tc_var[2]),
        KEY_WCVAR0: @(params.Wc_var[0]),
        KEY_WCVAR1: @(params.Wc_var[1]),
        KEY_WCVAR2: @(params.Wc_var[2]),
        KEY_WMEASVAR: @(params.w_meas_var),
        KEY_AMEASVAR: @(params.a_meas_var),
        KEY_IMAGE_WIDTH: @(params.image_width),
        KEY_IMAGE_HEIGHT: @(params.image_height)};
    
    [[NSUserDefaults standardUserDefaults] setObject:data forKey:PREF_DEVICE_PARAMS];
    return [[NSUserDefaults standardUserDefaults] synchronize];
}

+ (corvis_device_type) getCorvisDeviceForDeviceType: (DeviceType)type
{
    switch(type) {
        case DeviceTypeiPhone4s:
            return DEVICE_TYPE_IPHONE4S;
        case DeviceTypeiPhone5:
            return DEVICE_TYPE_IPHONE5;
        case DeviceTypeiPhone5c:
            return DEVICE_TYPE_IPHONE5C;
        case DeviceTypeiPhone5s:
            return DEVICE_TYPE_IPHONE5S;
        case DeviceTypeiPhone6:
            return DEVICE_TYPE_IPHONE6;
        case DeviceTypeiPhone6Plus:
            return DEVICE_TYPE_IPHONE6PLUS;
        case DeviceTypeiPod5:
            return DEVICE_TYPE_IPOD5;
        case DeviceTypeiPad2:
            return DEVICE_TYPE_IPAD2;
        case DeviceTypeiPad3:
            return DEVICE_TYPE_IPAD3;
        case DeviceTypeiPad4:
            return DEVICE_TYPE_IPAD4;
        case DeviceTypeiPadAir:
            return DEVICE_TYPE_IPADAIR;
        case DeviceTypeiPadAir2:
            return DEVICE_TYPE_IPADAIR2;
        case DeviceTypeiPadMini:
            return DEVICE_TYPE_IPADMINI;
        case DeviceTypeiPadMiniRetina:
            return DEVICE_TYPE_IPADMINIRETINA;
        case DeviceTypeiPadMiniRetina2:
            return DEVICE_TYPE_IPADMINIRETINA2;
        case DeviceTypeUnknown:
            return DEVICE_TYPE_UNKNOWN;
    }
}

+ (device_parameters) getDefaultsForCurrentDevice
{
    device_parameters dc;
    get_parameters_for_device([self getCorvisDeviceForDeviceType:[RCDeviceInfo getDeviceType]], &dc);
    return dc;
}

+ (device_parameters) getCalibrationData
{
    LOGME
    
    device_parameters defaults = [self getDefaultsForCurrentDevice], params;
    
    NSDictionary* data = [RCCalibration getCalibrationAsDictionary];
    if (data == nil || ![RCCalibration isCalibrationDataValid:data]) return defaults;

    params = [RCCalibration copySavedCalibrationData:data];
    
    //DLog(@"%@", [self stringFromCalibration:params]);
    return params;
}

+ (device_parameters) copySavedCalibrationData:(NSDictionary*)data
{
    device_parameters dc;
    if (!data) return dc;

    dc.Fx = [((NSNumber*)data[KEY_FX]) floatValue];
    dc.Fy = [((NSNumber*)data[KEY_FY]) floatValue];
    dc.Cx = [((NSNumber*)data[KEY_CX]) floatValue];
    dc.Cy = [((NSNumber*)data[KEY_CY]) floatValue];
    dc.px = [((NSNumber*)data[KEY_PX]) floatValue];
    dc.py = [((NSNumber*)data[KEY_PY]) floatValue];
    dc.K0 = [((NSNumber*)data[KEY_K0]) floatValue];
    dc.K1 = [((NSNumber*)data[KEY_K1]) floatValue];
    dc.K2 = [((NSNumber*)data[KEY_K2]) floatValue];
    dc.a_bias[0] = [((NSNumber*)data[KEY_ABIAS0]) floatValue];
    dc.a_bias[1] = [((NSNumber*)data[KEY_ABIAS1]) floatValue];
    dc.a_bias[2] = [((NSNumber*)data[KEY_ABIAS2]) floatValue];
    dc.w_bias[0] = [((NSNumber*)data[KEY_WBIAS0]) floatValue];
    dc.w_bias[1] = [((NSNumber*)data[KEY_WBIAS1]) floatValue];
    dc.w_bias[2] = [((NSNumber*)data[KEY_WBIAS2]) floatValue];
    dc.Tc[0] = [((NSNumber*)data[KEY_TC0]) floatValue];
    dc.Tc[1] = [((NSNumber*)data[KEY_TC1]) floatValue];
    dc.Tc[2] = [((NSNumber*)data[KEY_TC2]) floatValue];
    dc.Wc[0] = [((NSNumber*)data[KEY_WC0]) floatValue];
    dc.Wc[1] = [((NSNumber*)data[KEY_WC1]) floatValue];
    dc.Wc[2] = [((NSNumber*)data[KEY_WC2]) floatValue];
    dc.a_bias_var[0] = [((NSNumber*)data[KEY_ABIASVAR0]) floatValue];
    dc.a_bias_var[1] = [((NSNumber*)data[KEY_ABIASVAR1]) floatValue];
    dc.a_bias_var[2] = [((NSNumber*)data[KEY_ABIASVAR2]) floatValue];
    dc.w_bias_var[0] = [((NSNumber*)data[KEY_WBIASVAR0]) floatValue];
    dc.w_bias_var[1] = [((NSNumber*)data[KEY_WBIASVAR1]) floatValue];
    dc.w_bias_var[2] = [((NSNumber*)data[KEY_WBIASVAR2]) floatValue];
    dc.Tc_var[0] = [((NSNumber*)data[KEY_TCVAR0]) floatValue];
    dc.Tc_var[1] = [((NSNumber*)data[KEY_TCVAR1]) floatValue];
    dc.Tc_var[2] = [((NSNumber*)data[KEY_TCVAR2]) floatValue];
    dc.Wc_var[0] = [((NSNumber*)data[KEY_WCVAR0]) floatValue];
    dc.Wc_var[1] = [((NSNumber*)data[KEY_WCVAR1]) floatValue];
    dc.Wc_var[2] = [((NSNumber*)data[KEY_WCVAR2]) floatValue];
    dc.w_meas_var = [((NSNumber*)data[KEY_WMEASVAR]) floatValue];
    dc.a_meas_var = [((NSNumber*)data[KEY_AMEASVAR]) floatValue];
    dc.image_width = [((NSNumber*)data[KEY_IMAGE_WIDTH]) intValue];
    dc.image_height = [((NSNumber*)data[KEY_IMAGE_HEIGHT]) intValue];
    
    return dc;
}

+ (NSDictionary*) getCalibrationAsDictionary
{
    return [[NSUserDefaults standardUserDefaults] objectForKey:PREF_DEVICE_PARAMS];
}

+ (NSString*) stringFromCalibration:(device_parameters)dc
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
            
            "wm %e am %e width %d height %d\n",
            dc.Fx, dc.Fy,
            dc.Cx, dc.Cy,
            dc.px, dc.py,
            dc.K0, dc.K1, dc.K2,
            dc.a_bias[0], dc.a_bias[1], dc.a_bias[2], dc.a_bias_var[0], dc.a_bias_var[1], dc.a_bias_var[2],
            dc.w_bias[0], dc.w_bias[1], dc.w_bias[2], dc.w_bias_var[0], dc.w_bias_var[1], dc.w_bias_var[2],
            dc.Tc[0], dc.Tc[1], dc.Tc[2], dc.Tc_var[0], dc.Tc_var[1], dc.Tc_var[2],
            dc.Wc[0], dc.Wc[1], dc.Wc[2], dc.Wc_var[0], dc.Wc_var[1], dc.Wc_var[2],
            dc.w_meas_var, dc.a_meas_var, dc.image_width, dc.image_height];
}

+ (NSString*) getCalibrationAsString
{
    NSDictionary* data = [RCCalibration getCalibrationAsDictionary];
    if (!data) return nil;
    device_parameters dc = [self copySavedCalibrationData:data];
    return [self stringFromCalibration:dc];
}

+ (NSString*) getCalibrationAsJsonWithVendorId
{
    NSString* vendorId = [[[UIDevice currentDevice] identifierForVendor] UUIDString];
    NSDictionary *calibrationDict = [self getCalibrationAsDictionary];
    if(!calibrationDict) return nil;
    NSDictionary* dict = @{ @"id": vendorId, @"calibration": calibrationDict };
    
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

+ (void) clearCalibrationData
{
    [[NSUserDefaults standardUserDefaults] removeObjectForKey:PREF_DEVICE_PARAMS];
}

+ (BOOL) isCalibrationDataValid:(NSDictionary*)data
{
    BOOL result = NO;
    if (data)
    {
        NSNumber* calibrationVersion = data[KEY_CALIBRATION_VERSION];
        if (calibrationVersion && [calibrationVersion intValue] == CALIBRATION_VERSION) result = YES;
        device_parameters defaults = [self getDefaultsForCurrentDevice];
        //check if biases are within 5 sigma
        float a = [((NSNumber*)data[KEY_ABIAS0]) floatValue];
        if(a * a > 5. * 5. * defaults.a_bias_var[0]) result = NO;
        a = [((NSNumber*)data[KEY_ABIAS1]) floatValue];
        if(a * a > 5. * 5. * defaults.a_bias_var[1]) result = NO;
        a = [((NSNumber*)data[KEY_ABIAS2]) floatValue];
        if(a * a > 5. * 5. * defaults.a_bias_var[2]) result = NO;
        a = [((NSNumber*)data[KEY_WBIAS0]) floatValue];
        if(a * a > 5. * 5. * defaults.w_bias_var[0]) result = NO;
        a = [((NSNumber*)data[KEY_WBIAS1]) floatValue];
        if(a * a > 5. * 5. * defaults.w_bias_var[1]) result = NO;
        a = [((NSNumber*)data[KEY_WBIAS2]) floatValue];
        if(a * a > 5. * 5. * defaults.w_bias_var[2]) result = NO;
    }
    return result;
}

#ifndef OFFLINE
+ (void) postDeviceCalibration:(void (^)())successBlock onFailure:(void (^)(NSInteger statusCode))failureBlock
{
    LOGME;
    
    NSString *jsonString = [RCCalibration getCalibrationAsJsonWithVendorId];
    if(!jsonString)
    {
        DLog(@"Failed to get calibration. Result was nil.");
        return;
    }
    NSDictionary* postParams = @{ @"secret": @"BensTheDude", JSON_KEY_FLAG:[NSNumber numberWithInt: JsonBlobFlagCalibrationData], JSON_KEY_BLOB: jsonString };
    
    [HTTP_CLIENT
     postPath:API_DATUM_LOGGED
     parameters:postParams
     success:^(RCAFHTTPRequestOperation *operation, id JSON)
     {
//         DLog(@"POST Response\n%@", operation.responseString);
         if (successBlock) successBlock();
     }
     failure:^(RCAFHTTPRequestOperation *operation, NSError *error)
     {
         if (operation.response.statusCode)
         {
             DLog(@"Failed to POST. Status: %li %@", (long)operation.response.statusCode, operation.responseString);
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
#endif

@end
