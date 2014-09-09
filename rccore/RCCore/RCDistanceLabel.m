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

@interface RCDistanceLabel ()

@property (readwrite) UILabel* distanceLabel;
@property (readwrite) RCFractionLabel* fractionLabel;
@property (readwrite) UILabel* symbolLabel;

@end

@implementation RCDistanceLabel
{
    UIView* containerView;
    NSLayoutConstraint* heightConstraint;
    NSLayoutConstraint* containerHeightConstraint;
    NSLayoutConstraint* distHeightConstraint;
    NSLayoutConstraint* symbolHeightConstraint;
    NSLayoutConstraint* symbolTrailingSpaceConstraint;
    NSLayoutConstraint* alignmentConstraint;
    NSLayoutConstraint* distLeadingSpaceConstraint;
    NSLayoutConstraint* distTrailingSpaceConstraint;
}
@synthesize distanceLabel, fractionLabel, symbolLabel, centerAlignmentExcludesFraction;

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
    self.text = nil; // clears any text entered in storyboard
    [self addWidthConstraint:self.bounds.size.width andHeightConstraint:self.bounds.size.height];
    
    containerView = [[UIView alloc] initWithFrame:CGRectZero];
    containerView.translatesAutoresizingMaskIntoConstraints = NO;
    containerView.backgroundColor = [UIColor clearColor];
    [self addSubview:containerView];
    
    distanceLabel = [[UILabel alloc] initWithFrame:CGRectZero];
    distanceLabel.translatesAutoresizingMaskIntoConstraints = NO;
    distanceLabel.font = self.font;
    distanceLabel.textColor = self.textColor;
    distanceLabel.textAlignment = NSTextAlignmentRight;
    distanceLabel.backgroundColor = [UIColor clearColor];
    distanceLabel.text = self.text;
    [containerView addSubview:distanceLabel];
    
    fractionLabel = [[RCFractionLabel alloc] initWithFrame:CGRectZero];
    fractionLabel.translatesAutoresizingMaskIntoConstraints = NO;
    fractionLabel.font = self.font;
    fractionLabel.backgroundColor = [UIColor clearColor];
    fractionLabel.textColor = self.textColor;
    [containerView addSubview:fractionLabel];
    
    symbolLabel = [[UILabel alloc] initWithFrame:CGRectZero];
    symbolLabel.translatesAutoresizingMaskIntoConstraints = NO;
    symbolLabel.font = self.font;
    symbolLabel.textColor = self.textColor;
    symbolLabel.backgroundColor = [UIColor clearColor];
    symbolLabel.text = @"\"";
    [containerView addSubview:symbolLabel];
    
    [self setShadowColor:self.shadowColor];
    
    heightConstraint = [NSLayoutConstraint constraintWithItem:self
                                                    attribute:NSLayoutAttributeHeight
                                                    relatedBy:NSLayoutRelationEqual
                                                       toItem:nil
                                                    attribute:NSLayoutAttributeNotAnAttribute
                                                   multiplier:1.
                                                     constant:self.frame.size.height];
    [self addConstraint:heightConstraint];
    
    
    [containerView addConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"H:[distanceLabel]-1-[fractionLabel][symbolLabel]"
                                                                          options:0
                                                                          metrics:nil
                                                                            views:NSDictionaryOfVariableBindings(distanceLabel, fractionLabel, symbolLabel)]];
    
    // container view
    [containerView addCenterYInSuperviewConstraints];
    
    containerHeightConstraint = [containerView getHeightConstraint:0];
    [containerView addConstraint:containerHeightConstraint];
    
    // symbol label
    [symbolLabel addBottomSpaceToSuperviewConstraint:0];
    
    symbolHeightConstraint = [symbolLabel getHeightConstraint:self.frame.size.height];
    [symbolLabel addConstraint:symbolHeightConstraint];
    
    // fraction label
    [fractionLabel addBottomSpaceToSuperviewConstraint:0];
    
    // distance label
    [distanceLabel addBottomSpaceToSuperviewConstraint:0];
    distLeadingSpaceConstraint = [distanceLabel addLeftSpaceToSuperviewConstraint:0];
    distHeightConstraint = [distanceLabel getHeightConstraint:self.frame.size.height];
    [distanceLabel addConstraint:distHeightConstraint];
    
    [self setCenterAlignmentExcludesFraction:NO];
    
    [self addAlignmentConstraint:self.textAlignment];
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
    
    [containerView setNeedsUpdateConstraints];
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
    }
    else
    {
        distanceLabel.text = [distObj description];
        [self hideSymbol];
        [self hideFraction];
    }
    [self sizeToFit];
}

