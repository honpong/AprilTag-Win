//
//  MPAppDelegate.h
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 7/26/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <CoreLocation/CoreLocation.h>
#import <RCCore/RCCore.h>
#import "MPCalibration1.h"
#import <TestFlight.h>

@interface MPAppDelegate : UIResponder <UIApplicationDelegate, CLLocationManagerDelegate>

@property (strong, nonatomic) UIWindow *window;

@end
