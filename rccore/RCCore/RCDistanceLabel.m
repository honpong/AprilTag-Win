//
//  RCDistanceLabel.m
//  FractionView
//
//  Created by Ben Hirashima on 5/22/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCDistanceLabel.h"
#import "NSString+RCString.h"
#import "UIView+RCConstraints.h"

#define FRACTION_VIEW_WIDTH 30
#define FRACTION_VIEW_HEIGHT 22

#define SYMBOL_VIEW_WIDTH 7

@implementation RCDistanceLabel
{
    int fractionViewWidth;
    int symbolViewWidth;
}
@synthesize distanceLabel, fractionLabel, symbolLabel;

- (id) initWithCoder:(NSCoder *)decoder
{
    self = [super initWithCoder:decoder];
    if (self)
    {
        [self setupViews];
    }
    return self;
}

- (id) initWithFrame:(CGRect)frame
{
    self = [super initWithFrame:frame];
    if (self)
    {
        [self setupViews];
    }
    return self;
}

- (void) setupViews
{
    self.backgroundColor = [UIColor clearColor];
    fractionViewWidth = FRACTION_VIEW_WIDTH;
    symbolViewWidth = SYMBOL_VIEW_WIDTH;
    
    distanceLabel = [[UILabel alloc] initWithFrame:CGRectMake(0, 0, self.frame.size.width - fractionViewWidth - symbolViewWidth, self.frame.size.height)];
    distanceLabel.translatesAutoresizingMaskIntoConstraints = NO;
    distanceLabel.font = self.font;
    distanceLabel.textColor = self.textColor;
    distanceLabel.textAlignment = NSTextAlignmentRight;
    distanceLabel.backgroundColor = self.backgroundColor;
    distanceLabel.text = self.text;
    [distanceLabel sizeToFit];
    [self addSubview:distanceLabel];
    
    fractionLabel = [[RCFractionLabel alloc] initWithFrame:CGRectMake(distanceLabel.frame.size.width + SYMBOL_VIEW_WIDTH, 0, fractionViewWidth, FRACTION_VIEW_HEIGHT)];
    fractionLabel.translatesAutoresizingMaskIntoConstraints = NO;
    fractionLabel.font = self.font;
    fractionLabel.backgroundColor = self.backgroundColor;
    fractionLabel.textColor = self.textColor;
    [fractionLabel sizeToFit];
    [self addSubview:fractionLabel];
    
    symbolLabel = [[UILabel alloc] initWithFrame:CGRectMake(distanceLabel.frame.size.width, 0, symbolViewWidth, distanceLabel.frame.size.height)];
    symbolLabel.translatesAutoresizingMaskIntoConstraints = NO;
    symbolLabel.font = self.font;
    symbolLabel.textColor = self.textColor;
    symbolLabel.backgroundColor = self.backgroundColor;
    symbolLabel.text = @"\"";
    [symbolLabel sizeToFit];
    [self addSubview:symbolLabel];
    
    // symbol label
    [self addConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"H:[symbolLabel(width)]|"
                                                                 options:0
                                                                 metrics:@{@"width":@(symbolLabel.bounds.size.width)}
                                                                   views:NSDictionaryOfVariableBindings(symbolLabel)]];
    [self addConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"V:[symbolLabel(height)]|"
                                                                 options:0
                                                                 metrics:@{@"height":@(self.bounds.size.height)}
                                                                   views:NSDictionaryOfVariableBindings(symbolLabel)]];
    
    // fraction label
    [self addConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"H:[fractionLabel(width)][symbolLabel]"
                                                                 options:0
                                                                 metrics:@{@"width":@FRACTION_VIEW_WIDTH}
                                                                   views:NSDictionaryOfVariableBindings(fractionLabel, symbolLabel)]];
    [self addConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"V:[fractionLabel(height)]|"
                                                                 options:0
                                                                 metrics:@{@"height":@(self.bounds.size.height)}
                                                                   views:NSDictionaryOfVariableBindings(fractionLabel)]];
    
    // distance label
    [self addConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"H:|[distanceLabel][fractionLabel]"
                                                                 options:0
                                                                 metrics:nil
                                                                   views:NSDictionaryOfVariableBindings(fractionLabel, distanceLabel)]];
    [self addConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"V:[distanceLabel(height)]|"
                                                                 options:0
                                                                 metrics:@{@"height":@(self.bounds.size.height)}
                                                                   views:NSDictionaryOfVariableBindings(distanceLabel)]];
//    DLog(@"%@", self.constraints);
}

- (void) setDistanceText:(NSString*)dist
{
    NSArray* distComponents = [dist componentsSeparatedByString:@" "];
       
    if (distComponents.count >= 2)
    {
        NSString* fractionString = distComponents[distComponents.count - 1];
        NSArray* fractionComponents = [fractionString componentsSeparatedByString:@"/"];
        
        if (fractionComponents.count == 2)
        {
            [fractionLabel setFromStringsNominator:fractionComponents[0] andDenominator:fractionComponents[1]];
            [fractionLabel setNeedsDisplay];
            distanceLabel.text = [dist substringToIndex:dist.length - fractionString.length - 1];
            [distanceLabel setNeedsDisplay];
            return;
        }
    }
    
    distanceLabel.text = dist;
}

- (void) setDistance:(id<RCDistance>)distObj
{
    if ([distObj isKindOfClass:[RCDistanceImperialFractional class]])
    {
        [self setDistanceImperialFractional:distObj];
        symbolLabel.hidden = NO;
        fractionLabel.hidden = NO;
    }
    else
    {
        symbolLabel.hidden = YES;
        fractionLabel.hidden = YES;
        
        NSLayoutConstraint* constraint = [fractionLabel findWidthConstraint];
        constraint.constant = 0;
        [self setNeedsUpdateConstraints];
        
        self.text = [distObj getString];
    }
}

- (void) setDistanceImperialFractional:(RCDistanceImperialFractional*)distObj
{
    if (distObj.fraction.nominator > 0)
    {
        fractionViewWidth = FRACTION_VIEW_WIDTH;
        [fractionLabel setNominator:distObj.fraction.nominator andDenominator:distObj.fraction.denominator];
        [fractionLabel sizeToFit];
    }
    else
    {
        fractionViewWidth = 0; // hide fraction view if no fraction
        
        NSLayoutConstraint* constraint = [fractionLabel findWidthConstraint];
        constraint.constant = 0;
        [self setNeedsUpdateConstraints];
    }
    
    if (distObj.wholeInches + distObj.fraction.nominator == 0)
    {
        symbolViewWidth = 0; // hide inch symbol if no inches
    }
    else
    {
        symbolViewWidth = SYMBOL_VIEW_WIDTH;
    }
    
    [self setText:[distObj getStringWithoutFractionOrUnitsSymbol]];
}

- (void) setText:(NSString *)text
{
    [super setText:nil];
    distanceLabel.text = text;
    [distanceLabel sizeToFit];
    [self setNeedsDisplay];
    [self setNeedsLayout];
}

- (void) setTextColor:(UIColor *)textColor
{
    [super setTextColor:textColor];
    distanceLabel.textColor = textColor;
    fractionLabel.textColor = textColor;
    symbolLabel.textColor = textColor;
    [self setNeedsDisplay];
}

- (void) setBackgroundColor:(UIColor *)backgroundColor
{
    [super setBackgroundColor:backgroundColor];
    distanceLabel.backgroundColor = backgroundColor;
    fractionLabel.backgroundColor = backgroundColor;
    symbolLabel.backgroundColor = backgroundColor;
    [self setNeedsDisplay];
}

@end
