//
//  TMAppDelegate.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 10/2/12.
//  Copyright (c) 2012 RealityCap. All rights reserved.
//

#import "RCCore/RCUserManager.h"
#import <RC3DK/RC3DK.h>
#import "TMMeasurement.h"
#import "TMMeasurement+TMMeasurementExt.h"
#import "Flurry.h"
#import "TMLocationIntro.h"

@interface TMAppDelegate : UIResponder <UIApplicationDelegate, CLLocationManagerDelegate, TMLocationIntroDelegate>

@property (strong, nonatomic) UIWindow *window;

@end
