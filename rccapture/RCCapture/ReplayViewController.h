//
//  ReplayViewController.h
//  RCCapture
//
//  Created by Brian on 2/18/14.
//  Copyright (c) 2014 Realitycap. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "ReplayController.h"

@interface ReplayViewController : UIViewController <ReplayControllerDelegate>

- (IBAction) startStopClicked:(id)sender;

- (void) replayProgress:(float)progress;
- (void) replayFinished;

@property (weak,nonatomic) IBOutlet UILabel * progressText;
@property (weak,nonatomic) IBOutlet UIProgressView * progressBar;
@property (weak,nonatomic) IBOutlet UIButton * startButton;

@end
