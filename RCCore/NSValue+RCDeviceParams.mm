//
//  NSValue+RCDeviceParams.mm
//  RCCore
//
//  Created by Ben Hirashima on 4/9/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "NSValue+RCDeviceParams.h"

@implementation NSValue (RCDeviceParams)

+ (id) valueWithDeviceParams:(corvis_device_parameters)params
{
    return [NSValue value:&params withObjCType:@encode(corvis_device_parameters)];
}

- (corvis_device_parameters) getDeviceParams;
{
    corvis_device_parameters params;
    [self getValue:&params];
    return params;
}

@end
