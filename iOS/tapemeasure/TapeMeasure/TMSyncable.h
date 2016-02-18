//
//  TMSyncable.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 3/1/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <CoreData/CoreData.h>


@interface TMSyncable : NSManagedObject

@property (nonatomic) int32_t dbid;
@property (nonatomic) BOOL deleted;
@property (nonatomic) BOOL syncPending;

@end
