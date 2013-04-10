//
//  NSValue+RCDeviceParams.h
//  RCCore
//
//  Created by Ben Hirashima on 4/9/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "filter_setup.h"

@interface NSValue (RCDeviceParams)

+ (id) valueWithDeviceParams:(corvis_device_parameters)params;
- (corvis_device_parameters) getDeviceParams;

@end
