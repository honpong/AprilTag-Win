//
//  AppDelegate.h
//  RCCapture
//
//  Created by Brian on 9/27/13.
//  Copyright (c) 2013 Realitycap. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "RCCalibration1.h"

@interface AppDelegate : UIResponder <UIApplicationDelegate, RCCalibrationDelegate, CLLocationManagerDelegate>

@property (strong, nonatomic) UIWindow *window;

- (void) startFromCalibration;
- (void) startFromCapture;
- (void) startFromReplay;
- (void) startFromLive;
- (void) startFromHome;

+ (NSURL *) timeStampedURLWithSuffix:(NSString *)suffix;

@end
