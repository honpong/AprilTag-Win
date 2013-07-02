//
//  RCMeasuredPhoto.h
//  RCCore
//
//  Created by Jordan Miller on 6/28/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>
#import "RCSensorFusion.h"

@interface RCMeasuredPhoto : NSObject
{
    
}

- (void) initPhotoMeasurement:(RCSensorFusionData*)sensorFusionInput;
- (BOOL) is_persisted;
- (NSString*) url;
- (NSString*) jsonFromRCFeaturePointArray;

@end
