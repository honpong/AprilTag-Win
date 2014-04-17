//
//  RCViewController.h
//  DistanceLabelTest
//
//  Created by Ben Hirashima on 4/7/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <RCCore/RCCore.h>

@interface RCViewController : UIViewController

@property (weak, nonatomic) IBOutlet RCDistanceLabel *label1;
@property (weak, nonatomic) IBOutlet RCDistanceLabel *label2;
@property (weak, nonatomic) IBOutlet RCDistanceLabel *label3;
@property (weak, nonatomic) IBOutlet RCDistanceLabel *label4;

@end
