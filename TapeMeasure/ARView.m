
#import "ARView.h"

@implementation ARView
@synthesize drawTarget, drawCrosshairs;

- (id) init
{
    self = [super init];
    
    if (self)
    {
        drawTarget = NO; //prevents target from being drawn until setTargetCoordinatesWithX:withY: has been called at least once
        drawCrosshairs = NO;
    }
    
    return self;
}

- (void)drawRect:(CGRect)rect
{
	CGContextRef context = UIGraphicsGetCurrentContext();
	
    if (drawTarget)
    {
        targetX = targetX > self.frame.size.width ? self.frame.size.width : targetX;
        targetX = targetX < 0 ? 0 : targetX;
        targetY = targetY > self.frame.size.height ? self.frame.size.height : targetY;
        targetY = targetY < 0 ? 0 : targetY;
        
        CGContextSetAlpha(context, 0.3);
        
        CGContextSetFillColorWithColor(context, [UIColor blackColor].CGColor);
        CGContextAddArc(context, targetX, targetY, 40, -M_PI, M_PI, 1);
        CGContextFillPath(context);
        CGContextSetFillColorWithColor(context, [UIColor whiteColor].CGColor);
        CGContextAddArc(context, targetX, targetY, 30, -M_PI, M_PI, 1);
        CGContextFillPath(context);
        CGContextSetFillColorWithColor(context, [UIColor blackColor].CGColor);
        CGContextAddArc(context, targetX, targetY, 20, -M_PI, M_PI, 1);
        CGContextFillPath(context);
        CGContextSetFillColorWithColor(context, [UIColor whiteColor].CGColor);
        CGContextAddArc(context, targetX, targetY, 10, -M_PI, M_PI, 1);
        CGContextFillPath(context);        
    }
    
    if (drawCrosshairs)
    {
        float const xCenter = self.frame.size.width / 2;
        float const yCenter = self.frame.size.height / 2;
        int const circleRadius = 40;
                
        CGContextAddArc(context, xCenter, yCenter, circleRadius, -M_PI, M_PI, 1);
                
        CGContextMoveToPoint(context, xCenter, 0);
        CGContextAddLineToPoint(context, xCenter, yCenter - circleRadius);
        
        CGContextMoveToPoint(context, xCenter, yCenter + circleRadius);
        CGContextAddLineToPoint(context, xCenter, self.frame.size.height);
        
        CGContextMoveToPoint(context, 0, yCenter);
        CGContextAddLineToPoint(context, xCenter - circleRadius, yCenter);
        
        CGContextMoveToPoint(context, xCenter + circleRadius, yCenter);
        CGContextAddLineToPoint(context, self.frame.size.width, yCenter);
        
        CGContextSetAlpha(context, 1.0);
        CGContextSetLineWidth(context, 1);
        CGContextSetStrokeColorWithColor(context, [[UIColor redColor] CGColor]);
        CGContextStrokePath(context);        
    }
}

- (void) startDrawingCrosshairs
{
    drawCrosshairs = YES;
    [self setNeedsDisplay];
}

- (void) setTargetCoordinatesWithX: (float)x withY: (float)y
{
    targetX = x;
    targetY = y;
    drawTarget = YES;
    [self setNeedsDisplay];
}

- (void) clearDrawing
{
    drawCrosshairs = NO;
    drawTarget = NO;
    [self setNeedsDisplay];
}

@end
