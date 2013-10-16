//
//  TMMeasuredPhoto.m
//  TrueMeasureSDK
//
//  Created by Ben Hirashima on 10/15/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMMeasuredPhoto.h"

NSString* kTMMeasuredPhotoUTI = @"com.realitycap.truemeasure.measuredphoto";
static NSString* kTMKeyMeasuredPhotoData = @"kTMKeyMeasuredPhotoData";
static NSString* kTMKeyAppVersion = @"kTMKeyAppVersion";
static NSString* kTMKeyAppBuildNumber = @"kTMKeyAppBuildNumber";

@implementation TMMeasuredPhoto

#pragma mark - NSCoding

- (void) encodeWithCoder:(NSCoder *)encoder
{
    [encoder encodeObject:self.appVersion forKey:kTMKeyAppVersion];
    [encoder encodeObject:self.appBuildNumber forKey:kTMKeyAppBuildNumber];
}

- (id) initWithCoder:(NSCoder *)decoder
{
    NSString* appVersion = [decoder decodeObjectForKey:kTMKeyAppVersion];
    NSNumber* appBuild = [decoder decodeObjectForKey:kTMKeyAppBuildNumber];
    
    TMMeasuredPhoto* mp = [TMMeasuredPhoto new];
    mp.appVersion = appVersion;
    mp.appBuildNumber = appBuild;
    
    return mp;
}

#pragma mark - Data Helpers

- (NSData*) dataRepresentation
{
    NSMutableData *data = [[NSMutableData alloc] init];
    NSKeyedArchiver *archiver = [[NSKeyedArchiver alloc] initForWritingWithMutableData:data];
    [archiver encodeObject:self forKey:kTMKeyMeasuredPhotoData];
    [archiver finishEncoding];
    
    return [NSData dataWithData:data];
}

+ (TMMeasuredPhoto*) unarchivePackageData:(NSData *)data
{
    NSKeyedUnarchiver *unarchiver = [[NSKeyedUnarchiver alloc] initForReadingWithData:data];
    TMMeasuredPhoto *measuredPhoto = [unarchiver decodeObjectForKey:kTMKeyMeasuredPhotoData];
    [unarchiver finishDecoding];
    
    return measuredPhoto;
}

@end
