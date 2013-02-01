//
//  TMLocation.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 2/1/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <CoreData/CoreData.h>

@class TMMeasurement;

@interface TMLocation : NSManagedObject

@property (nonatomic) double accuracyInMeters;
@property (nonatomic, retain) NSString * address;
@property (nonatomic) double latititude;
@property (nonatomic, retain) NSString * locationName;
@property (nonatomic) double longitude;
@property (nonatomic, retain) NSSet *measurement;
@end

@interface TMLocation (CoreDataGeneratedAccessors)

- (void)addMeasurementObject:(TMMeasurement *)value;
- (void)removeMeasurementObject:(TMMeasurement *)value;
- (void)addMeasurement:(NSSet *)values;
- (void)removeMeasurement:(NSSet *)values;

@end
