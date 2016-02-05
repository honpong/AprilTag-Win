//
//  RCDateFormatterTests.m
//  RCCore
//
//  Created by Ben Hirashima on 3/7/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCDateFormatterTests.h"
#import "RCDateFormatter.h"

@implementation RCDateFormatterTests

- (void) testGetInstanceForFormat
{
    NSString *formatString = @"yyyy-MM-dd'T'HH:mm:ss";
    NSDateFormatter *df = [RCDateFormatter getInstanceForFormat:formatString];
    
    STAssertNotNil(df, @"Date formatter instance was nil");
    STAssertTrue([df.dateFormat isEqualToString:formatString], @"Date format string doesn't match");
}

@end
