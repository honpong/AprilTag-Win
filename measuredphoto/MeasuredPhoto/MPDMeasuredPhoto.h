//
//  MPDMeasuredPhoto.h
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 7/8/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <CoreData/CoreData.h>

@class MPDLocation;

@interface MPDMeasuredPhoto : NSManagedObject

@property (nonatomic, retain) NSString * id_guid;
@property (nonatomic, retain) NSString * current_version_guid;
@property (nonatomic, retain) NSString * parent_version_guid;
@property (nonatomic) NSTimeInterval measured_at;
@property (nonatomic, retain) NSString * name;
@property (nonatomic, retain) NSString * location_guid;
@property (nonatomic, retain) MPDLocation *location;

@end
