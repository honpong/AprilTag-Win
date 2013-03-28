//  Created by Ben Hirashima on 11/30/12.
//  Copyright (c) 2012 RealityCap. All rights reserved.
//

#import "RCDistanceFormatter.h"

//inner class
@interface Fraction : NSObject

@property int nominator;
@property int denominator;

- (BOOL) isEqualToOne;

@end

@implementation Fraction

+ (Fraction*) fractionWithInches:(float)inches
{
    return [[Fraction alloc] initWithInches:inches];
}

- (id) initWithInches:(float)inches
{
    if(self = [super init])
    {
        int wholeInches = floor(inches);
        float remainder = inches - wholeInches;
        
        int sixteenths = roundf(remainder * 16);
        
        int gcd = [Fraction gcdForNumber1:sixteenths andNumber2:16];
        
        self.nominator = sixteenths / gcd;
        self.denominator = 16 / gcd;
    }
    
    return self;
}

- (BOOL) isEqualToOne
{
    return self.nominator / self.denominator == 1 ? YES : NO;
}

+ (int) gcdForNumber1:(int) m andNumber2:(int) n
{
    if(!(m && n)) return 1;
    
    while( m!= n) // execute loop until m == n
    {
        if( m > n)
            m= m - n; // large - small , store the results in large variable
        else
            n= n - m;
    }
    return m;
}

@end

@implementation RCDistanceFormatter

+ (NSString*)getFormattedDistance:(float)meters withUnits:(Units)units withScale:(UnitsScale)scale withFractional:(BOOL)fractional
{
    NSString *unitsSymbol;
    double convertedDist;
    
    if(units == UnitsMetric)
    {
        if(scale == UnitsScaleCM)
        {
            unitsSymbol = @"cm";
            convertedDist = meters * 100; //convert to cm
            return [NSString localizedStringWithFormat:@"%0.1f%@", convertedDist, unitsSymbol];
        }
        else if(scale == UnitsScaleKM)
        {
            unitsSymbol = @"km";
            convertedDist = meters / 1000; //convert to km
            return [NSString localizedStringWithFormat:@"%0.3f%@", convertedDist, unitsSymbol];
        }
        else if(scale == UnitsScaleM)
        {
            unitsSymbol = @"m";
            return [NSString localizedStringWithFormat:@"%0.2f%@", meters, unitsSymbol];
        }
        else
        {
            //TODO: throw NSError
            return @"ERROR";
        }
    }
    else if(units == UnitsImperial)
    { 
        convertedDist = meters * INCHES_PER_METER; //convert to inches
        
        if(scale == UnitsScaleFT)
        {
            if(fractional)
            {
                return [self getFormattedFractionalFeet:convertedDist];
            }
            else
            {
                convertedDist = convertedDist / INCHES_PER_FOOT;
                return [NSString localizedStringWithFormat:@"%0.2f\'", convertedDist];
            }
        }
        else if(scale == UnitsScaleYD)
        {
            if(fractional)
            {
                return [self getFormattedFractionalYards:convertedDist];
            }
            else
            {
                convertedDist = convertedDist / INCHES_PER_YARD;
                return [NSString localizedStringWithFormat:@"%0.2fyd", convertedDist];
            }
        }
        else if(scale == UnitsScaleMI)
        {
            if(fractional)
            {
                return [self getFormattedFractionalMiles:convertedDist];
            }
            else
            {
                convertedDist = convertedDist / INCHES_PER_MILE;
                return [NSString localizedStringWithFormat:@"%0.3fmi", convertedDist];
            }
        }
        else if(scale == UnitsScaleIN)
        {
            if(fractional)
            {
                return [self getFormattedFractionalInches:convertedDist];
            }
            else
            {
                return [NSString localizedStringWithFormat:@"%0.1f\"", convertedDist];
            }
        }
        else
        {
            //TODO: throw NSError
            return @"ERROR";
        }
    }
    else
    {
        //TODO: throw NSError
        return @"ERROR"; 
    }
}

