//
//  RC3DKPlusWebController.h
//  Sunlayar
//
//  Created by Ben Hirashima on 2/25/15.
//  Copyright (c) 2015 Sunlayar. All rights reserved.
//

#import "RC3DKWebController.h"
#import "SLMeasuredPhoto.h"

@interface RC3DKPlusWebController : RC3DKWebController <RCStereoDelegate>

@property (readonly) BOOL isStereoRunning;
@property (nonatomic, readonly) SLMeasuredPhoto* measuredPhoto;

@end
