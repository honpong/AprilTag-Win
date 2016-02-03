//
//  TMMeasurementType.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 4/2/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMMeasurementType.h"

@implementation TMMeasurementType

- (id) initWithName:(NSString*)name withImageName:(NSString*)imageName withType:(MeasurementType)type
{
    if (self = [super init])
    {
        self.name = name;
        self.imageName = imageName;
        self.type = type;
    }
    
    return self;
}

@end
