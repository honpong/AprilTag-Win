//
//  RCDeviceInfo.m
//  RCCore
//
//  Created by Ben Hirashima on 2/20/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCDeviceInfo.h"
#import "RCDebugLog.h"

@implementation RCDeviceInfo

+ (NSString*) getOSVersion
{
    return [[UIDevice currentDevice] systemVersion];
}

+ (NSString *) getPlatformString {
    // Gets a string with the device model
    size_t size;
    sysctlbyname("hw.machine", NULL, &size, NULL, 0);
    char *machine = malloc(size);
    sysctlbyname("hw.machine", machine, &size, NULL, 0);
    NSString *platform = @(machine);
    free(machine);
    
    return platform;
}

//Update from: https://theiphonewiki.com/wiki/Models
+ (DeviceType) getDeviceType
{
    NSString *platform = [self getPlatformString];

    if ([platform isEqualToString:@"iPhone1,1"])    return DeviceTypeUnknown; //iPhone 1
    if ([platform isEqualToString:@"iPhone1,2"])    return DeviceTypeUnknown; //iPhone 3G
    if ([platform isEqualToString:@"iPhone2,1"])    return DeviceTypeUnknown; //iPhone 3GS
    if ([platform isEqualToString:@"iPhone3,1"])    return DeviceTypeUnknown; //iPhone 4
    if ([platform isEqualToString:@"iPhone3,2"])    return DeviceTypeUnknown; //iPhone 4
    if ([platform isEqualToString:@"iPhone3,3"])    return DeviceTypeUnknown; //iPhone 4
    if ([platform isEqualToString:@"iPhone4,1"])    return DeviceTypeiPhone4s;
    if ([platform isEqualToString:@"iPhone5,1"])    return DeviceTypeiPhone5;
    if ([platform isEqualToString:@"iPhone5,2"])    return DeviceTypeiPhone5;
    if ([platform isEqualToString:@"iPhone5,3"])    return DeviceTypeiPhone5c;
    if ([platform isEqualToString:@"iPhone5,4"])    return DeviceTypeiPhone5c;
    if ([platform isEqualToString:@"iPhone6,1"])    return DeviceTypeiPhone5s;
    if ([platform isEqualToString:@"iPhone6,2"])    return DeviceTypeiPhone5s;
    if ([platform isEqualToString:@"iPhone7,1"])    return DeviceTypeiPhone6Plus;
    if ([platform isEqualToString:@"iPhone7,2"])    return DeviceTypeiPhone6;

    if ([platform isEqualToString:@"iPod1,1"])      return DeviceTypeUnknown; //iPod touch 1
    if ([platform isEqualToString:@"iPod2,1"])      return DeviceTypeUnknown; //iPod touch 2
    if ([platform isEqualToString:@"iPod3,1"])      return DeviceTypeUnknown; //iPod touch 3
    if ([platform isEqualToString:@"iPod4,1"])      return DeviceTypeUnknown; //iPod touch 4
    if ([platform isEqualToString:@"iPod5,1"])      return DeviceTypeiPod5;

    if ([platform isEqualToString:@"iPad1,1"])      return DeviceTypeUnknown; //iPad 1
    if ([platform isEqualToString:@"iPad1,2"])      return DeviceTypeUnknown; //iPad 1
    if ([platform isEqualToString:@"iPad2,1"])      return DeviceTypeiPad2;
    if ([platform isEqualToString:@"iPad2,2"])      return DeviceTypeiPad2;
    if ([platform isEqualToString:@"iPad2,3"])      return DeviceTypeiPad2;
    if ([platform isEqualToString:@"iPad2,4"])      return DeviceTypeiPad2;
    if ([platform isEqualToString:@"iPad2,5"])      return DeviceTypeiPadMini;
    if ([platform isEqualToString:@"iPad2,6"])      return DeviceTypeiPadMini;
    if ([platform isEqualToString:@"iPad2,7"])      return DeviceTypeiPadMini;
    if ([platform isEqualToString:@"iPad3,1"])      return DeviceTypeiPad3;
    if ([platform isEqualToString:@"iPad3,2"])      return DeviceTypeiPad3; 
    if ([platform isEqualToString:@"iPad3,3"])      return DeviceTypeiPad3; 
    if ([platform isEqualToString:@"iPad3,4"])      return DeviceTypeiPad4;
    if ([platform isEqualToString:@"iPad3,5"])      return DeviceTypeiPad4;
    if ([platform isEqualToString:@"iPad3,6"])      return DeviceTypeiPad4;
    if ([platform isEqualToString:@"iPad4,1"])      return DeviceTypeiPadAir;
    if ([platform isEqualToString:@"iPad4,2"])      return DeviceTypeiPadAir;
    if ([platform isEqualToString:@"iPad4,3"])      return DeviceTypeiPadAir;
    if ([platform isEqualToString:@"iPad4,4"])      return DeviceTypeiPadMiniRetina;
    if ([platform isEqualToString:@"iPad4,5"])      return DeviceTypeiPadMiniRetina;
    if ([platform isEqualToString:@"iPad4,6"])      return DeviceTypeiPadMiniRetina;
    if ([platform isEqualToString:@"iPad4,7"])      return DeviceTypeiPadMiniRetina2;
    if ([platform isEqualToString:@"iPad4,8"])      return DeviceTypeiPadMiniRetina2;
    if ([platform isEqualToString:@"iPad4,9"])      return DeviceTypeiPadMiniRetina2;
    //iPad5,1 and 5,2 were mysteriously skipped? But keep them in here as they will likely show up
    if ([platform isEqualToString:@"iPad5,1"])      return DeviceTypeiPadAir2;
    if ([platform isEqualToString:@"iPad5,2"])      return DeviceTypeiPadAir2;
    if ([platform isEqualToString:@"iPad5,3"])      return DeviceTypeiPadAir2;
    if ([platform isEqualToString:@"iPad5,4"])      return DeviceTypeiPadAir2;

    if ([platform isEqualToString:@"i386"])         return DeviceTypeUnknown;
    if ([platform isEqualToString:@"x86_64"])       return DeviceTypeUnknown;

    return DeviceTypeUnknown;
}

