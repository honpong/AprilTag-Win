//
//  LocationPermissionController.h
//  RC3DKSampleGL
//
//  Created by Ben Hirashima on 9/15/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import <UIKit/UIKit.h>

@protocol LocationPermissionControllerDelegate <NSObject>

- (void) locationPermissionResult:(BOOL)granted;

@end

@interface LocationPermissionController : UIViewController

@property (weak, nonatomic) id<LocationPermissionControllerDelegate> delegate;

@property (weak, nonatomic) IBOutlet UIButton *allowButton;
@property (weak, nonatomic) IBOutlet UIButton *neverButton;
@property (weak, nonatomic) IBOutlet UILabel *introLabel;

- (IBAction)handleAllowButton:(id)sender;
- (IBAction)handleNeverButton:(id)sender;

@end