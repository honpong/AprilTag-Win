//
//  TMNewMeasurementTests.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 5/1/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMNewMeasurementTests.h"

@implementation TMNewMeasurementTests

- (void) setUp
{
    UIStoryboard *storyboard = [UIStoryboard storyboardWithName:@"MainStoryboard_iPhone" bundle:nil];
    nav = [storyboard instantiateViewControllerWithIdentifier:@"NavController"];
    vc = [storyboard instantiateViewControllerWithIdentifier:@"NewMeasurement"];
    [nav pushViewController:vc animated:NO];
}

@end
