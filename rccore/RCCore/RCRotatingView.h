//
//  RCRotatingView.h
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 2/6/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

@protocol RCRotatingView <NSObject>

- (void) handleOrientationChange:(UIDeviceOrientation)orientation animated:(BOOL)animated;

@end
