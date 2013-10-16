//
//  MPPhotoRequest.h
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 10/16/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface MPPhotoRequest : NSObject

@property (nonatomic, readonly) NSURL* url;
@property (nonatomic, readonly) int apiVersion;
@property (nonatomic, readonly) NSString* action;
@property (nonatomic, readonly) NSString* apiKey;
@property (nonatomic, readonly) NSString* sourceApp;
@property (nonatomic, readonly) BOOL isRepliedTo;
@property (nonatomic, readonly) NSDate* dateReceived;

+ (MPPhotoRequest*) lastRequest;
+ (void) setLastRequest:(NSURL*)url withSourceApp:(NSString*)bundleId;

- (id) initWithUrl:(NSURL*)url withSourceApp:(NSString*)bundleId;

@end
