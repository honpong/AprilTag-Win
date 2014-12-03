//  CATInstructionsView.m
//  TrackLinks
//
//  Created by Ben Hirashima on 2/4/14.
//  Copyright (c) 2014 Caterpillar. All rights reserved.
//

#import "CATInstructionsView.h"
#import "MPCircleLayerDelegate.h"
#import "MPDotLayerDelegate.h"
#import "CATCapturePhoto.h"
#import <RCCore/RCCore.h>

const static CGFloat lineWidth = 10.; //TODO: this is hardcoded in subviews
const static float smoothing = .5;

@implementation CATInstructionsView
{
    CALayer* circleLayer;
    CALayer* dotLayer;
    MPCircleLayerDelegate* circleLayerDel;
    MPDotLayerDelegate* dotLayerDel;
    float circleRadius;
    float dotX, dotY;
}
@synthesize delegate;

- (id) initWithCoder:(NSCoder *)aDecoder
{
    if (self = [super initWithCoder:aDecoder])
    {
        circleRadius = (self.bounds.size.width - lineWidth) / 2;
        self.translatesAutoresizingMaskIntoConstraints = NO;
        
        circleLayer = [CALayer new];
        circleLayerDel = [MPCircleLayerDelegate new];
        circleLayer.delegate = circleLayerDel;
        [self.layer addSublayer:circleLayer];
        
        dotLayer = [CALayer new];
        dotLayerDel = [MPDotLayerDelegate new];
        dotLayer.delegate = dotLayerDel;
        [self.layer addSublayer:dotLayer];
        [circleLayer setNeedsDisplay];
        [dotLayer setNeedsDisplay];
    }
    return self;
}

- (void) layoutSubviews
{
    [super layoutSubviews];
 
    CGFloat const xCenter = self.frame.size.width / 2;
    CGFloat const yCenter = self.frame.size.height / 2;
    circleLayer.frame = CGRectMake(xCenter - circleRadius, yCenter - circleRadius, circleRadius * 2., circleRadius * 2.);
    [circleLayer setNeedsDisplay];
  
    dotLayer.frame = CGRectMake(dotX, dotY, self.bounds.size.width, self.bounds.size.height);
}

- (void) moveDotToCenter
{
    dotX = dotY = 0.;
}

- (void) updateDotPosition:(RCTransformation*)transformation withDepth:(float)depth
{
//    DLog("%0.1f %0.1f %0.1f", transformation.translation.x, transformation.translation.y, transformation.translation.z);
    
    RCTranslation *trans = [[transformation.rotation getInverse] transformTranslation:transformation.translation];
    float distFromStartPoint = sqrt(transformation.translation.x * transformation.translation.x + transformation.translation.y * transformation.translation.y + transformation.translation.z * transformation.translation.z);
    // require movement of at least 5cm
    float targetDist = MAX(0.05, depth / 8.);
    float progress = distFromStartPoint / targetDist;
    if (progress >= 1)
    {
        if ([delegate respondsToSelector:@selector(moveComplete)]) [delegate moveComplete];
    }
    else
    {
        float maxDim = self.bounds.size.width > self.bounds.size.height ? self.bounds.size.height : self.bounds.size.width;
        float maxRadius = maxDim / 2. - lineWidth * 2; //aligns center of circle with center of line
        float radius = 1. / (1. + 1./targetDist) * maxRadius;
        circleRadius = (1. - smoothing) * radius + smoothing * circleRadius; //simple low-pass filter to keep circle from stuttering around
        dotX = trans.x / targetDist * circleRadius;
        dotY = -trans.y / targetDist * circleRadius;
        [self setNeedsLayout];
    }
}

@end
