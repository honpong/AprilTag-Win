//
//  RCFractionView.m
//  RCCore
//
//  Created by Ben Hirashima on 5/22/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCFractionLabel.h"

#define LABEL_WIDTH 13
#define LABEL_HEIGHT 13

@implementation RCFractionLabel
{
    UILabel* nominatorLabel;
    UILabel* denominatorLabel;
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
    nominatorLabel = [[UILabel alloc] initWithFrame:CGRectMake(0, 0, LABEL_WIDTH, LABEL_HEIGHT)];
    nominatorLabel.text = @"11";
    nominatorLabel.textColor = self.textColor;
    nominatorLabel.textAlignment = NSTextAlignmentRight;
    nominatorLabel.font = [UIFont systemFontOfSize:10.0];
    nominatorLabel.backgroundColor = [UIColor clearColor];
    [self addSubview:nominatorLabel];
    
    denominatorLabel = [[UILabel alloc] initWithFrame:CGRectMake(12, 8, LABEL_WIDTH, LABEL_HEIGHT)];
    denominatorLabel.text = @"16";
    denominatorLabel.textColor = self.textColor;
    denominatorLabel.textAlignment = NSTextAlignmentLeft;
    denominatorLabel.font = [UIFont systemFontOfSize:10.0];
    denominatorLabel.backgroundColor = [UIColor clearColor];
    [self addSubview:denominatorLabel];
}

- (void)setNominator:(int)nominator andDenominator:(int)denominator
{
    [self setFromStringsNominator:[NSString stringWithFormat:@"%i", nominator] andDenominator:[NSString stringWithFormat:@"%i", denominator]];
}

- (void)setFromStringsNominator:(NSString*)nominator andDenominator:(NSString*)denominator
{
    if ([nominator isEqualToString:@"0"])
    {
        self.hidden = YES;        
    }
    else
    {
        nominatorLabel.text = nominator;
        denominatorLabel.text = denominator;
        self.hidden = NO;
    }
    
    [self setNeedsDisplay];
}

- (void)parseFraction:(NSString*)fractionString
{
    NSArray* components = [fractionString componentsSeparatedByString:@"/"];
    if (components.count == 2)
    {
        [self setFromStringsNominator:[components objectAtIndex:0] andDenominator:[components objectAtIndex:1]];
    }
}

- (void)setText:(NSString *)text
{
    [super setText:nil];
    [self parseFraction:text];
}

- (void)setTextColor:(UIColor *)textColor
{
    [super setTextColor:textColor];
    nominatorLabel.textColor = textColor;
    denominatorLabel.textColor = textColor;
    [self setNeedsDisplay];
}

- (void)drawRect:(CGRect)rect
{
    CGContextRef context = UIGraphicsGetCurrentContext();
    
    CGContextMoveToPoint(context, 9, 16);
    CGContextAddLineToPoint(context, 16, 5);
    
    CGContextSetLineWidth(context, 1);
    CGContextSetStrokeColorWithColor(context, [[self textColor] CGColor]);
    CGContextStrokePath(context);
}

@end
