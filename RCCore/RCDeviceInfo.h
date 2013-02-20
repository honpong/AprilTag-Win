//
//  RCDeviceInfo.h
//  RCCore
//
//  Created by Ben Hirashima on 2/20/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface RCDeviceInfo : NSObject

+ (NSString*) getOSVersion;
+ (NSString *) getPlatformString;

@end
