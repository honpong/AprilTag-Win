//
//  RCCalibration1.h
//
//  Created by Ben Hirashima on 8/13/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <RC3DK/RC3DK.h>

@protocol RCCalibrationDelegate;

@interface RCCalibration1 : UIViewController <RCSensorFusionDelegate>

@property (weak, nonatomic) IBOutlet UIButton *button;
@property (weak, nonatomic) IBOutlet UILabel *messageLabel;
@property (weak) id<RCCalibrationDelegate> delegate;

- (IBAction)handleButton:(id)sender;

@end
