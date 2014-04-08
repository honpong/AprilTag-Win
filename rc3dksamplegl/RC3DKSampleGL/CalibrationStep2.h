//
//  CalibrationStep2.h
//  RC3DKSampleApp
//
//  Created by Ben Hirashima on 8/29/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

@protocol CalibrationDelegate;

@interface CalibrationStep2 : UIViewController <RCSensorFusionDelegate>

@property (weak, nonatomic) IBOutlet UIButton *button;
@property (weak, nonatomic) IBOutlet UILabel *messageLabel;
@property (weak) id<CalibrationDelegate> delegate;

- (IBAction)handleButton:(id)sender;

@end
