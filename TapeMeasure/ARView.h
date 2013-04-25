
#import <UIKit/UIKit.h>

@interface ARView : UIView {
	float targetX;
    float targetY;
}

@property (readonly) BOOL drawTarget;
@property (readonly) BOOL drawCrosshairs;

- (void) startDrawingCrosshairs;
- (void) setTargetCoordinatesWithX: (float)x withY: (float)y;
- (void) clearDrawing;

@end

