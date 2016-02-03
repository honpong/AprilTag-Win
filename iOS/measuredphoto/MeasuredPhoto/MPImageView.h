//
//  MPImageView.h
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 1/24/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <CoreMedia/CoreMedia.h>

@interface MPImageView : UIImageView

- (void) setImageWithSampleBuffer:(CMSampleBufferRef)sampleBuffer;

@end
