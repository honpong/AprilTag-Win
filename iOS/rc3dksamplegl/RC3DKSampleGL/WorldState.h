//
//  WorldStateManager.h
//  RC3DKSampleVis
//
//  Created by Brian on 2/28/14.
//  Copyright (c) 2014 Realitycap. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface WorldState : NSObject

typedef struct _feature {
    float x, y, z;
    float lastSeen;
    bool good;
} Feature;

typedef struct _translation {
    float x, y, z;
    float time;
} Translation;

/* RCVisDataDelegate */
- (void) observeTime:(float)time;
- (void) observeFeatureWithId:(uint64_t)id x:(float)x y:(float)y z:(float)z lastSeen:(float)lastSeen good:(bool)good;
- (void) observePathWithTranslationX:(float)x y:(float)y z:(float)z time:(float)time;
- (void) reset;

/* Get data */
- (NSDictionary *)getFeatures;
- (NSArray *)getPath;
- (float)getTime;

+ (WorldState *) sharedInstance;

@end
