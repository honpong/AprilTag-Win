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
#define SYMBOL_VIEW_HEIGHT 22

@implementation RCDistanceLabel
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
    distanceLabel = [[UILabel alloc] initWithFrame:CGRectMake(0, 0, self.frame.size.width - FRACTION_VIEW_WIDTH - SYMBOL_VIEW_WIDTH, self.frame.size.height)];
    distanceLabel.textColor = self.textColor;
    distanceLabel.textAlignment = NSTextAlignmentRight;
    distanceLabel.backgroundColor = self.backgroundColor;
    distanceLabel.text = self.text;
    self.text = @"";
    [self addSubview:distanceLabel];
    
    fractionLabel = [[RCFractionView alloc] initWithFrame:CGRectMake(distanceLabel.frame.size.width, 0, FRACTION_VIEW_WIDTH, FRACTION_VIEW_HEIGHT)];
    fractionLabel.backgroundColor = self.backgroundColor;
    fractionLabel.textColor = self.textColor;
    [self addSubview:fractionLabel];
    
    symbolLabel = [[UILabel alloc] initWithFrame:CGRectMake(distanceLabel.frame.size.width + FRACTION_VIEW_WIDTH, 0, 7, distanceLabel.frame.size.height)];
    symbolLabel.textColor = self.textColor;
    symbolLabel.text = @"\"";
    symbolLabel.backgroundColor = self.backgroundColor;
    [self addSubview:symbolLabel];
}

- (void) layoutSubviews
{
    int distanceLabelOriginX = 0;
    int contentWidth = distanceLabel.frame.size.width + fractionLabel.frame.size.width + symbolLabel.frame.size.width;
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
    fractionLabel.frame = CGRectMake(distanceLabel.frame.origin.x + distanceLabel.frame.size.width, 0, FRACTION_VIEW_WIDTH, FRACTION_VIEW_HEIGHT);
    symbolLabel.frame = CGRectMake(fractionLabel.frame.origin.x + fractionLabel.frame.size.width, 0, symbolLabel.frame.size.width, distanceLabel.frame.size.height);
}

- (void) setDistanceText:(NSString*)dist
{
    NSArray* distComponents = [dist componentsSeparatedByString:@" "];
       
    if (distComponents.count >= 2)
    {
        NSString* fractionString = [distComponents objectAtIndex:distComponents.count - 1];
        NSArray* fractionComponents = [fractionString componentsSeparatedByString:@"/"];
        
        if (fractionComponents.count == 2)
        {
            [fractionLabel setFromStringsNominator:[fractionComponents objectAtIndex:0] andDenominator:[fractionComponents objectAtIndex:1]];
            [fractionLabel setNeedsDisplay];
            distanceLabel.text = [dist substringToIndex:dist.length - fractionString.length - 1];
            [distanceLabel setNeedsDisplay];
            return;
        }
    }
    
    distanceLabel.text = dist;
    [distanceLabel setNeedsDisplay];
}

- (void) setDistanceImperialFractional:(RCDistanceImperialFractional*)distObj
{
    [fractionLabel setNominator:distObj.fraction.nominator andDenominator:distObj.fraction.denominator];
    [self setText:[distObj getStringWithoutFractionOrUnitsSymbol]];
}

- (void) setText:(NSString *)text
{
    distanceLabel.text = text;
    [distanceLabel sizeToFit];
    [self setNeedsDisplay];
}

@end
