//
//  TMFeatureLayerManager.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 5/3/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMFeatureLayerDelegate.h"

@interface TMFeatureLayerManager : NSObject

- (id) initWithLayerCount:(int)count;

@property CALayer* featureLayer;

@end
