//
//  RCSensorFusionError.h
//  RC3DK
//
//  Created by Ben Hirashima on 6/17/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface RCSensorFusionError : NSError

/**
 - RCSensorFusionErrorCodeNone = 0 - No error has occurred.
 - RCSensorFusionErrorCodeVision = 1 - No visual features were detected in the most recent image. This is normal in some circumstances, such as quick motion or if the device temporarily looks at a blank wall. However, if this is received repeatedly, it may indicate that the camera is covered or it is too dark. RCSensorFusion will continue.
 - RCSensorFusionErrorCodeTooFast = 2 - The device moved more rapidly than expected for typical handheld motion. This may indicate that RCSensorFusion has failed and is providing invalid data. RCSensorFusion will continue.
 - RCSensorFusionErrorCodeOther = 3 - A fatal internal error has occured. Please contact RealityCap and provide [RCSensorFusionStatus statusCode] from the status property of the last received RCSensorFusionData object. RCSensorFusion will be reset.
 - RCSensorFusionErrorCodeLicense = 4 - A license error indicates that the license has not been properly validated, or needs to be validated again.
 */
- (NSInteger) code;

@end
