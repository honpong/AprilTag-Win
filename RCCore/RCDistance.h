//
//  RCDistance.h
//  RCCore
//
//  Created by Ben Hirashima on 5/23/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <Foundation/Foundation.h>

@protocol RCDistance <NSObject>

@property float meters;
@property UnitsScale scale;
@property NSString* unitSymbol;

- (id) initWithMeters:(float)distance withScale:(UnitsScale)scale;
- (NSString*) getString;

+ (UnitsScale)autoSelectUnitsScale:(float)meters withUnits:(Units)units;

@end
