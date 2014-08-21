//
//  MPDLocation.h
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 7/9/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <CoreData/CoreData.h>

@class MPDMeasuredPhoto;

@interface MPDLocation : NSManagedObject

@property (nonatomic, retain) NSString * location_name;
@property (nonatomic, retain) NSString * address;
@property (nonatomic, retain) NSString * id_guid;
@property (nonatomic, retain) NSString * state;
@property (nonatomic, retain) NSString * country_code;
@property (nonatomic, retain) NSString * postal_code;
@property (nonatomic) float latitude;
@property (nonatomic) float longitude;
@property (nonatomic) NSTimeInterval created_at;
@property (nonatomic, retain) NSSet *measuredPhotos;
@end

@interface MPDLocation (CoreDataGeneratedAccessors)

- (void)addMeasuredPhotosObject:(MPDMeasuredPhoto *)value;
- (void)removeMeasuredPhotosObject:(MPDMeasuredPhoto *)value;
- (void)addMeasuredPhotos:(NSSet *)values;
- (void)removeMeasuredPhotos:(NSSet *)values;

@end
