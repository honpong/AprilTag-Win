//
//  RCDeviceInfoTests.m
//  RCCore
//
//  Created by Ben Hirashima on 3/7/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCDeviceInfoTests.h"
#import "RCDeviceInfo.h"

@implementation RCDeviceInfoTests

- (void) testGetOSVersion
{
    NSString *version = [RCDeviceInfo getOSVersion];
    STAssertNotNil(version, @"OS version was nil");
    STAssertTrue(version.length > 0, @"OS version was zero length");
}

- (void) testGetPlatformString
{
    NSString *version = [RCDeviceInfo getPlatformString];
    STAssertNotNil(version, @"PlatformString was nil");
    STAssertTrue(version.length > 0, @"PlatformString was zero length");
}

@end
