//
//  AppDelegate.h
//  RC3DKSampleApp
//
//  Created by Ben Hirashima on 7/19/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "CalibrationStep1.h"

@interface AppDelegate : UIResponder <UIApplicationDelegate, CalibrationDelegate, CLLocationManagerDelegate>

@property (strong, nonatomic) UIWindow *window;

@end