+ (NSString*)getFormattedFractionalMiles:(double)inches
{
    NSString *result = @"";
    
    double miles = inches / INCHES_PER_MILE;
    int wholeMiles = floor(miles);
    double remainingInches = inches - (wholeMiles * INCHES_PER_MILE);
    
    result = [NSString localizedStringWithFormat:@"%imi", wholeMiles];
    if (remainingInches == 0) return result;

    if (INCHES_PER_MILE - remainingInches < INCHES_PER_FOOT) //if remaining inches are within 1' of a whole mile
    {
        return [NSString stringWithFormat:@"%imi", (wholeMiles + 1)]; //round up to a whole mile
    }
    
    NSString *fractionalFeet = [self getFormattedFractionalFeet:remainingInches];
    if(result.length > 0 && fractionalFeet.length) result = [NSString stringWithFormat:@"%@ ", result]; //add space if there are any whole miles
    return [NSString stringWithFormat:@"%@%@", result, fractionalFeet];
}

+ (NSString*)getFormattedFractionalYards:(double)inches
{
    NSString *result = @"";
    
    double yards = inches / INCHES_PER_YARD;
    int wholeYards = floor(yards);
    double remainingInches = inches - (wholeYards * INCHES_PER_YARD);
    
    result = [NSString localizedStringWithFormat:@"%iyd", wholeYards];
    if (remainingInches == 0) return result;
        
    if (INCHES_PER_YARD - remainingInches < SIXTEENTH_INCH) //if remaining inches are within 1/32" of a whole yard
    {
        return [NSString stringWithFormat:@"%iyd", (wholeYards + 1)]; //round up to a whole yard
    }
    
    NSString *fractionalFeet = [self getFormattedFractionalFeet:remainingInches];
    if(result.length && fractionalFeet.length) result = [NSString stringWithFormat:@"%@ ", result]; //add space if there are any whole yards
    return [NSString stringWithFormat:@"%@%@", result, fractionalFeet];
}

+ (NSString*)getFormattedFractionalFeet:(double)inches
{
    NSString *result = @"";
    
    double feet = inches / INCHES_PER_FOOT;
    int wholeFeet = floor(feet);
    double remainingInches = inches - (wholeFeet * 12);
    
    if (feet >= 1) result = [NSString localizedStringWithFormat:@"%i'", wholeFeet];
    if (remainingInches == 0) return result;
    
    if (INCHES_PER_FOOT - remainingInches < SIXTEENTH_INCH) //if remaining inches are within 1/32" of a whole foot
    {
        return [NSString stringWithFormat:@"%i'", (wholeFeet + 1)]; //round up to a whole foot
    }
    
    NSString *fractionalInches = [self getFormattedFractionalInches:remainingInches];
    if(result.length && fractionalInches.length) result = [NSString stringWithFormat:@"%@ ", result]; //add space if there are any whole feet
    return [NSString stringWithFormat:@"%@%@", result, fractionalInches];
}

+ (NSString*)getFormattedFractionalInches:(double)inches
{
    NSString *result = @"";
    
    if (inches <= 0) return @"0\"";
   
    Fraction* fract = [Fraction fractionWithInches:inches];
        
    if([fract isEqualToOne])
    {
        result = [NSString localizedStringWithFormat:@"%0.0f", ceil(inches)];
    }
    else
    {
        if(inches >= 1)
        {
            result = [NSString localizedStringWithFormat:@"%0.0f", floor(inches)];
        }
    }
    
    if(fract.nominator && (float)fract.denominator / (float)fract.nominator > 1)
    {
        if(result.length > 0) result = [NSString stringWithFormat:@"%@ ", result]; //add space if there are any whole inches
        result = [NSString stringWithFormat:@"%@%u/%u", result, fract.nominator, fract.denominator];
    }
    
    if (result.length) result = [NSString stringWithFormat:@"%@\"", result]; //add " inches symbol
    return result;
}

+ (UnitsScale)autoSelectUnitsScale:(float)meters withUnits:(Units)units
{
    if (units == UnitsImperial)
    {
        float inches = meters * INCHES_PER_METER;
        
        if (inches < INCHES_PER_FOOT) return UnitsScaleIN;
        if (inches >= INCHES_PER_MILE) return UnitsScaleMI;
        return UnitsScaleFT; //default
    }
    else
    {
        if (meters < 1) return UnitsScaleCM;
        if (meters >= 1000) return UnitsScaleKM;
        return UnitsScaleM; //default
    }
}

@end


