//
//  RCCalibration.h
//  RCCore
//
//  Created by Ben Hirashima on 4/9/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "device_parameters.h"
#import "RCDeviceInfo.h"

@interface RCCalibration : NSObject

+ (void) saveCalibrationData: (struct corvis_device_parameters)params;
+ (struct corvis_device_parameters) getCalibrationData;
+ (NSDictionary*) getCalibrationAsDictionary;
+ (NSString*) getCalibrationAsString;

@end
