//
//  ViewController.h
//  RC3DKSampleApp
//
//  Created by Ben Hirashima on 7/19/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "RCAVSessionManager.h"
#import "RCLocationManager.h"
#import "RCMotionManager.h"
#import "RCVideoManager.h"

@interface ViewController : UIViewController <RCSensorFusionDelegate>

@property (weak, nonatomic) IBOutlet UIButton *startStopButton;
@property (weak, nonatomic) IBOutlet UITextField *distanceText;
@property (weak, nonatomic) IBOutlet UILabel *statusLabel;

- (IBAction)buttonTapped:(id)sender;

@end
