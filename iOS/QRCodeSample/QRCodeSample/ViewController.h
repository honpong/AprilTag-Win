//
//  ViewController.h
//  QRCodeSample
//
//  Created by Ben Hirashima on 5/8/15.
//  Copyright (c) 2015 RealityCap. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "RC3DK.h"

@interface ViewController : UIViewController <RCSensorFusionDelegate>

@property (weak, nonatomic) IBOutlet UILabel *statusLabel;
@property (weak, nonatomic) IBOutlet UILabel *distanceLabel;
@property (weak, nonatomic) IBOutlet UIButton *button;
- (IBAction)handleButtonTap:(id)sender;

@end

