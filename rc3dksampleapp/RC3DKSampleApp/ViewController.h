//
//  ViewController.h
//  RC3DKSampleApp
//
//  Created by Ben Hirashima on 7/19/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <RC3DK/RC3DK.h>
#import "AVSessionManager.h"
#import "LocationManager.h"
#import "MotionManager.h"
#import "VideoManager.h"

@interface ViewController : UIViewController <RCSensorFusionDelegate>

@property (weak, nonatomic) IBOutlet UIButton *startStopButton;
@property (weak, nonatomic) IBOutlet UITextField *distanceText;
@property (weak, nonatomic) IBOutlet UILabel *statusLabel;

- (IBAction)buttonTapped:(id)sender;

@end
