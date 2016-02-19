//
//  ReplayViewController.h
//  RCCapture
//
//  Created by Brian on 2/18/14.
//  Copyright (c) 2014 Realitycap. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "RC3DK.h"
#import "RCReplayManager.h"

@interface ReplayViewController : UIViewController <RCReplayManagerDelegate, UITableViewDataSource, UITableViewDelegate>

- (IBAction) startStopClicked:(id)sender;

- (void) replayProgress:(float)progress;
- (void) replayFinishedWithLength:(float)length withPathLength:(float)pathLength;

@property (weak,nonatomic) IBOutlet UILabel * progressText;
@property (weak,nonatomic) IBOutlet UIProgressView * progressBar;
@property (weak,nonatomic) IBOutlet UIButton * startButton;
@property (weak,nonatomic) IBOutlet UISwitch * realtimeSwitch;
@property (weak,nonatomic) IBOutlet UITableView * replayFilesTableView;
@property (weak, nonatomic) IBOutlet UILabel *outputLabel;
@end
