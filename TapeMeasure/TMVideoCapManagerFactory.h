//
//  TMVideoCapManagerFactory.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 1/17/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <AVFoundation/AVFoundation.h>
#import <CoreImage/CoreImage.h>
#include <stdio.h>
#import <ImageIO/ImageIO.h>
#import "TMAvSessionManagerFactory.h"
#import "TMCorvisManagerFactory.h"

@protocol TMVideoCapManager <NSObject>
- (void)startVideoCap;
- (void)stopVideoCap;
@end

@interface TMVideoCapManagerFactory
+ (void)setupVideoCapManager;
+ (id<TMVideoCapManager>)getVideoCapManagerInstance;
+ (void)setVideoCapManagerInstance:(id<TMVideoCapManager>)mockObject;
@end
