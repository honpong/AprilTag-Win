//
//  RCTransformation.h
//  RCCore
//
//  Created by Ben Hirashima on 6/21/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCTranslation.h"
#import "RCRotation.h"

@interface RCTransformation : NSObject

@property (nonatomic, readonly) RCTranslation* translation;
@property (nonatomic, readonly) RCRotation* rotation;

- (id) initWithTranslation:(RCTranslation*)translation withRotation:(RCRotation*)rotation;
- (void) getOpenGLMatrix:(float[16])matrix;
- (RCPoint *) transformPoint:(RCPoint *)point;
- (RCTransformation *) getInverse;
- (RCTransformation *) composeWithTransformation:(RCTransformation *)other;

@end
