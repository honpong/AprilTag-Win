//
//  RCViewController.m
//  DistanceLabelTest
//
//  Created by Ben Hirashima on 4/7/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "RCViewController.h"

@interface RCViewController ()

@end

@implementation RCViewController

- (void)viewDidLoad
{
    [super viewDidLoad];

    [self.label1 setDistance:[RCDistanceImperialFractional distWithMiles:0 withYards:0 withFeet:0 withInches:2 withNominator:3 withDenominator:4]];
    [self.label2 setDistance:[RCDistanceImperialFractional distWithMiles:0 withYards:0 withFeet:0 withInches:3 withNominator:3 withDenominator:8]];
    [self.label3 setDistance:[RCDistanceImperialFractional distWithMiles:0 withYards:0 withFeet:0 withInches:4 withNominator:5 withDenominator:16]];
    [self.label4 setDistance:[RCDistanceImperialFractional distWithMiles:0 withYards:0 withFeet:0 withInches:5 withNominator:13 withDenominator:16]];
    self.label4.centerAlignmentExcludesFraction = YES;
}

@end
