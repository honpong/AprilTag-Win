//
//  TMDataManagerFactory.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 1/18/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMDataManagerFactory.h"

@interface TMDataManagerImpl : NSObject <TMDataManager>
{
    NSManagedObjectContext *managedObjectContext;
    NSManagedObjectModel *managedObjectModel;
    NSPersistentStoreCoordinator *persistentStoreCoordinator;
}
@end

@implementation TMDataManagerImpl

- (id)init
{
    self = [super init];
    
    if (self)
    {
         [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(handleTerminate)
                                                     name:UIApplicationWillTerminateNotification
                                                   object:nil];
    }
    
    return self;
}

- (void)handleTerminate
{
    [self saveContext];
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

/** @returns YES if no error, NO if error occurred. Returns YES if no save was necessary because no changes were made. */
- (BOOL)saveContext
{
    NSError *error = nil;
    
    if (managedObjectContext != nil)
    {
        if ([managedObjectContext hasChanges])
        {
            if ([managedObjectContext save:&error])
            {
                return YES;
            }
            else
            {
                [TMAnalytics logError:@"Data.SaveContext" message:@"Save context error" error:error];
                return NO;
            }
        }
        else
        {
            return YES; //save unnecessary. report success.
        }
    }
    else
    {
        [TMAnalytics logError:@"Data.SaveContext" message:@"Managed object context is nil" error:nil];
        return NO;
    }
}

// Returns the managed object context for the application.
// If the context doesn't already exist, it is created and bound to the persistent store coordinator for the application.
- (NSManagedObjectContext *)getManagedObjectContext
{
    if (managedObjectContext != nil) {
        return managedObjectContext;
    }
    
    NSPersistentStoreCoordinator *coordinator = [self getPersistentStoreCoordinator];
    if (coordinator != nil) {
        managedObjectContext = [[NSManagedObjectContext alloc] init];
        [managedObjectContext setPersistentStoreCoordinator:coordinator];
    }
    return managedObjectContext;
}

// Returns the managed object model for the application.
// If the model doesn't already exist, it is created from the application's model.
- (NSManagedObjectModel *)getManagedObjectModel
{
    if (managedObjectModel != nil) {
        return managedObjectModel;
    }
    NSURL *modelURL = [[NSBundle mainBundle] URLForResource:DATA_MODEL_URL withExtension:@"momd"];
    managedObjectModel = [[NSManagedObjectModel alloc] initWithContentsOfURL:modelURL];
    return managedObjectModel;
}

// Returns the persistent store coordinator for the application.
// If the coordinator doesn't already exist, it is created and the application's store added to it.
- (NSPersistentStoreCoordinator *)getPersistentStoreCoordinator
{
    if (persistentStoreCoordinator != nil) {
        return persistentStoreCoordinator;
    }
    
    NSURL *storeURL = [DOCUMENTS_DIRECTORY URLByAppendingPathComponent:@"TapeMeasure.sqlite"];
    
    NSError *error = nil;
    persistentStoreCoordinator = [[NSPersistentStoreCoordinator alloc] initWithManagedObjectModel:[self getManagedObjectModel]];
    
    if (![persistentStoreCoordinator
          addPersistentStoreWithType:NSSQLiteStoreType
          configuration:nil
          URL:storeURL
          options:@{NSMigratePersistentStoresAutomaticallyOption:@YES, NSInferMappingModelAutomaticallyOption:@YES}
          error:&error])
    {
        /*
         Typical reasons for an error here include:
         * The persistent store is not accessible;
         * The schema for the persistent store is incompatible with current managed object model.
         Check the error message to determine what the actual problem was.
                  
         If the persistent store is not accessible, there is typically something wrong with the file path. Often, a file URL is pointing into the application's resources directory instead of a writeable directory.
         
         If you encounter schema incompatibility errors during development, you can reduce their frequency by:
         * Simply deleting the existing store:
         [[NSFileManager defaultManager] removeItemAtURL:storeURL error:nil]
         
         * Performing automatic lightweight migration by passing the following dictionary as the options parameter:
         @{NSMigratePersistentStoresAutomaticallyOption:@YES, NSInferMappingModelAutomaticallyOption:@YES}
         
         Lightweight migration will only work for a limited set of schema changes; consult "Core Data Model Versioning and Data Migration Programming Guide" for details.
         */
        [TMAnalytics logError:@"Data.GetPersistentStore" message:@"Error" error:error];
    }
    
    return persistentStoreCoordinator;
}

- (NSManagedObject*)getNewObjectOfType:(NSEntityDescription*)entity
{
    //here, we create the new instance of our model object, but do not yet insert it into the persistent store
    return (NSManagedObject*)[[NSManagedObject alloc] initWithEntity:entity insertIntoManagedObjectContext:nil];
}

- (NSManagedObject*)getObjectOfType:(NSEntityDescription*)entity byDbid:(int)dbid
{
    NSFetchRequest *fetchRequest = [[NSFetchRequest alloc] init];
    NSPredicate *predicate = [NSPredicate predicateWithFormat:@"(dbid = %i)", dbid];
    
    [fetchRequest setPredicate:predicate];
    [fetchRequest setEntity:entity];
    
    NSError *error;
    NSArray *data = [[self getManagedObjectContext] executeFetchRequest:fetchRequest error:&error]; 
    
    if(error)
    {
        [TMAnalytics logError:@"Data.QueryObject" message:@"Error in getObjectOfType" error:error];
    }
    
    return data.count > 0 ? data[0] : nil;
}

- (void)insertObject:(NSManagedObject*)obj
{
    [[self getManagedObjectContext] insertObject:obj];
    DLog(@"Object inserted into db");
}

- (void)deleteObject:(NSManagedObject*)obj
{
    [[self getManagedObjectContext] deleteObject:obj];
    DLog(@"Object deleted from db");
}

- (void)cleanOutDeletedOfType:(NSEntityDescription*)entity
{
    int count = 0;
    
    for (TMSyncable *obj in [self getMarkedForDeletion:entity])
    {
        if (!obj.syncPending)
        {
            [self deleteObject:obj];
            count++;
        }
    }
    
    [self saveContext];
    
    DLog(@"Cleaned out %i objects of type %@ marked for deletion", count, entity.name);
}

- (NSArray*)queryObjectsOfType:(NSEntityDescription*)entity withPredicate:(NSPredicate*)predicate
{
    NSFetchRequest *fetchRequest = [[NSFetchRequest alloc] init];
    
    if ([entity attributesByName][@"timestamp"])
    {
        NSSortDescriptor *sortDescriptor = [[NSSortDescriptor alloc] initWithKey:@"timestamp" ascending:NO];
        NSArray *descriptors = @[sortDescriptor];
        [fetchRequest setSortDescriptors:descriptors];
    }
     
    if (predicate) [fetchRequest setPredicate:predicate];
    [fetchRequest setEntity:entity];
    
    NSError *error;
    NSArray *objects = [[self getManagedObjectContext] executeFetchRequest:fetchRequest error:&error]; 
    
    if(error)
    {
        [TMAnalytics logError:@"Data.QueryObjects" message:@"Error querying objects from db" error:error];
    }
    
    return objects;
}

- (NSArray*)getMarkedForDeletion:(NSEntityDescription*)entity
{
    NSPredicate *predicate = [NSPredicate predicateWithFormat:@"(deleted = true)"];
    return [self queryObjectsOfType:entity withPredicate:predicate];
}

- (NSArray*)getAllExceptDeleted:(NSEntityDescription*)entity
{
    NSPredicate *predicate = [NSPredicate predicateWithFormat:@"(deleted = false)"];
    return [self queryObjectsOfType:entity withPredicate:predicate];
}

- (NSArray*)getAllPendingSync:(NSEntityDescription*)entity
{
    NSPredicate *predicate = [NSPredicate predicateWithFormat:@"(syncPending = true)"];
    return [self queryObjectsOfType:entity withPredicate:predicate];
}

- (void)deleteAllOfType:(NSEntityDescription*)entity
{
    NSArray *array = [self queryObjectsOfType:entity withPredicate:nil];
    
    for (NSManagedObject *obj in array)
    {
        [self deleteObject:obj];
    }
    
    [self saveContext];
}

/** This is for when the user wants to add existing measurements to their account when they log in */
- (void)markAllPendingUpload
{
    [self markAllPendingUpload:[TMMeasurement getEntity]];
    [self markAllPendingUpload:[TMLocation getEntity]];
}

- (void)markAllPendingUpload:(NSEntityDescription*)entity
{
    NSArray *array = [self queryObjectsOfType:entity withPredicate:nil];
    
    for (TMSyncable *obj in array)
    {
        obj.syncPending = YES;
        obj.dbid = 0;
    }
    
    [self saveContext];
}

- (int)getObjectCount:(NSEntityDescription*)entity
{
    NSArray *array = [self queryObjectsOfType:entity withPredicate:nil];
    return (int)array.count;
}

- (void)deleteAllData;
{
    [self deleteAllOfType:[TMMeasurement getEntity]];
    [self deleteAllOfType:[TMLocation getEntity]];
}

@end

@implementation TMDataManagerFactory

static id<TMDataManager> instance;

+ (id<TMDataManager>) getInstance
{
    if (instance == nil)
    {
        instance = [[TMDataManagerImpl alloc] init];
    }
    
    return instance;
}

//for testing. you can set this factory to return a mock object.
+ (void) setInstance:(id<TMDataManager>)mockObject
{
    instance = mockObject;
}

@end
