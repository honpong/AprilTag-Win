//
//  RCScalar.m
//  RCCore
//
//  Created by Eagle Jones on 6/27/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCScalar.h"

@implementation RCScalar

- (id) initWithScalar:(float)scalar withStdDev:(float)stdDev
{
    if(self = [super init])
    {
        _scalar = scalar;
        _standardDeviation = stdDev;
    }
    return self;
}

- (NSDictionary*) dictionaryRepresentation
{
    //create a dictionary and add the two memebers of this class as floats
    NSMutableDictionary *tmpDic = [NSMutableDictionary dictionaryWithCapacity:2];
    [tmpDic setObject:[NSNumber numberWithFloat:self.scalar] forKey:@"scalar"];
    [tmpDic setObject:[NSNumber numberWithFloat:self.standardDeviation] forKey:@"standardDeviation"];
    
    //we return an immutable version
    return [NSDictionary dictionaryWithDictionary:tmpDic];
}

@end
