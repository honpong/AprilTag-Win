//
//  RCCalibration2.h
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 8/29/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <RC3DK/RC3DK.h>

@protocol RCCalibrationDelegate;

@interface RCCalibration2 : UIViewController <RCSensorFusionDelegate>

@property (weak, nonatomic) IBOutlet UIButton *button;
@property (weak, nonatomic) IBOutlet UILabel *messageLabel;
@property (weak) id<RCCalibrationDelegate> delegate;

- (IBAction)handleButton:(id)sender;

@end
