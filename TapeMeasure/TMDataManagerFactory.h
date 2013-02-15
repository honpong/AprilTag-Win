//
//  TMDataManagerFactory.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 1/18/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMMeasurement.h"

@protocol TMDataManager <NSObject>

- (NSArray*)getAllMeasurements;
- (TMMeasurement*)getNewMeasurement;
- (void)insertMeasurement:(TMMeasurement*)measurement;
- (TMMeasurement*)getMeasurementById:(int)dbid;

- (void)saveContext;
- (NSManagedObjectContext *)getManagedObjectContext;

@end

@interface TMDataManagerFactory

+ (id<TMDataManager>)getDataManagerInstance;
+ (void)setDataManagerInstance:(id<TMDataManager>)mockObject;

@end
