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
#import "RCDistanceLabelContainer.h"

@interface RCDistanceLabel ()

@end

@implementation RCDistanceLabel
{
    RCDistanceLabelContainer* containerView;
    NSLayoutConstraint* alignmentConstraint;
}
@synthesize centerAlignmentExcludesFraction;

+ (RCDistanceLabel*) distLabel:(id<RCDistance>)distObj withFrame:(CGRect)frame
{
    return [RCDistanceLabel distLabel:distObj withFrame:frame withFont:[UIFont systemFontOfSize:[UIFont systemFontSize]]];
}

+ (RCDistanceLabel*) distLabel:(id<RCDistance>)distObj withFrame:(CGRect)frame withFont:(UIFont*)font
{
    RCDistanceLabel* label = [[RCDistanceLabel alloc] initWithFrame:frame];
    [label setDistance:distObj];
    label.font = font;
    return label;
}

- (id) initWithCoder:(NSCoder *)decoder
{
    if (self = [super initWithCoder:decoder])
    {
        [self setupViews];
    }
    return self;
}

- (id) initWithFrame:(CGRect)frame
{
    if (self = [super initWithFrame:frame])
    {
        [self setupViews];
    }
    return self;
}

- (void) setupViews
{
    containerView = [RCDistanceLabelContainer new];
    [self addSubview:containerView];
    
    [self setCenterAlignmentExcludesFraction:NO];
    [self setFont:[UIFont systemFontOfSize:17.]]; // must be done after subviews have been created
}

- (void) updateConstraints
{
    [containerView addCenterInSuperviewConstraints];
    [super updateConstraints];
}

- (void) addAlignmentConstraint:(NSTextAlignment)textAlignment
{
    if (alignmentConstraint) [containerView.superview removeConstraint:alignmentConstraint];
    
    if (textAlignment == NSTextAlignmentLeft)
        alignmentConstraint = [containerView addLeftSpaceToSuperviewConstraint:0];
    else if (textAlignment == NSTextAlignmentCenter)
        if (centerAlignmentExcludesFraction)
            alignmentConstraint = [containerView addCenterXInSuperviewConstraints:self.font.pointSize / 60 * -35];
        else
            alignmentConstraint = [containerView addCenterXInSuperviewConstraints];
    else if (textAlignment == NSTextAlignmentRight)
        alignmentConstraint = [containerView addRightSpaceToSuperviewConstraint:0];
}

- (CGSize) intrinsicContentSize
{
    return [containerView intrinsicContentSize];
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
            [containerView.fractionLabel setFromStringsNominator:fractionComponents[0] andDenominator:fractionComponents[1]];
            containerView.distanceLabel.text = [dist substringToIndex:dist.length - fractionString.length - 1];
            return;
        }
    }
    
    containerView.distanceLabel.text = dist;
}

- (void) setDistance:(id<RCDistance>)distObj
{
    if ([distObj isKindOfClass:[RCDistanceImperialFractional class]])
    {
        [self setDistanceImperialFractional:distObj];
    }
    else
    {
        containerView.distanceLabel.text = [distObj description];
        [self hideSymbol];
        [self hideFraction];
    }
    [containerView invalidateIntrinsicContentSize];
}

- (void) setDistanceImperialFractional:(RCDistanceImperialFractional*)distObj
{
    if (distObj.fraction.nominator > 0)
    {
        [self showFraction];
        [containerView.fractionLabel setNominator:distObj.fraction.nominator andDenominator:distObj.fraction.denominator];
    }
    else
    {
        [self hideFraction];
    }
    
    if (distObj.wholeInches + distObj.fraction.nominator == 0)
        [self hideSymbol];
    else
        [self showSymbol];
    
    containerView.distanceLabel.text = [distObj getStringWithoutFractionOrUnitsSymbol];
    [containerView.distanceLabel sizeToFit];
    
    [containerView invalidateIntrinsicContentSize];
}

- (void) showFraction
{
    containerView.fractionLabel.hidden = NO;
    [containerView invalidateIntrinsicContentSize];
}

- (void) hideFraction
{
    containerView.fractionLabel.hidden = YES;
    [containerView invalidateIntrinsicContentSize];
}

- (void) showSymbol
{
    containerView.symbolLabel.text = @"\"";
    [containerView.symbolLabel sizeToFit];
    [containerView invalidateIntrinsicContentSize];
}

- (void) hideSymbol
{
    containerView.symbolLabel.text = nil;
    [containerView invalidateIntrinsicContentSize];
}

- (void) setText:(NSString *)text
{
    _text = text;
    containerView.distanceLabel.text = text;
    [self invalidateIntrinsicContentSize];
}

- (void) setTextColor:(UIColor *)textColor
{
    _textColor = textColor;
    containerView.distanceLabel.textColor = textColor;
    containerView.fractionLabel.textColor = textColor;
    containerView.symbolLabel.textColor = textColor;
    [self setNeedsDisplay];
}

- (void) setFont:(UIFont *)font
{
    _font = font;
    
    if (containerView.distanceLabel) // test if subviews have been setup
    {
        containerView.symbolLabel.font = font;
        containerView.fractionLabel.font = font;
        containerView.distanceLabel.font = font;
        
        [containerView invalidateIntrinsicContentSize];
        [containerView sizeToFit];
    }
}

- (void) setTextAlignment:(NSTextAlignment)textAlignment
{
    [self addAlignmentConstraint:textAlignment];
    _textAlignment = textAlignment;
    containerView.textAlignment = textAlignment;
    [self setNeedsUpdateConstraints];
}

- (void) setCenterAlignmentExcludesFraction:(BOOL)centerAlignmentExcludesFraction_
{
    centerAlignmentExcludesFraction = centerAlignmentExcludesFraction_;
    containerView.centerAlignmentExcludesFraction = centerAlignmentExcludesFraction_;
    if (self.textAlignment == NSTextAlignmentCenter) [self setNeedsLayout];
}

- (void) setShadowColor:(UIColor *)shadowColor
{
    _shadowColor = shadowColor;
    containerView.distanceLabel.shadowColor = shadowColor;
    containerView.distanceLabel.shadowOffset = CGSizeMake(1, 1);
    containerView.fractionLabel.shadowColor = shadowColor;
    containerView.fractionLabel.shadowOffset = CGSizeMake(1, 1);
    [containerView.fractionLabel setNeedsDisplay];
    containerView.symbolLabel.shadowColor = shadowColor;
    containerView.symbolLabel.shadowOffset = CGSizeMake(1, 1);
}

@end
