//
//  RCViewController.h
//  TrueMeasureSDKSampleApp
//
//  Created by Ben Hirashima on 10/16/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface RCViewController : UIViewController

@property (weak, nonatomic) IBOutlet NSLayoutConstraint *button;
- (IBAction)handleButtonPress:(id)sender;

@end
