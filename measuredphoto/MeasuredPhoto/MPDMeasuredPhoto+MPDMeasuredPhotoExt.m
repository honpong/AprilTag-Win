//
//  MPDMeasuredPhoto+MPDMeasuredPhotoExt.m
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 7/8/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "MPDMeasuredPhoto+MPDMeasuredPhotoExt.h"

@implementation MPDMeasuredPhoto (MPDMeasuredPhotoExt)

- (void) awakeFromInsert
{
    [super awakeFromInsert];
    NSString* guid = [[NSUUID UUID] UUIDString];
    self.id_guid = guid;
    self.current_version_guid = [[NSUUID UUID] UUIDString];
    self.measured_at = [[NSDate date] timeIntervalSince1970];
}

@end
