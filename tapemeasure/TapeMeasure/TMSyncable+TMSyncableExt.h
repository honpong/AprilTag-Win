//
//  TMSyncable+TMSyncableExt.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 3/1/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMSyncable.h"
#import "TMDataManagerFactory.h"

@interface TMSyncable (TMSyncableExt)

+ (NSEntityDescription*)getEntity;
- (void)insertIntoDb;
- (void)deleteFromDb;
+ (void)cleanOutDeleted;
+ (NSArray*)getAllPendingSync;
+ (NSArray*)getAllExceptDeleted;
+ (void)deleteAllFromDb;
+ (void)markAllPendingUpload;
+ (int)getCount;

@end
