//
//  RCDeviceInfo.h
//  RCCore
//
//  Created by Ben Hirashima on 2/20/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "sys/sysctl.h"

typedef enum
{
    DeviceTypeUnknown, DeviceTypeiPhone4s, DeviceTypeiPhone5, DeviceTypeiPad2, DeviceTypeiPad3, DeviceTypeiPad4, DeviceTypeiPadMini, DeviceTypeiPod5,
    DeviceTypeiPhone5c, DeviceTypeiPhone5s,
    DeviceTypeiPadAir, DeviceTypeiPadMiniRetina
} DeviceType;

@interface RCDeviceInfo : NSObject

+ (NSString*) getOSVersion;
+ (NSString *) getPlatformString;
+ (DeviceType) getDeviceType;
+ (float) getPhysicalScreenMetersX;

@end
