//
//  TrueMeasureSDK.h
//  TrueMeasureSDK
//
//  Created by Ben Hirashima on 10/15/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <Foundation/Foundation.h>

static NSString* kRCQueryStringApiKey = @"apikey";

typedef NS_ENUM(int, RCTrueMeasureErrorCode)
{
    RCTrueMeasureErrorCodeMissingApiKey = 100,
    RCTrueMeasureErrorCodeLicenseValidationFailure = 200,
    RCTrueMeasureErrorCodeLicenseInvalid = 300,
    RCTrueMeasureErrorCodeWrongLicenseType = 400,
    RCTrueMeasureErrorCodeInvalidAction = 500,
    RCTrueMeasureErrorCodeUnsupportedApiVersion = 600
};

@interface TrueMeasureSDK : NSObject

@end
