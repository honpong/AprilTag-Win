//
//  TMViewController.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 10/2/12.
//  Copyright (c) 2012 RealityCap. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <CoreMotion/CoreMotion.h>

@interface TMViewController : UIViewController <UIAccelerometerDelegate> {
@private
	int distanceMeasured;
	NSTimer *repeatingTimer;
	bool isMeasuring;
	
	float lastAccel;
	int lastBump;
	
	CMMotionManager *motionMan;
}

- (IBAction)startRepeatingTimer:sender;
- (IBAction)handleButtonTap:(id)sender;

@property (weak, nonatomic) IBOutlet UILabel *lblDistance;
@property (weak, nonatomic) IBOutlet UILabel *lblInstructions;
@property (weak, nonatomic) IBOutlet UIButton *btnBegin;
@property (nonatomic,retain) IBOutlet UIView *vImagePreview;

@end
