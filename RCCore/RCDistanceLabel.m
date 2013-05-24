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
    distanceLabel.textColor = [UIColor blackColor];
    distanceLabel.textAlignment = NSTextAlignmentRight;
    [self addSubview:distanceLabel];
    
    fractionLabel = [[RCFractionView alloc] initWithFrame:CGRectMake(distanceLabel.frame.size.width, 0, FRACTION_VIEW_WIDTH, FRACTION_VIEW_HEIGHT)];
    [self addSubview:fractionLabel];
    
    symbolLabel = [[UILabel alloc] initWithFrame:CGRectMake(distanceLabel.frame.size.width + FRACTION_VIEW_WIDTH, 0, 7, distanceLabel.frame.size.height)];
    symbolLabel.textColor = [UIColor blackColor];
    symbolLabel.text = @"\"";
    [self addSubview:symbolLabel];
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
    [fractionLabel setNeedsDisplay];
    distanceLabel.text = [distObj getStringWithoutFractionOrUnitsSymbol];
    [distanceLabel setNeedsDisplay];
}

@end
