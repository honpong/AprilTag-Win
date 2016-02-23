//
//  TMMeasurementType.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 4/2/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "TMConstants.h"

@interface TMMeasurementType : NSObject

@property NSString* name;
@property NSString* imageName;
@property MeasurementType type;

- (id) initWithName:(NSString*)name withImageName:(NSString*)imageName withType:(MeasurementType)type;

@end
