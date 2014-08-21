//
//  MPOrientationChangeData.h
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 8/1/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface MPOrientationChangeData : NSObject

@property (nonatomic) UIDeviceOrientation orientation;
@property (nonatomic) BOOL animated;

+ (MPOrientationChangeData*) dataWithOrientation:(UIDeviceOrientation)orientation animated:(BOOL)animated;
- (id) initWithOrientation:(UIDeviceOrientation)orientation animated:(BOOL)animated;

@end
