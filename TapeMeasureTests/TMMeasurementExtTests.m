//
//  TMMeasurementExtTests.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 3/1/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMMeasurementExtTests.h"
#import "TMMeasurement+TMMeasurementExt.h"

@implementation TMMeasurementExtTests

- (void)testGetEntity
{
    STAssertTrue([[[TMMeasurement getEntity] name] isEqualToString:ENTITY_STRING_MEASUREMENT], @"getEntity returned wrong entity name");
}

@end
