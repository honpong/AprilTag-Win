//
//  RCCalibration.h
//  RCCore
//
//  Created by Ben Hirashima on 4/9/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "filter_setup.h"
#import "RCDeviceInfo.h"

#define KEY_CALIBRATION_DATA @"DeviceCalibration"

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

@interface RCCalibration : NSObject

+ (void) saveCalibrationData: (corvis_device_parameters)params;
+ (corvis_device_parameters) getCalibrationData;
+ (NSDictionary*) getCalibrationAsDictionary;

@end
