//
//  RCFractionView.m
//  RCCore
//
//  Created by Ben Hirashima on 5/22/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCFractionLabel.h"
#import "UIView+RCConstraints.h"

#define LABEL_WIDTH 13
#define LABEL_HEIGHT 13

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
    nominatorLabel = [[UILabel alloc] initWithFrame:CGRectMake(0, 0, LABEL_WIDTH, LABEL_HEIGHT)];
    nominatorLabel.translatesAutoresizingMaskIntoConstraints = NO;
    nominatorLabel.textColor = self.textColor;
    nominatorLabel.textAlignment = NSTextAlignmentRight;
    nominatorLabel.backgroundColor = [UIColor clearColor];
    nominatorLabel.font = self.font;
    [self addSubview:nominatorLabel];
    
    denominatorLabel = [[UILabel alloc] initWithFrame:CGRectMake(12, 8, LABEL_WIDTH, LABEL_HEIGHT)];
    denominatorLabel.translatesAutoresizingMaskIntoConstraints = NO;
    denominatorLabel.textColor = self.textColor;
    denominatorLabel.textAlignment = NSTextAlignmentLeft;
    denominatorLabel.backgroundColor = [UIColor clearColor];
    denominatorLabel.font = self.font;
    [self addSubview:denominatorLabel];
    
    [nominatorLabel addLeadingSpaceToSuperviewConstraint:0];
    [nominatorLabel addTopSpaceToSuperviewConstraint:0];
    [denominatorLabel addBottomSpaceToSuperviewConstraint:0];
    
    spacingConstraint = [NSLayoutConstraint constraintWithItem:nominatorLabel
                                                     attribute:NSLayoutAttributeRight
                                                     relatedBy:NSLayoutRelationEqual
                                                        toItem:denominatorLabel
                                                     attribute:NSLayoutAttributeLeft
                                                    multiplier:1
                                                      constant:0];
    [self addConstraint:spacingConstraint];
    
    widthConstraint = [NSLayoutConstraint constraintWithItem:self
                                                    attribute:NSLayoutAttributeWidth
                                                    relatedBy:NSLayoutRelationEqual
                                                       toItem:nil
                                                    attribute:NSLayoutAttributeNotAnAttribute
                                                   multiplier:1
                                                     constant:self.frame.size.width];
    [self addConstraint:widthConstraint];
    
    heightConstraint = [NSLayoutConstraint constraintWithItem:self
                                                     attribute:NSLayoutAttributeHeight
                                                     relatedBy:NSLayoutRelationEqual
                                                        toItem:nil
                                                     attribute:NSLayoutAttributeNotAnAttribute
                                                    multiplier:1
                                                      constant:self.frame.size.height];
    [self addConstraint:heightConstraint];
}

- (CGSize) sizeThatFits:(CGSize)size
{
    return CGSizeMake(nominatorLabel.bounds.size.width + denominatorLabel.bounds.size.width, (self.font.pointSize / 17) * 21);
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
    
    [self sizeToFit];
}

- (void)parseFraction:(NSString*)fractionString
{
    NSArray* components = [fractionString componentsSeparatedByString:@"/"];
    if (components.count == 2)
    {
        [self setFromStringsNominator:components[0] andDenominator:components[1]];
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

- (void) setFont:(UIFont *)font
{
    CGFloat fontSize = (10. / 17.) * font.pointSize;
    UIFont* subFont = [UIFont fontWithName:font.familyName size:fontSize];
    nominatorLabel.font = subFont;
    denominatorLabel.font = subFont;
    [self sizeToFit];
    [super setFont:font];
}

- (void) sizeToFit
{
    [nominatorLabel sizeToFit];
    [denominatorLabel sizeToFit];
    
    CGSize size = [self sizeThatFits:self.frame.size];
    widthConstraint.constant = size.width;
    heightConstraint.constant = size.height;
    [self setNeedsUpdateConstraints];
}

- (void)drawRect:(CGRect)rect
{
    CGContextRef context = UIGraphicsGetCurrentContext();
    
    CGFloat nomX = nominatorLabel.bounds.size.width / 2 - 1;
//    CGFloat nomY = nominatorLabel.bounds.size.height * 1.2;
    
    CGFloat nomY;
    if (nominatorLabel.text.length == 1 && denominatorLabel.text.length > 1)
    {
        nomY = nominatorLabel.bounds.size.height * 1.1;
    }
    else
    {
        nomY = nominatorLabel.bounds.size.height * 1.2;
    }
    
    CGFloat denomX = denominatorLabel.frame.origin.x + denominatorLabel.bounds.size.width / 2;
//    if (denominatorLabel.text.length > 1)
//    {
//        denomX = denominatorLabel.frame.origin.x + denominatorLabel.bounds.size.width / 2;
//    }
//    else
//    {
//        denomX = denominatorLabel.frame.origin.x + denominatorLabel.bounds.size.width / 2;
//    }
    
    CGFloat denomY = denominatorLabel.frame.origin.y / 1.2;
    
    CGContextMoveToPoint(context, nomX, nomY);
    CGContextAddLineToPoint(context, denomX, denomY);
    
    CGContextSetLineWidth(context, 1);
    CGContextSetStrokeColorWithColor(context, [[self textColor] CGColor]);
    CGContextStrokePath(context);
}

@end
