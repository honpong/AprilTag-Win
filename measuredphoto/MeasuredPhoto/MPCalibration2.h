//
//  MPCalibration2.h
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 8/29/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "MPViewController.h"
#import <RCCore/RCCore.h>
#import "MPVideoPreview.h"

@interface MPCalibration2 : MPViewController <RCSensorFusionDelegate>

@property (weak, nonatomic) IBOutlet UIButton *button;
@property (weak, nonatomic) IBOutlet UILabel *messageLabel;
@property (weak, nonatomic) IBOutlet MPVideoPreview *videoPreview;

- (IBAction)handleButton:(id)sender;

@end
