//
//  TM2DTape.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 6/13/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMTickMarksLayerDelegate.h"
#import "RCCore/RCDeviceInfo.h"
#import "RCCore/RCDistance.h"

@interface TM2DTapeView : UIImageView

- (void)drawTickMarksWithUnits:(Units)units;
- (void)moveTapeWithXDisp:(float)x withDistance:(float)meters withUnits:(Units)units;

@end
