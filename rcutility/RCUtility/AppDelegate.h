//
//  AppDelegate.h
//  RCCapture
//
//  Created by Brian on 9/27/13.
//  Copyright (c) 2013 Realitycap. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "CalibrationStep1.h"

@interface AppDelegate : UIResponder <UIApplicationDelegate, CalibrationDelegate, CLLocationManagerDelegate>

@property (strong, nonatomic) UIWindow *window;

- (void) startFromCalibration;
- (void) startFromCapture;
- (void) startFromReplay;
- (void) startFromLive;
- (void) startFromHome;

+ (NSURL *) timeStampedURLWithSuffix:(NSString *)suffix;

@end
