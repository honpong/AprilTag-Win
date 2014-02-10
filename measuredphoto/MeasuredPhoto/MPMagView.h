//
//  MPMagView.h
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 1/20/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <CoreMedia/CoreMedia.h>

@interface MPMagView : UIView

@property (nonatomic) UIImageView* photo;

- (void) setPhotoWithSampleBuffer:(CMSampleBufferRef)sampleBuffer;

@end
