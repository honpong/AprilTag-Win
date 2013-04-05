//
//  TMResultsVCTests.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 4/5/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMResultsVCTests.h"

@implementation TMResultsVCTests

- (void) setUp
{
    UIStoryboard *storyboard = [UIStoryboard storyboardWithName:@"MainStoryboard_iPhone" bundle:nil];
    nav = [storyboard instantiateViewControllerWithIdentifier:@"NavController"];
    vc = [storyboard instantiateViewControllerWithIdentifier:@"Results"];
    [nav pushViewController:vc animated:NO];
}

- (void) tearDown
{
    nav = nil;
    vc = nil;
}

- (void) testSaveWhenMeasurementDeleted
{
    vc.theMeasurement = (TMMeasurement*)[DATA_MANAGER getNewObjectOfType:[TMMeasurement getEntity]];
    vc.theMeasurement.name = @"Test";
    [DATA_MANAGER saveContext];
    
    [vc handleDeleteButton:vc];
    [vc saveMeasurement];
    
    STAssertNil([DATA_MANAGER getObjectOfType:[TMMeasurement getEntity] byDbid:vc.theMeasurement.dbid], @"Measurement should be deleted but isn't");
}

@end
