//
//  RCDateFormatter.h
//  RCCore
//
//  Created by Ben Hirashima on 2/13/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

/** Instatiating NSDateFormatter is expensive, so we cache it in this singleton */
@interface RCDateFormatter : NSObject

+ (NSDateFormatter *)getInstanceForFormat:(NSString*)format;

@end
