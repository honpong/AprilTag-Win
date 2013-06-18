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
#import "RCMeasurementManagerDelegate.h"

@interface RCMeasurementManager : NSObject

@property (weak) id<RCMeasurementManagerDelegate> delegate;

- (void) startSensorFusion:(AVCaptureSession*)session withLocation:(CLLocation*)location;
- (void) stopSensorFusion;
- (void) startMeasuring;
- (void) stopMeasuring;

@end
