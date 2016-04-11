//
//  RCCalibration.m
//  RCCore
//
//  Created by Ben Hirashima on 4/9/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCConstants.h"
#import "RCCalibration.h"
#import "RCPrivateHTTPClient.h"
#include "device_parameters.h"

@implementation RCCalibration

+ (BOOL) saveCalibrationData: (device_parameters)params
{
    std::string calJson;
    if (calibration_serialize(params, calJson))
    {
        [NSUserDefaults.standardUserDefaults setObject:[NSString stringWithUTF8String:calJson.c_str()] forKey:PREF_DEVICE_PARAMS];
        return [[NSUserDefaults standardUserDefaults] synchronize];
    }
    else
    {
        DLog(@"Failed to save calibration data");
        return NO;
    }
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
        case DeviceTypeiPhone6s:
            return DEVICE_TYPE_IPHONE6S;
        case DeviceTypeiPhone6sPlus:
            return DEVICE_TYPE_IPHONE6SPLUS;
        case DeviceTypeiPhoneSE:
            return DEVICE_TYPE_IPHONESE;
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
    NSString* calNSString = [RCCalibration loadStoredCalibrationJSONString];
    if (calNSString == nil) return [RCCalibration getDefaultsForCurrentDevice];

    std::string calString = [calNSString cStringUsingEncoding:NSUTF8StringEncoding];
    device_parameters params;
    if(calibration_deserialize(calString, params))
    {
        
        return params;
    }
    else
    {
        DLog(@"Failed to deserialize calibration");
        return [RCCalibration getDefaultsForCurrentDevice];
    }
}

+ (NSString*) loadStoredCalibrationJSONString
{
    return [[NSUserDefaults standardUserDefaults] objectForKey:PREF_DEVICE_PARAMS];
}

+ (NSString*) getCalibrationAsJsonWithVendorId
{
    NSString* vendorId = [[[UIDevice currentDevice] identifierForVendor] UUIDString];
    NSString* calJson = [RCCalibration loadStoredCalibrationJSONString];
    if (vendorId == nil || calJson == nil) return nil;
    return [NSString stringWithFormat:@"{\n    \"id\" : %@,\n    \"calibration\" :\n    %@\n}", vendorId, calJson];
}

+ (BOOL) hasCalibrationData
{
    NSString* jsonString = [[NSUserDefaults standardUserDefaults] objectForKey:PREF_DEVICE_PARAMS];
    if (jsonString == nil) return NO;
    
    std::string jsonStdString = [jsonString cStringUsingEncoding:NSUTF8StringEncoding];
    device_parameters params;
    if (!calibration_deserialize(jsonStdString, params)) return NO;
    
    return [RCCalibration isCalibrationDataValid:params];
}

+ (void) clearCalibrationData
{
    [[NSUserDefaults standardUserDefaults] removeObjectForKey:PREF_DEVICE_PARAMS];
}

+ (BOOL) isCalibrationDataValid:(device_parameters)params
{
    if (params.version != CALIBRATION_VERSION) return NO;
    
    device_parameters defaults = [self getDefaultsForCurrentDevice];
    
    double a = params.imu.a_bias_m__s2[0];
    if(a * a > 5. * 5. * defaults.imu.a_bias_var_m2__s4[0]) return NO;
    a = params.imu.a_bias_m__s2[1];
    if(a * a > 5. * 5. * defaults.imu.a_bias_var_m2__s4[1]) return NO;
    a = params.imu.a_bias_m__s2[2];
    if(a * a > 5. * 5. * defaults.imu.a_bias_var_m2__s4[2]) return NO;
    a = params.imu.w_bias_rad__s[0];
    if(a * a > 5. * 5. * defaults.imu.w_bias_var_rad2__s2[0]) return NO;
    a = params.imu.w_bias_rad__s[1];
    if(a * a > 5. * 5. * defaults.imu.w_bias_var_rad2__s2[1]) return NO;
    a = params.imu.w_bias_rad__s[2];
    if(a * a > 5. * 5. * defaults.imu.w_bias_var_rad2__s2[2]) return NO;
    
    return YES;
}

#ifndef OFFLINE
+ (void) postDeviceCalibration:(void (^)())successBlock onFailure:(void (^)(NSInteger statusCode))failureBlock
{
    LOGME;
    
    NSString *jsonString = [RCCalibration getCalibrationAsJsonWithVendorId];
    if(jsonString == nil)
    {
        DLog(@"Failed to get calibration. Result was nil.");
        return;
    }
    NSDictionary* postParams = @{ @"secret": @"BensTheDude", JSON_KEY_FLAG:[NSNumber numberWithInt: JsonBlobFlagCalibrationData], JSON_KEY_BLOB: jsonString };
    
    [[RCPrivateHTTPClient sharedInstance]
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
