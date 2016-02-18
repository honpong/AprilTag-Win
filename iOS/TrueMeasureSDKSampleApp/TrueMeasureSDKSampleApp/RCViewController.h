//
//  RCViewController.h
//  TrueMeasureSDKSampleApp
//
//  Created by Ben Hirashima on 10/16/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <TrueMeasureSDK/TrueMeasureSDK.h>

@interface RCViewController : UIViewController

@property (weak, nonatomic) IBOutlet NSLayoutConstraint *button;
@property (weak, nonatomic) IBOutlet UIImageView *imageView;

- (IBAction)handleButtonPress:(id)sender;
- (void) setMeasuredPhoto:(TMMeasuredPhoto*)measuredPhoto;

@end
