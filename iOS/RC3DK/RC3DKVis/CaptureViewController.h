//
//  ViewController.h
//  RCCapture
//
//  Created by Brian on 9/27/13.
//  Copyright (c) 2013 Realitycap. All rights reserved.
//

#import <UIKit/UIKit.h>

#import "RC3DK.h"
#import "RCCaptureManager.h"

@interface CaptureViewController : UIViewController

- (IBAction)startStopClicked:(id)sender;
- (IBAction)cameraConfigureClick:(id)sender;

@property (weak, nonatomic) IBOutlet UIView *previewView;
@property (weak, nonatomic) IBOutlet UISegmentedControl *frameRateSelector;
@property (weak, nonatomic) IBOutlet UISegmentedControl *resolutionSelector;
@property (weak, nonatomic) IBOutlet UIButton *startStopButton;

@end
