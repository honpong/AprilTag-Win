//
//  RCMeasurementManager.h
//  RCCore
//
//  Created by Ben Hirashima on 6/14/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <CoreLocation/CoreLocation.h>
#import "RCCorvisManagerFactory.h"
#import "RCVideoCapManagerFactory.h"
#import "RCMotionCapManagerFactory.h"
#import "RCLocationManagerFactory.h"

@interface RCMeasurementManager : NSObject

- (id) initWithDelegate:(id<RCCorvisManagerDelegate>)delegate;
- (void) startDataCapture:(CLLocation*)location;
- (void) shutdownDataCapture;
- (void) startMeasuring;
- (void) stopMeasuring;
- (void) setDelegate:(id<RCCorvisManagerDelegate>)delegate;

@end
