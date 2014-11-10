//
//  ViewController.h
//  RC3DKSampleAR
//
//  Created by Eagle Jones on 10/23/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "RCSensorFusion.h"
#import "QRDelegate.h"

@interface ViewController : UIViewController <RCSensorFusionDelegate,QRDetectionDelegate>

@property (weak, nonatomic) IBOutlet UILabel *statusLabel;
@property (weak, nonatomic) IBOutlet UIProgressView *progressBar;

@end

