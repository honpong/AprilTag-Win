//
//  TMMeasuredPhoto.h
//  TrueMeasureSDK
//
//  Created by Ben Hirashima on 10/15/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <Foundation/Foundation.h>

extern NSString *kTMMeasuredPhotoUTI;

@interface TMMeasuredPhoto : NSObject <NSCoding>

@property (nonatomic) NSString* appVersion;
@property (nonatomic) NSNumber* appBuildNumber;

- (NSData*) dataRepresentation;
+ (TMMeasuredPhoto*) unarchivePackageData:(NSData *)data;

@end
