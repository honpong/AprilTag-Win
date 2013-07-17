//
//  RCCameraParameters.h
//  RCCore
//
//  Created by Eagle Jones on 6/28/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <Foundation/Foundation.h>

/** Represents the optical, or intrinsic properties of a camera.
 
 See http://www.vision.caltech.edu/bouguetj/calib_doc/ for more information.
 */
@interface RCCameraParameters : NSObject

/** The focal length of the camera, in units of pixels. */
@property(readonly,nonatomic) float focalLength;

/** The horizontal position of the optical center of the camera, in units of pixels.
 
 A nominal 640x480 camera has its optical center at position (319.5, 239.5). */
@property(readonly,nonatomic) float opticalCenterX;

/** The vertical position of the optical center of the camera, in units of pixels.
 
 A nominal 640x480 camera has its optical center at position (319.5, 239.5). */
@property(readonly,nonatomic) float opticalCenterY;

/** The coefficient of the second degree polynomial to compensate for radial distortion. In most representations, this is the first "K" parameter. */
@property(readonly,nonatomic) float radialSecondDegree;

/** The coefficient of the fourth degree polynomial to compensate for radial distortion. In most representations, this is the second "K" parameter. */
@property(readonly,nonatomic) float radialFourthDegree;

/** You will not typically need to instantiate this class yourself. */
- (id) initWithFocalLength:(float)focalLength withOpticalCenterX:(float)opticalCenterX withOpticalCenterY:(float)opticalCenterY withRadialSecondDegree:(float)radialSecondDegree withRadialFourthDegree:(float)radialFouthDegree;

@end
