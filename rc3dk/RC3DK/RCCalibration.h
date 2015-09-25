//
//  RCCalibration.h
//  RCCore
//
//  Created by Ben Hirashima on 4/9/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "device_parameters.h"
#import "RCDeviceInfo.h"

#define KEY_FX @"Fx"
#define KEY_FY @"Fy"
#define KEY_CX @"Cx"
#define KEY_CY @"Cy"
#define KEY_PX @"px"
#define KEY_PY @"py"
#define KEY_K0 @"K0"
#define KEY_K1 @"K1"
#define KEY_K2 @"K2"
#define KEY_ABIAS0 @"abias0"
#define KEY_ABIAS1 @"abias1"
#define KEY_ABIAS2 @"abias2"
#define KEY_WBIAS0 @"wbias0"
#define KEY_WBIAS1 @"wbias1"
#define KEY_WBIAS2 @"wbias2"
#define KEY_TC0 @"Tc0"
#define KEY_TC1 @"Tc1"
#define KEY_TC2 @"Tc2"
#define KEY_WC0 @"Wc0"
#define KEY_WC1 @"Wc1"
#define KEY_WC2 @"Wc2"
#define KEY_ABIASVAR0 @"abiasvar0"
#define KEY_ABIASVAR1 @"abiasvar1"
#define KEY_ABIASVAR2 @"abiasvar2"
#define KEY_WBIASVAR0 @"wbiasvar0"
#define KEY_WBIASVAR1 @"wbiasvar1"
#define KEY_WBIASVAR2 @"wbiasvar2"
#define KEY_TCVAR0 @"TcVar0"
#define KEY_TCVAR1 @"TcVar1"
#define KEY_TCVAR2 @"TcVar2"
#define KEY_WCVAR0 @"WcVar0"
#define KEY_WCVAR1 @"WcVar1"
#define KEY_WCVAR2 @"WcVar2"
#define KEY_WMEASVAR @"wMeasVar"
#define KEY_AMEASVAR @"aMeasVar"
#define KEY_IMAGE_WIDTH @"imageWidth"
#define KEY_IMAGE_HEIGHT @"imageHeight"
#define KEY_CALIBRATION_VERSION @"calibrationVersion"

#define CALIBRATION_VERSION 7

#define JSON_KEY_FLAG @"flag"
#define JSON_KEY_BLOB @"blob"
#define JSON_KEY_DEVICE_TYPE @"device_type"

typedef enum {
    JsonBlobFlagCalibrationData = 1
} JsonBlobFlag;

@interface RCCalibration : NSObject

+ (void) saveCalibrationData: (struct corvis_device_parameters)params;
+ (struct corvis_device_parameters) getCalibrationData;
+ (NSDictionary*) getCalibrationAsDictionary;
+ (NSString*) getCalibrationAsString;
+ (void) clearCalibrationData;
+ (NSString*) stringFromCalibration:(struct corvis_device_parameters)dc;
+ (BOOL) hasCalibrationData;
+ (BOOL) isCalibrationDataValid:(NSDictionary*)data;
+ (void) postDeviceCalibration:(void (^)())successBlock onFailure:(void (^)(NSInteger statusCode))failureBlock;
+ (NSString*) getCalibrationAsJsonWithVendorId;

@end
