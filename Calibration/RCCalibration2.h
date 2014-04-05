//
//  MPCalibration2.h
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 8/29/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <RC3DK/RC3DK.h>
#import "RCCalibration1.h"

@interface RCCalibration2 : UIViewController <RCSensorFusionDelegate>

@property (weak, nonatomic) id<RCCalibrationDelegate> delegate;
@property (weak, nonatomic) IBOutlet UIButton *button;
@property (weak, nonatomic) IBOutlet UILabel *messageLabel;
@property (nonatomic) IBOutlet RCVideoPreview *videoPreview;

- (IBAction)handleButton:(id)sender;

@end
