//  CATOrientationChangeData.h
//  TrackLinks
//
//  Created by Ben Hirashima on 8/1/14.
//  Copyright (c) 2014 Caterpillar. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

@interface CATOrientationChangeData : NSObject

@property (nonatomic) UIDeviceOrientation orientation;
@property (nonatomic) BOOL animated;

+ (CATOrientationChangeData*) dataWithOrientation:(UIDeviceOrientation)orientation animated:(BOOL)animated;
- (id) initWithOrientation:(UIDeviceOrientation)orientation animated:(BOOL)animated;

@end
