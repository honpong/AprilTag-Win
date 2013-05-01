//
//  RCCalibrationTests.m
//  RCCore
//
//  Created by Ben Hirashima on 5/1/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCCalibrationTests.h"
#import "RCCalibration.h"
#import "filter_setup.h"

@implementation RCCalibrationTests

- (void) testSaveAndRetrieve
{
    corvis_device_parameters params = [RCCalibration getCalibrationData];
    params.Fx = 0.1234;
    
    [RCCalibration saveCalibrationData:params];
    
    corvis_device_parameters params2 = [RCCalibration getCalibrationData];
    
    STAssertEquals(params.Fx, params2.Fx, @"Fx doesn't match");
}

- (void) testGetCalibrationAsDictionary
{
    NSDictionary* data = [RCCalibration getCalibrationAsDictionary];
    STAssertNotNil(data, @"Dictionary is nil");
    STAssertNotNil([data objectForKey:KEY_FX], @"Fx is nil");
}
@end
