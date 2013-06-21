//
//  RCTransformation.h
//  RCCore
//
//  Created by Ben Hirashima on 6/21/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCPosition.h"
#import "RCOrientation.h"

@interface RCTransformation : NSObject

@property (nonatomic, readonly) RCPosition* position;
@property (nonatomic, readonly) RCOrientation* orientation;

- (id) initWithPosition:(RCPosition*)position withOrientation:(RCOrientation*)orientation;

@end
