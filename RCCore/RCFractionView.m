//
//  RCFractionView.m
//  RCCore
//
//  Created by Ben Hirashima on 5/22/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCFractionView.h"

#define LABEL_WIDTH 13
#define LABEL_HEIGHT 13

@implementation RCFractionView
{
    UILabel* nominator;
    UILabel* denominator;
//    UILabel* symbolLabel;
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
    
    nominator = [[UILabel alloc] initWithFrame:CGRectMake(0, 0, LABEL_WIDTH, LABEL_HEIGHT)];
    nominator.text = @"11";
    nominator.textAlignment = NSTextAlignmentRight;
    nominator.font = [UIFont systemFontOfSize:10.0];
    nominator.backgroundColor = [UIColor clearColor];
    [self addSubview:nominator];
    
    denominator = [[UILabel alloc] initWithFrame:CGRectMake(12, 8, LABEL_WIDTH, LABEL_HEIGHT)];
    denominator.text = @"16";
    denominator.textAlignment = NSTextAlignmentLeft;
    denominator.font = [UIFont systemFontOfSize:10.0];
    denominator.backgroundColor = [UIColor clearColor];
    [self addSubview:denominator];
    
//    symbolLabel = [[UILabel alloc] initWithFrame:CGRectMake(self.frame.size.width - 7, 5, 7, 12)];
//    symbolLabel.textColor = [UIColor blackColor];
//    symbolLabel.text = @"\"";
//    [self insertSubview:symbolLabel belowSubview:denominator];
}

- (void)setNominator:(int)nom andDenominator:(int)denom
{
    nominator.text = [NSString stringWithFormat:@"%i", nom];
    denominator.text = [NSString stringWithFormat:@"%i", denom];
    [self setNeedsDisplay];
}

- (void)setFromStringsNominator:(NSString*)nom andDenominator:(NSString*)denom
{
    nominator.text = nom;
    denominator.text = denom;
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
    
    CGContextMoveToPoint(context, 8, 17);
    CGContextAddLineToPoint(context, 16, 5);
    
    CGContextSetLineWidth(context, 1);
    CGContextSetStrokeColorWithColor(context, [[UIColor blackColor] CGColor]);
    CGContextStrokePath(context);
}



@end
