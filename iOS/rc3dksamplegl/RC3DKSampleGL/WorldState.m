//
//  WorldStateManager.m
//  RC3DKSampleVis
//
//  Created by Brian on 2/28/14.
//  Copyright (c) 2014 Realitycap. All rights reserved.
//

#import "WorldState.h"

@implementation WorldState
{
    NSMutableDictionary * features;
    NSMutableArray * path;
    float currentTime;
}


+ (WorldState *) sharedInstance
{
    static WorldState* instance = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        instance = [WorldState new];
    });
    return instance;
}


-(id)init
{
    self = [super init];
    if(self) {
        features = [NSMutableDictionary dictionaryWithCapacity:100];
        path = [NSMutableArray arrayWithCapacity:100];
    }
    return self;
}

-(void) reset
{
    [features removeAllObjects];
    [path removeAllObjects];
}

-(void)observeTime:(float)time
{
    currentTime = time;
}

-(void)observeFeatureWithId:(uint64_t)id x:(float)x y:(float)y z:(float)z lastSeen:(float)lastSeen good:(bool)good
{
    NSNumber * key = [NSNumber numberWithUnsignedLongLong:id];

    Feature f;
    f.x = x;
    f.y = y;
    f.z = z;
    f.lastSeen = lastSeen;
    f.good = good;
    NSValue * value = [NSValue value:&f withObjCType:@encode(Feature)];
    [features setObject:value forKey:key];
}

-(void)observePathWithTranslationX:(float)x y:(float)y z:(float)z time:(float)time
{
    Translation t;
    t.x = x;
    t.y = y;
    t.z = z;
    t.time = time;

    NSValue * value = [NSValue value:&t withObjCType:@encode(Translation)];
    [path addObject:value];
}

-(NSDictionary *)getFeatures
{
    return features;
}

-(NSArray *)getPath
{
    return path;
}

-(float)getTime
{
    return currentTime;
}

@end
