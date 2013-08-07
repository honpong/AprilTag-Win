//
//  TMLocationExtTests.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 3/1/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMLocationExtTests.h"
#import "TMLocation+TMLocationExt.h"

@implementation TMLocationExtTests

- (void)testGetEntity
{
    STAssertTrue([[[TMLocation getEntity] name] isEqualToString:ENTITY_STRING_LOCATION], @"getEntity returned wrong entity name");
}

@end
