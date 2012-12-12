//
//  TMLocation.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 12/12/12.
//  Copyright (c) 2012 RealityCap. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <CoreData/CoreData.h>

@class TMMeasurement;

@interface TMLocation : NSManagedObject

@property (nonatomic, retain) NSNumber * accuracyInMeters;
@property (nonatomic, retain) NSNumber * latititude;
@property (nonatomic, retain) NSString * locationName;
@property (nonatomic, retain) NSNumber * longitude;
@property (nonatomic, retain) NSString * address;
@property (nonatomic, retain) NSSet *measurement;
@end

@interface TMLocation (CoreDataGeneratedAccessors)

- (void)addMeasurementObject:(TMMeasurement *)value;
- (void)removeMeasurementObject:(TMMeasurement *)value;
- (void)addMeasurement:(NSSet *)values;
- (void)removeMeasurement:(NSSet *)values;

@end
