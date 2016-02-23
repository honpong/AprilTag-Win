//
//  RCMeasuredPhoto.h
//  RCCore
//
//  Created by Jordan Miller on 6/28/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>
#import "RCUserManager.h"
#import "RCHttpClient.h"
#import "RCDateFormatter.h"
#import "RC3DK.h"

@interface RCMeasuredPhoto : NSObject

//made these public properties in order to facilitate unit testing. 
@property    NSData      *imageData;
@property    NSString    *fileName;
@property    NSArray     *featurePoints;
@property    NSURL       *persistedUrl;
@property    BOOL        is_persisted;
@property    NSString    *pngFileName;
@property    NSString    *bundleID;
@property    NSString    *vendorUniqueId;
@property    NSString    *jsonRepresntation;
@property    NSDate      *timestamp;
@property    NSInteger   statusCode;
//TODO put unique user identifiers in here, as well as client company identifiers, timestamps, etc - necessary for url gen

- (void) initPhotoMeasurement:(RCSensorFusionData*)sensorFusionInput;
- (NSString*) jsonRepresenation;
- (NSDictionary*) dictionaryRepresentation;
- (void) setIdentifiers;
- (void) upLoad:(void (^)())successBlock onFailure:(void (^)(NSInteger statusCode))failureBlock;

@end
