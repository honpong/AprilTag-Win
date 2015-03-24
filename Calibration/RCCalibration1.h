//
//  RCCalibration1.h
//
//  Created by Ben Hirashima on 8/13/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "MBProgressHUD.h"
#import "RC3DK.h"

@protocol RCCalibrationDelegate <NSObject>

@required
/**
 Starts the AV session, video capture, and motion capture
 */
- (void) startMotionSensors;
/**
 Stops the AV session, video capture, and motion capture
 */
- (void) stopMotionSensors;
/**
 Called when all three calibration steps have been completed. 
 @returns A reference to the last view controller in the calibration sequence.
 */
- (void) calibrationDidFinish:(UIViewController*)lastViewController;

@optional
/**
 For notifying the delegate when calibration progresses from one step (view controller) to another
 */
- (void) calibrationScreenDidAppear:(UIViewController*)lastViewController;

@end

/**
 The calibration procedure consists of three distinct steps. Each step is handled by its own view controller.
 The calibration view controllers have a delegate that gets passed from one view controller to the next. When
 the last step is completed, the calibrationDidFinish method is called on the delegate. See the 
 RCCalibrationDelegate protocol.
 */
@interface RCCalibration1 : UIViewController <RCSensorFusionDelegate>

@property (weak, nonatomic) id<RCCalibrationDelegate> calibrationDelegate;
@property (weak, nonatomic) IBOutlet UILabel *messageLabel;

+ (RCCalibration1 *)instantiateFromQuickstartKit;

@end
