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
    params.a_meas_var = 0.1234;
    
    [RCCalibration saveCalibrationData:params];
    
    corvis_device_parameters params2 = [RCCalibration getCalibrationData];
    
    STAssertEquals(params.a_meas_var, params2.a_meas_var, @"a_meas_var doesn't match");
}

- (void) testGetCalibrationAsDictionary
{
    corvis_device_parameters params = [RCCalibration getCalibrationData];
    [RCCalibration saveCalibrationData:params];
    
    NSDictionary* data = [RCCalibration getCalibrationAsDictionary];
    STAssertNotNil(data, @"Dictionary is nil");
    STAssertNotNil([data objectForKey:KEY_FX], @"Fx is nil");
}

- (void) testHasCalibrationDataTrue
{
    corvis_device_parameters params = [RCCalibration getCalibrationData];
    [RCCalibration saveCalibrationData:params];
    
    STAssertTrue([RCCalibration hasCalibrationData], @"hasCalibrationData didn't return YES");
}

- (void) testHasCalibrationDataFalse
{
    [[NSUserDefaults standardUserDefaults] setObject:nil forKey:PREF_DEVICE_PARAMS];
    STAssertFalse([RCCalibration hasCalibrationData], @"hasCalibration data didn't return NO");
}
@end
