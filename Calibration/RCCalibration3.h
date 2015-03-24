//
//  RCCalibration3.h
//
//  Created by Ben Hirashima on 8/29/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCCalibration1.h"

@interface RCCalibration3 : UIViewController <RCSensorFusionDelegate>

@property (weak, nonatomic) id<RCCalibrationDelegate> calibrationDelegate;
@property (weak, nonatomic) IBOutlet UILabel *messageLabel;

@end
