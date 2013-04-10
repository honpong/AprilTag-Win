//
//  RCCalibration.h
//  RCCore
//
//  Created by Ben Hirashima on 4/9/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "filter_setup.h"
#import "RCDeviceInfo.h"
#import "NSValue+RCDeviceParams.h"

@interface RCCalibration : NSObject

+ (corvis_device_parameters) getCalibrationData;

@end
