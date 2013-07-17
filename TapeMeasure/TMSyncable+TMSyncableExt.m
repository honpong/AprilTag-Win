//
//  TMSyncable+TMSyncableExt.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 3/1/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMSyncable+TMSyncableExt.h"

@implementation TMSyncable (TMSyncableExt)

+ (NSEntityDescription*)getEntity
{
    return [NSEntityDescription entityForName:NSStringFromClass([self class]) inManagedObjectContext:[DATA_MANAGER getManagedObjectContext]];
}

- (void)insertIntoDb
{
    [[DATA_MANAGER getManagedObjectContext] insertObject:self];
}

- (void)deleteFromDb
{
    [[DATA_MANAGER getManagedObjectContext] deleteObject:self];
    DLog(@"%@ deleted", NSStringFromClass([self class]));
}

+ (void)cleanOutDeleted
{
    [DATA_MANAGER cleanOutDeletedOfType:[self getEntity]];
}

+ (NSArray*)query:(NSPredicate*)predicate
{
    return [DATA_MANAGER queryObjectsOfType:[self getEntity] withPredicate:predicate];
}

+ (NSArray*)getMarkedForDeletion
{
    return [DATA_MANAGER getMarkedForDeletion:[self getEntity]];
}

+ (NSArray*)getAllExceptDeleted
{
    return [DATA_MANAGER getAllExceptDeleted:[self getEntity]];
}

+ (NSArray*)getAllPendingSync
{
    return [DATA_MANAGER getAllPendingSync:[self getEntity]];
}

+ (void)deleteAllFromDb
{
    [DATA_MANAGER deleteAllOfType:[self getEntity]];
}

/** This is for when the user wants to add existing measurements to their account when they log in */
+ (void)markAllPendingUpload
{
    [DATA_MANAGER markAllPendingUpload:[self getEntity]];
}

+ (int)getCount
{
    return [DATA_MANAGER getObjectCount:[self getEntity]];
}

@end