+ (float) getPhysicalScreenMetersX
{
    // we only need to include cases that differ from the default case
    switch ([RCDeviceInfo getDeviceType])
    {
        case DeviceTypeiPadMini:
            return 0.055f;
        case DeviceTypeiPadMiniRetina:
            return 0.055f;
        case DeviceTypeiPadMiniRetina2:
            return 0.055f;
        case DeviceTypeiPhone6:
            return 0.05852f;
        case DeviceTypeiPhone6Plus:
            return 0.06848f;
        default:
            return UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad ? 0.061f : 0.050f;
    }
}

+ (uint64_t) getFreeDiskSpaceInBytes
{
    uint64_t totalSpace = 0;
    uint64_t totalFreeSpace = 0;
    NSError *error = nil;
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSDictionary *dictionary = [[NSFileManager defaultManager] attributesOfFileSystemForPath:[paths lastObject] error: &error];
    
    if (dictionary)
    {
        NSNumber *fileSystemSizeInBytes = [dictionary objectForKey: NSFileSystemSize];
        NSNumber *freeFileSystemSizeInBytes = [dictionary objectForKey:NSFileSystemFreeSize];
        totalSpace = [fileSystemSizeInBytes unsignedLongLongValue];
        totalFreeSpace = [freeFileSystemSizeInBytes unsignedLongLongValue];
        DLog(@"Memory Capacity of %llu MiB with %llu MiB Free memory available.", ((totalSpace/1024ll)/1024ll), ((totalFreeSpace/1024ll)/1024ll));
    }
    else
    {
        DLog(@"Error Obtaining System Memory Info: Domain = %@, Code = %ld", [error domain], (long)[error code]);
    }
    
    return totalFreeSpace;
}

@end
