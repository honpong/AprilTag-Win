
#import "ARView.h"

@implementation ARView
@synthesize drawTarget, drawCrosshairs;

- (id) init
{
    self = [super init];
    
    if (self)
    {
        drawTarget = NO; //prevents target from being drawn until setTargetCoordinatesWithX:withY: has been called at least once
    }
    
    return self;
}

- (void)drawRect:(CGRect)rect
{
	CGContextRef context = UIGraphicsGetCurrentContext();
	
    if (drawTarget)
    {
        CGContextBeginPath(context);
        CGContextAddArc(context, targetX, targetY, 50, -M_PI, M_PI, 1);
        CGContextClosePath(context); // could be omitted
        CGContextSetFillColorWithColor(context, [UIColor redColor].CGColor);
        CGContextFillPath(context);
    }
    
    if (drawCrosshairs)
    {
        CGContextSetLineWidth(context, 1);
        CGContextSetStrokeColorWithColor(context, [[UIColor blackColor] CGColor]);
        CGMutablePathRef pathRef = CGPathCreateMutable();
        CGPathMoveToPoint(pathRef, NULL, self.frame.size.width / 2, 0);
        CGPathAddLineToPoint(pathRef, NULL, self.frame.size.width / 2, self.frame.size.height);
        CGPathMoveToPoint(pathRef, NULL, 0, self.frame.size.height / 2);
        CGPathAddLineToPoint(pathRef, NULL, self.frame.size.width, self.frame.size.height / 2);
        CGContextAddPath(context, pathRef);
        CGContextStrokePath(context);
    }
}

- (void) setTargetCoordinatesWithX: (float)x withY: (float)y
{
    targetX = x;
    targetY = y;
    drawTarget = YES;
    drawCrosshairs = YES;
    [self setNeedsDisplay];
}

- (void) clearDrawing
{
    drawCrosshairs = NO;
    drawTarget = NO;
    [self setNeedsDisplay];
}

@end
