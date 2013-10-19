//
//  ViewController.h
//  RCCapture
//
//  Created by Brian on 9/27/13.
//  Copyright (c) 2013 Realitycap. All rights reserved.
//

#import <UIKit/UIKit.h>

#import "CaptureController.h"

@interface ViewController : UIViewController <CaptureControllerDelegate>

- (IBAction)startStopClicked:(id)sender;

@property (weak, nonatomic) IBOutlet UIView *previewView;
@property (weak, nonatomic) IBOutlet UIButton *startStopButton;

@end
