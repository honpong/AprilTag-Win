//
//  RCDistanceLabel.m
//  FractionView
//
//  Created by Ben Hirashima on 5/22/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCDistanceLabel.h"
#import "NSString+RCString.h"

#define FRACTION_VIEW_WIDTH 23
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
    distanceLabel.textColor = self.textColor;
    distanceLabel.textAlignment = NSTextAlignmentRight;
    distanceLabel.backgroundColor = self.backgroundColor;
    distanceLabel.text = self.text;
    [self addSubview:distanceLabel];
    
    fractionLabel = [[RCFractionLabel alloc] initWithFrame:CGRectMake(distanceLabel.frame.size.width, 0, fractionViewWidth, FRACTION_VIEW_HEIGHT)];
    fractionLabel.backgroundColor = self.backgroundColor;
    fractionLabel.textColor = self.textColor;
    [self addSubview:fractionLabel];
    
    symbolLabel = [[UILabel alloc] initWithFrame:CGRectMake(distanceLabel.frame.size.width + fractionViewWidth, 0, symbolViewWidth, distanceLabel.frame.size.height)];
    symbolLabel.textColor = self.textColor;
    symbolLabel.text = @"\"";
    symbolLabel.backgroundColor = self.backgroundColor;
    [self addSubview:symbolLabel];
}

- (void) layoutSubviews
{
    int distanceLabelOriginX = 0;
    int contentWidth = distanceLabel.frame.size.width + fractionViewWidth + symbolViewWidth;
    int center = self.frame.size.width / 2;
    
    if (self.textAlignment == NSTextAlignmentCenter)
    {        
        distanceLabelOriginX = center - (contentWidth / 2);
    }
    else if (self.textAlignment == NSTextAlignmentRight)
    {
        distanceLabelOriginX = self.frame.size.width - contentWidth;
    }
    
    distanceLabel.frame = CGRectMake(distanceLabelOriginX, 0, distanceLabel.frame.size.width, distanceLabel.frame.size.height);
    fractionLabel.frame = CGRectMake(distanceLabel.frame.origin.x + distanceLabel.frame.size.width, 0, fractionViewWidth, FRACTION_VIEW_HEIGHT);
    symbolLabel.frame = CGRectMake(fractionLabel.frame.origin.x + fractionLabel.frame.size.width, 0, symbolViewWidth, FRACTION_VIEW_HEIGHT);
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
    [self setNeedsDisplay];
    [self setNeedsLayout];
}

- (void) setDistance:(id<RCDistance>)distObj
{
    if ([distObj isKindOfClass:[RCDistanceImperialFractional class]])
    {
        [self setDistanceImperialFractional:distObj];
    }
    else
    {
        symbolViewWidth = 0;
        fractionViewWidth = 0;
        self.text = [distObj getString];
    }
}

- (void) setDistanceImperialFractional:(RCDistanceImperialFractional*)distObj
{
    if (distObj.fraction.nominator > 0)
    {
        fractionViewWidth = FRACTION_VIEW_WIDTH;
        [fractionLabel setNominator:distObj.fraction.nominator andDenominator:distObj.fraction.denominator];
    }
    else
    {
        fractionViewWidth = 0; // hide fraction view if no fraction
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
