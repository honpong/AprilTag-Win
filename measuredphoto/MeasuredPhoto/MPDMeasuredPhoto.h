//
//  MPDMeasuredPhoto.h
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 7/9/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <CoreData/CoreData.h>

@class MPDLocation;

@interface MPDMeasuredPhoto : NSManagedObject

@property (nonatomic, retain) NSString * id_guid;
@property (nonatomic) NSTimeInterval created_at;
@property (nonatomic, retain) NSString * name;
@property (nonatomic, retain) NSString * location_guid;
@property (nonatomic, retain) MPDLocation *location;

@end
