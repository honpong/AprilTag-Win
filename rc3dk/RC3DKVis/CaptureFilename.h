//
//  CaptureFilename.h
//  RCUtility
//
//  Created by Brian on 4/8/15.
//  Copyright (c) 2015 Realitycap. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface CaptureFilename : NSObject

+ (NSDictionary *) parseFilename:(NSString *)filename;
+ (NSString *) filenameFromParameters:(NSDictionary *)parameters;

@end