//
//  TMMeasurement+TMMeasurementExt.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 1/23/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMMeasurement+TMMeasurementExt.h"

@implementation TMMeasurement (TMMeasurementExt)

#pragma mark - Database operations

+ (TMMeasurement*)getNewMeasurement
{
    //here, we create the new instance of our model object, but do not yet insert it into the persistent store
    NSEntityDescription *entity = [NSEntityDescription entityForName:ENTITY_MEASUREMENT inManagedObjectContext:[DATA_MANAGER getManagedObjectContext]];
    TMMeasurement *m = (TMMeasurement*)[[NSManagedObject alloc] initWithEntity:entity insertIntoManagedObjectContext:nil];
    m.units = [[NSUserDefaults standardUserDefaults] integerForKey:PREF_UNITS];
    return m;
}

+ (TMMeasurement*)getMeasurementById:(int)dbid
{
    NSFetchRequest *fetchRequest = [[NSFetchRequest alloc] init];
    NSEntityDescription *entity = [NSEntityDescription entityForName:ENTITY_MEASUREMENT inManagedObjectContext:[DATA_MANAGER getManagedObjectContext]];
    
    NSPredicate *predicate = [NSPredicate predicateWithFormat:@"(dbid = %i)", dbid];
    
    [fetchRequest setPredicate:predicate];
    [fetchRequest setEntity:entity];
    
    NSError *error;
    NSArray *measurementsData = [[DATA_MANAGER getManagedObjectContext] executeFetchRequest:fetchRequest error:&error]; //TODO: Handle fetch error
    
    if(error)
    {
        NSLog(@"Error fetching measurement with id %i: %@", dbid, [error localizedDescription]);
    }
    
    return measurementsData.count > 0 ? measurementsData[0] : nil; //TODO:error handling
}

- (void)insertMeasurement
{
    [[DATA_MANAGER getManagedObjectContext] insertObject:self];
}

- (void)deleteMeasurement
{
    [[DATA_MANAGER getManagedObjectContext] deleteObject:self];
    NSLog(@"Measurement deleted");
}

+ (void)cleanOutDeleted
{
    int count = 0;
    
    for (TMMeasurement *m in [TMMeasurement getMarkedForDeletion])
    {
        if (!m.syncPending)
        {
            [m deleteMeasurement];
            count++;
        }
    }
    
    [DATA_MANAGER saveContext];
    
    NSLog(@"Cleaned out %i measurements marked for deletion", count);
}

+ (NSArray*)queryMeasurements:(NSPredicate*)predicate
{
    NSFetchRequest *fetchRequest = [[NSFetchRequest alloc] init];
    NSEntityDescription *entity = [NSEntityDescription entityForName:ENTITY_MEASUREMENT inManagedObjectContext:[DATA_MANAGER getManagedObjectContext]];
    
    NSSortDescriptor *sortDescriptor = [[NSSortDescriptor alloc] initWithKey:@"timestamp" ascending:NO];
    NSArray *descriptors = [[NSArray alloc] initWithObjects:sortDescriptor, nil];
    
    [fetchRequest setSortDescriptors:descriptors];
    [fetchRequest setPredicate:predicate];
    [fetchRequest setEntity:entity];
    
    NSError *error;
    NSArray *measurementsData = [[DATA_MANAGER getManagedObjectContext] executeFetchRequest:fetchRequest error:&error]; //TODO: Handle fetch error
    
    if(error)
    {
        NSLog(@"Error fetching measurements from db: %@", [error localizedDescription]);
    }
    
    return measurementsData;
}

+ (NSArray*)getMarkedForDeletion
{
    NSPredicate *predicate = [NSPredicate predicateWithFormat:@"(deleted = true)"];
    return [self queryMeasurements:predicate];
}

+ (NSArray*)getAllExceptDeleted
{
    NSPredicate *predicate = [NSPredicate predicateWithFormat:@"(deleted = false)"];
    return [self queryMeasurements:predicate];
}

+ (NSArray*)getAllPendingSync
{
    NSPredicate *predicate = [NSPredicate predicateWithFormat:@"(syncPending = true)"];
    return [self queryMeasurements:predicate];
}

#pragma mark - Misc

/** Gets formatted distance string according to this instance's properties, such as units and units scale */
- (NSString*)getFormattedDistance:(float)meters
{
    if((Units)self.units == UnitsImperial)
    {
        return [RCDistanceFormatter getFormattedDistance:meters
                                               withUnits:self.units
                                               withScale:self.unitsScaleImperial
                                          withFractional:self.fractional];
    }
    else
    {
        return [RCDistanceFormatter getFormattedDistance:meters
                                               withUnits:self.units
                                               withScale:self.unitsScaleMetric
                                          withFractional:self.fractional];
    }
}

@end
