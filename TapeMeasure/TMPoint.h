//
//  TMPoint.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 5/7/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <CoreData/CoreData.h>
#import "TMSyncable.h"

@class TMMeasurement;

@interface TMPoint : TMSyncable

@property (nonatomic) int16_t quality;
@property (nonatomic) int16_t imageX;
@property (nonatomic) int16_t imageY;
@property (nonatomic, retain) TMMeasurement *measurement;

@end
