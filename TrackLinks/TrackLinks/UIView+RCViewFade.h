//
//  UIView+RCViewFade.h
//  RCCore
//
//  Created by Ben Hirashima on 5/6/14.
//  Copyright (c) 2014 Caterpillar. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface UIView (RCViewFade)

-(void)fadeOutWithDuration:(NSTimeInterval)duration andWait:(NSTimeInterval)wait;
-(void)fadeInWithDuration:(NSTimeInterval)duration withAlpha:(float)alpha andWait:(NSTimeInterval)wait;
-(void)fadeInWithDuration:(NSTimeInterval)duration andWait:(NSTimeInterval)wait;

@end
