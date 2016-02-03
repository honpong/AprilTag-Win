//
//  CLPlacemark+RCPlacemark.h
//  RCCore
//
//  Created by Ben Hirashima on 1/29/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <CoreLocation/CoreLocation.h>

@interface CLPlacemark (RCPlacemark)

- (NSString*)getFormattedAddress;
- (NSString*)getFormattedAddress:(CLPlacemark*)place;

@end
