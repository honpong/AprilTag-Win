//
//  TMDataManagerFactory.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 1/18/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMMeasurement+TMMeasurementExt.h"
#import "TMLocation+TMLocationExt.h"
#import "TMSyncable.h"

@protocol TMDataManager <NSObject>

- (BOOL)saveContext;
- (NSManagedObjectContext *)getManagedObjectContext;

- (NSManagedObject*)getNewObjectOfType:(NSEntityDescription*)entity;
- (NSManagedObject*)getObjectOfType:(NSEntityDescription*)entity byDbid:(int)dbid;
- (void)insertObject:(NSManagedObject*)obj;
- (void)deleteObject:(NSManagedObject*)obj;
- (void)cleanOutDeletedOfType:(NSEntityDescription*)entity;
- (NSArray*)queryObjectsOfType:(NSEntityDescription*)entity withPredicate:(NSPredicate*)predicate;
- (NSArray*)getMarkedForDeletion:(NSEntityDescription*)entity;
- (NSArray*)getAllExceptDeleted:(NSEntityDescription*)entity;
- (NSArray*)getAllPendingSync:(NSEntityDescription*)entity;
- (void)deleteAllOfType:(NSEntityDescription*)entity;
- (void)markAllPendingUpload;
- (void)markAllPendingUpload:(NSEntityDescription*)entity;
- (int)getObjectCount:(NSEntityDescription*)entity;
- (void)deleteAllData;

@end

@interface TMDataManagerFactory : NSObject 

+ (id<TMDataManager>) getInstance;
+ (void) setInstance:(id<TMDataManager>)mockObject;

@end
