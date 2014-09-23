//
//  MPAppDelegate.h
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 7/26/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <CoreLocation/CoreLocation.h>
#import <RCCore/RCCore.h>
#import "RCCalibration1.h"
#import "MPLocalMoviePlayer.h"

@interface MPAppDelegate : UIResponder <UIApplicationDelegate, CLLocationManagerDelegate, RCCalibrationDelegate, RCLocalMoviePlayerDelegate>

@property (strong, nonatomic) UIWindow *window;

@end
