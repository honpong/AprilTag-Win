//
//  ViewController.h
//  CalibrationSample
//
//  Created by Ben Hirashima on 5/6/15.
//  Copyright (c) 2015 RealityCap. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "RC3DK.h"

@interface ViewController : UIViewController <RCSensorFusionDelegate>

@property (weak, nonatomic) IBOutlet UILabel *statusLabel;

@end

