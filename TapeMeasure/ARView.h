//
//  OverlayView.h
//  AugmentedRealitySample
//
//  Created by Chris Greening on 01/01/2010.
//

#import <UIKit/UIKit.h>

@interface ARView : UIView {
	float targetX;
    float targetY;
}

@property BOOL drawTarget;
@property BOOL drawCrosshairs;

- (void) setTargetCoordinatesWithX: (float)x withY: (float)y;
- (void) clearDrawing;

@end

