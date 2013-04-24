//
//  OverlayView.h
//  AugmentedRealitySample
//
//  Created by Chris Greening on 01/01/2010.
//

#import <UIKit/UIKit.h>

@interface ARView : UIView {
	CGMutablePathRef pathToDraw;
}

@property (nonatomic, assign) CGMutablePathRef pathToDraw;

@end

