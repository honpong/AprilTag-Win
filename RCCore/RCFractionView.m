//
//  RCFractionView.m
//  RCCore
//
//  Created by Ben Hirashima on 5/22/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCFractionView.h"

#define LABEL_SIZE 15

@implementation RCFractionView
{
    UILabel* nominator;
    UILabel* denominator;
}

- (id)initWithCoder:(NSCoder *)decoder
{
    self = [super initWithCoder:decoder];
    if (self)
    {
        [self setupViews];
    }
    return self;
}

- (id)initWithFrame:(CGRect)frame
{
    self = [super initWithFrame:frame];
    if (self) {
        [self setupViews];
    }
    return self;
}

- (void)setupViews
{
    self.backgroundColor = [UIColor whiteColor];
    
    nominator = [[UILabel alloc] initWithFrame:CGRectMake(0, 0, LABEL_SIZE, LABEL_SIZE)];
    nominator.text = @"11";
    nominator.textAlignment = NSTextAlignmentRight;
    nominator.font = [UIFont systemFontOfSize:12.0];
    nominator.backgroundColor = [UIColor clearColor];
    [self addSubview:nominator];
    
    denominator = [[UILabel alloc] initWithFrame:CGRectMake(16, 8, LABEL_SIZE, LABEL_SIZE)];
    denominator.text = @"16";
    denominator.textAlignment = NSTextAlignmentLeft;
    denominator.font = [UIFont systemFontOfSize:12.0];
    denominator.backgroundColor = [UIColor clearColor];
    [self addSubview:denominator];
}

- (void)setNominator:(int)nom andDenominator:(int)denom
{
    nominator.text = [NSString stringWithFormat:@"%i", nom];
    denominator.text = [NSString stringWithFormat:@"%i", denom];
    [self setNeedsDisplay];
}

- (void)parseFraction:(NSString*)fractionString
{
    NSArray* components = [fractionString componentsSeparatedByString:@"/"];
    if (components.count == 2)
    {
        nominator.text = [components objectAtIndex:0];
        denominator.text = [components objectAtIndex:1];
    }
}

- (void)drawRect:(CGRect)rect
{
    CGContextRef context = UIGraphicsGetCurrentContext();
    
    CGContextMoveToPoint(context, 11, 17);
    CGContextAddLineToPoint(context, 19, 5);
    
    CGContextSetLineWidth(context, 1);
    CGContextSetStrokeColorWithColor(context, [[UIColor blackColor] CGColor]);
    CGContextStrokePath(context);
}



@end
