//
//  RCFractionView.m
//  RCCore
//
//  Created by Ben Hirashima on 5/22/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCFractionLabel.h"
#import "UIView+RCConstraints.h"
#import "UIView+RCAutoLayoutDebugging.h"

@implementation RCFractionLabel
{
    UILabel* nominatorLabel;
    UILabel* denominatorLabel;
    NSLayoutConstraint* spacingConstraint;
    NSLayoutConstraint* widthConstraint;
    NSLayoutConstraint* heightConstraint;
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
    self.translatesAutoresizingMaskIntoConstraints = NO;
    self.autoLayoutNameTag = @"fractionLabel";
    self.backgroundColor = [UIColor clearColor];
    
    nominatorLabel = [[UILabel alloc] init];
    nominatorLabel.translatesAutoresizingMaskIntoConstraints = NO;
    nominatorLabel.textColor = self.textColor;
    nominatorLabel.textAlignment = NSTextAlignmentRight;
    nominatorLabel.backgroundColor = [UIColor clearColor];
    nominatorLabel.font = self.font;
    nominatorLabel.autoLayoutNameTag = @"nominator";
    [self addSubview:nominatorLabel];
    
    denominatorLabel = [[UILabel alloc] init];
    denominatorLabel.translatesAutoresizingMaskIntoConstraints = NO;
    denominatorLabel.textColor = self.textColor;
    denominatorLabel.textAlignment = NSTextAlignmentLeft;
    denominatorLabel.backgroundColor = [UIColor clearColor];
    denominatorLabel.font = self.font;
    denominatorLabel.autoLayoutNameTag = @"denominator";
    [self addSubview:denominatorLabel];
    
    [self setShadowColor:self.shadowColor];
    [self setFont:[UIFont systemFontOfSize:17.]]; // must be done after nom and denom have been created
}

- (void) updateConstraints
{
    [nominatorLabel removeConstraintsFromSuperview];
    [denominatorLabel removeConstraintsFromSuperview];
    
    [nominatorLabel addLeftSpaceToSuperviewConstraint:0];
    [nominatorLabel addTopSpaceToSuperviewConstraint:0];
    [denominatorLabel addBottomSpaceToSuperviewConstraint:0];
    [denominatorLabel addRightSpaceToSuperviewConstraint:0];
    
    [super updateConstraints];
}

- (CGSize) intrinsicContentSize
{
    return [self sizeThatFits:CGSizeZero];
}

- (CGSize) sizeThatFits:(CGSize)size
{
    CGFloat width = 0;
    if (!self.hidden) width = nominatorLabel.bounds.size.width + denominatorLabel.bounds.size.width;
    CGSize sizeThatFits = CGSizeMake(width, (self.font.pointSize / 17) * 21);
    return sizeThatFits;
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
        [nominatorLabel sizeToFit];
        denominatorLabel.text = denominator;
        [denominatorLabel sizeToFit];
        self.hidden = NO;
    }
    
    [self invalidateIntrinsicContentSize];
    [self setNeedsDisplay]; // redraws line
}

- (void)parseFraction:(NSString*)fractionString
{
    NSArray* components = [fractionString componentsSeparatedByString:@"/"];
    if (components.count == 2)
    {
        [self setFromStringsNominator:components[0] andDenominator:components[1]];
    }
}

- (void)setTextColor:(UIColor *)textColor
{
    _textColor = textColor;
    nominatorLabel.textColor = textColor;
    denominatorLabel.textColor = textColor;
    [self setNeedsDisplay];
}

- (void) setFont:(UIFont *)font
{
    _font = font;
    CGFloat fontSize = (10. / 17.) * font.pointSize;
    UIFont* subFont = [UIFont fontWithName:font.familyName size:fontSize];
    nominatorLabel.font = subFont;
    denominatorLabel.font = subFont;
    [self invalidateIntrinsicContentSize];
    [self setNeedsDisplay]; // redraws line
}

- (void) setShadowColor:(UIColor *)shadowColor
{
    _shadowColor = shadowColor;
    nominatorLabel.shadowColor = shadowColor;
    nominatorLabel.shadowOffset = CGSizeMake(1, 1);
    denominatorLabel.shadowColor = shadowColor;
    denominatorLabel.shadowOffset = CGSizeMake(1, 1);
    [self setNeedsDisplay];
}

- (void)drawRect:(CGRect)rect
{
    [self drawLineWithColor:self.textColor withOffset:CGSizeZero];
    if (self.shadowColor) [self drawLineWithColor:self.shadowColor withOffset:self.shadowOffset];
}

- (void) drawLineWithColor:(UIColor*)color withOffset:(CGSize)offset
{
    CGContextRef context = UIGraphicsGetCurrentContext();
    
    CGFloat nomX;
    if (nominatorLabel.text.length > 1)
    {
        // if two char nominator, start slash more to the left
        nomX = nominatorLabel.frame.origin.x + nominatorLabel.bounds.size.width / 1.5;
    }
    else
    {
        nomX = nominatorLabel.frame.origin.x + nominatorLabel.bounds.size.width / 2;
    }
    nomX = nomX + offset.width;
    
    CGFloat nomY = nominatorLabel.bounds.size.height * 1.2 + offset.height;
    
    CGFloat denomX;
    if (denominatorLabel.text.length > 1)
    {
        // if two char denominator, end slash more to the left
        denomX = denominatorLabel.frame.origin.x + denominatorLabel.bounds.size.width / 4;
    }
    else
    {
        denomX = denominatorLabel.frame.origin.x + denominatorLabel.bounds.size.width / 2;
    }
    denomX = denomX + offset.width;
    
    CGFloat denomY = denominatorLabel.frame.origin.y / 1.2 + offset.height;
    
    CGContextMoveToPoint(context, nomX, nomY);
    CGContextAddLineToPoint(context, denomX, denomY);
    
    CGContextSetLineWidth(context, 1);
    CGContextSetStrokeColorWithColor(context, [color CGColor]);
    CGContextStrokePath(context);

}

@end
