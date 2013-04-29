
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
        //contrain target location to bounds of frame
        targetX = targetX > self.frame.size.width ? self.frame.size.width : targetX;
        targetX = targetX < 0 ? 0 : targetX;
        targetY = targetY > self.frame.size.height ? self.frame.size.height : targetY;
        targetY = targetY < 0 ? 0 : targetY;
        
        //draw concentric circles for the target
        CGContextSetFillColorWithColor(context, [UIColor colorWithRed:0.7 green:0 blue:0 alpha:0.3].CGColor);
        CGContextAddArc(context, targetX, targetY, 40, -M_PI, M_PI, 1);
        CGContextFillPath(context);
        CGContextSetFillColorWithColor(context, [UIColor colorWithRed:1 green:1 blue:1 alpha:0.4].CGColor);
        CGContextAddArc(context, targetX, targetY, 30, -M_PI, M_PI, 1);
        CGContextFillPath(context);
        CGContextSetFillColorWithColor(context, [UIColor colorWithRed:0.7 green:0 blue:0 alpha:0.3].CGColor);
        CGContextAddArc(context, targetX, targetY, 20, -M_PI, M_PI, 1);
        CGContextFillPath(context);
        CGContextSetFillColorWithColor(context, [UIColor colorWithRed:1 green:1 blue:1 alpha:0.4].CGColor);
        CGContextAddArc(context, targetX, targetY, 10, -M_PI, M_PI, 1);
        CGContextFillPath(context);        
    }
    
    if (drawCrosshairs)
    {
        float const xCenter = self.frame.size.width / 2;
        float const yCenter = self.frame.size.height / 2;
        int const circleRadius = 40;
        
        //circle in center of crosshairs
        CGContextAddArc(context, xCenter, yCenter, circleRadius, -M_PI, M_PI, 1);
        
        //line from top of screen to top of circle
        CGContextMoveToPoint(context, xCenter, 0);
        CGContextAddLineToPoint(context, xCenter, yCenter - circleRadius);
        
        //line from bottom of circle to bottom of screen
        CGContextMoveToPoint(context, xCenter, yCenter + circleRadius);
        CGContextAddLineToPoint(context, xCenter, self.frame.size.height);
        
        //line from left of screen to left of circle
        CGContextMoveToPoint(context, 0, yCenter);
        CGContextAddLineToPoint(context, xCenter - circleRadius, yCenter);
        
        //line from right of circle to right of screen
        CGContextMoveToPoint(context, xCenter + circleRadius, yCenter);
        CGContextAddLineToPoint(context, self.frame.size.width, yCenter);
        
        CGContextSetAlpha(context, 0.5);
        CGContextSetLineWidth(context, 1);
        CGContextSetStrokeColorWithColor(context, [[UIColor blackColor] CGColor]);
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
