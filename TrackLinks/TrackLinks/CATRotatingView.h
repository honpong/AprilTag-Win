//
//  RCRotatingView.h
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 2/6/14.
//  Copyright (c) 2014 Caterpillar. All rights reserved.
//

@protocol CATRotatingView <NSObject>

- (void) handleOrientationChange:(UIDeviceOrientation)orientation animated:(BOOL)animated;

@end
