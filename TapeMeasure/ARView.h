
#import <UIKit/UIKit.h>

@interface ARView : UIView {
	float targetX;
    float targetY;
}

@property BOOL drawTarget;
@property BOOL drawCrosshairs;

- (void) startDrawingCrosshairs;
- (void) setTargetCoordinatesWithX: (float)x withY: (float)y;
- (void) clearDrawing;

@end