- (void) setDistanceImperialFractional:(RCDistanceImperialFractional*)distObj
{
    if (distObj.fraction.nominator > 0)
    {
        [self showFraction];
        [fractionLabel setNominator:distObj.fraction.nominator andDenominator:distObj.fraction.denominator];
    }
    else
    {
        [self hideFraction];
    }
    
    if (distObj.wholeInches + distObj.fraction.nominator == 0)
        [self hideSymbol];
    else
        [self showSymbol];
    
    distanceLabel.text = [distObj getStringWithoutFractionOrUnitsSymbol];
}

- (void) showFraction
{
    fractionLabel.hidden = NO;
}

- (void) hideFraction
{
    fractionLabel.hidden = YES;
}

- (void) showSymbol
{
    symbolLabel.text = @"\"";
}

- (void) hideSymbol
{
    symbolLabel.text = nil;
}

- (void) setText:(NSString *)text
{
    [super setText:nil];
    distanceLabel.text = text;
    [distanceLabel sizeToFit];
    [self setNeedsDisplay];
}

- (void) setTextColor:(UIColor *)textColor
{
    [super setTextColor:textColor];
    distanceLabel.textColor = textColor;
    fractionLabel.textColor = textColor;
    symbolLabel.textColor = textColor;
    [self setNeedsDisplay];
}

- (void) setFont:(UIFont *)font
{
    [super setFont:font];
    
    if (distanceLabel) // test if subviews have been setup
    {
        symbolLabel.font = font;
        fractionLabel.font = font;
        distanceLabel.font = font;
        
        [self sizeToFit];
    }
}

- (void) setTextAlignment:(NSTextAlignment)textAlignment
{
    [self addAlignmentConstraint:textAlignment];
    [super setTextAlignment:textAlignment];
}

- (void) setCenterAlignmentExcludesFraction:(BOOL)centerAlignmentExcludesFraction_
{
    centerAlignmentExcludesFraction = centerAlignmentExcludesFraction_;
    
    if (centerAlignmentExcludesFraction_)
    {
        [symbolLabel removeConstraint:symbolTrailingSpaceConstraint];
        distTrailingSpaceConstraint = [distanceLabel addRightSpaceToSuperviewConstraint:32];
    }
    else
    {
        symbolTrailingSpaceConstraint = [symbolLabel addRightSpaceToSuperviewConstraint:0];
        [distanceLabel removeConstraint:distTrailingSpaceConstraint];
    }
    
    [self addAlignmentConstraint:self.textAlignment];
    [self sizeToFit];
}

- (void) setShadowColor:(UIColor *)shadowColor
{
    [super setShadowColor:shadowColor];
    distanceLabel.shadowColor = shadowColor;
    distanceLabel.shadowOffset = CGSizeMake(1, 1);
    fractionLabel.shadowColor = shadowColor;
    fractionLabel.shadowOffset = CGSizeMake(1, 1);
    [fractionLabel setNeedsDisplay];
    symbolLabel.shadowColor = shadowColor;
    symbolLabel.shadowOffset = CGSizeMake(1, 1);
}

- (void) sizeToFit
{
    [symbolLabel sizeToFit];
    [fractionLabel sizeToFit];
    [distanceLabel sizeToFit];
    
    CGSize fractionSize = [fractionLabel sizeThatFits:fractionLabel.bounds.size];
    CGSize symbolSize = [symbolLabel sizeThatFits:symbolLabel.bounds.size];
    
    containerHeightConstraint.constant = fractionSize.height;
    [containerView setNeedsUpdateConstraints];
    
    distHeightConstraint.constant = fractionSize.height;
    
    if (self.centerAlignmentExcludesFraction)
    {
        if (fractionSize.width + symbolSize.width == 0)
        {
            distLeadingSpaceConstraint.constant = 0;
            distTrailingSpaceConstraint.constant = 0;
        }
        else
        {
            distTrailingSpaceConstraint.constant = self.font.pointSize / 17 * 32;
            distLeadingSpaceConstraint.constant = distTrailingSpaceConstraint.constant;
        }
    }
    
    [distanceLabel setNeedsUpdateConstraints];
    
    
    symbolHeightConstraint.constant = fractionSize.height;
    [symbolLabel setNeedsUpdateConstraints];
    
    [containerView sizeToFit];
}

@end
