//
//  TMAppDelegate.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 10/2/12.
//  Copyright (c) 2012 RealityCap. All rights reserved.
//

#import "TMMeasurement.h"
#import "TMMeasurement+TMMeasurementExt.h"
#import "Flurry.h"
#import "TMIntroScreen.h"
#import "TMLocalMoviePlayer.h"

@interface TMAppDelegate : UIResponder <UIApplicationDelegate, CLLocationManagerDelegate, RCLocalMoviePlayerDelegate>

@property (strong, nonatomic) UIWindow *window;

@end
