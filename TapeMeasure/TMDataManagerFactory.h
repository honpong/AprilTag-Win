//
//  TMDataManagerFactory.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 1/18/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMMeasurement.h"

@protocol TMDataManager <NSObject>

- (TMMeasurement*)getNewMeasurement;
- (void)insertMeasurement:(TMMeasurement*)measurement;
- (void)saveContext;
- (NSManagedObjectContext *)getManagedObjectContext;

@end

@interface TMDataManagerFactory

+ (id<TMDataManager>)getDataManagerInstance;
+ (void)setDataManagerInstance:(id<TMDataManager>)mockObject;

@end
