//
//  MPAppDelegate.h
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 7/26/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <CoreLocation/CoreLocation.h>
#import <RCCore/RCCore.h>
#import "MPCalibrationVC.h"
#import <RC3DK/RC3DK.h>

@interface MPAppDelegate : UIResponder <UIApplicationDelegate, CLLocationManagerDelegate, MPCalibrationDelegate>

@property (strong, nonatomic) UIWindow *window;

@end
