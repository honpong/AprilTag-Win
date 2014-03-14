//
//  RCDateFormatter.m
//  RCCore
//
//  Created by Ben Hirashima on 2/13/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCDateFormatter.h"

@implementation RCDateFormatter

static NSMutableDictionary *dateFormatters;

+ (NSDateFormatter *)getInstanceForFormat:(NSString*)format
{
    if (dateFormatters == nil)
    {
        dateFormatters = [NSMutableDictionary dictionaryWithObjectsAndKeys:[self initInstance:format], format, nil];
    }
    else
    {
        if (dateFormatters[format] == nil)
        {
            dateFormatters[format] = [self initInstance:format];
        }
    }
    
    return dateFormatters[format];
}

+ (NSDateFormatter *)initInstance:(NSString*)format
{
    NSDateFormatter* instance = [[NSDateFormatter alloc] init];
    [instance setDateFormat:format];
    
    NSTimeZone *timeZone = [NSTimeZone timeZoneWithName:@"UTC"];
    [instance setTimeZone:timeZone];
    
    return instance;
}

@end
