//
//  RCDistance.h
//  RCCore
//
//  Created by Ben Hirashima on 5/23/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <Foundation/Foundation.h>

typedef enum {
    UnitsMetric = 0, UnitsImperial = 1
} Units;

//items are numbered by their order in the segmented button list. that's why there's two 0s, etc. not ideal, but convenient and workable.
typedef enum {
    UnitsScaleKM = 0, UnitsScaleM = 1, UnitsScaleCM = 2,
    UnitsScaleMI = 0, UnitsScaleYD = 1, UnitsScaleFT = 2, UnitsScaleIN = 3
} UnitsScale;

@protocol RCDistance <NSObject>

@property (readonly) float meters;
@property (readonly) UnitsScale scale;
@property (readonly) NSString* unitSymbol;

- (id) initWithMeters:(float)distance withScale:(UnitsScale)scale;

@end
