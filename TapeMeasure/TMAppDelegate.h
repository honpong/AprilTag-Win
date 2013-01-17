//
//  TMAppDelegate.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 10/2/12.
//  Copyright (c) 2012 RealityCap. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <CoreLocation/CoreLocation.h>
#import "RCCore/cor.h"

@class RCVideoCap, AVCaptureSession, RCMotionCap, CMMotionManager, AVCaptureVideoPreviewLayer;
@protocol TMAVSessionManager;

@interface TMAppDelegate : UIResponder <UIApplicationDelegate, CLLocationManagerDelegate>
{
    struct outbuffer _databuffer;
    bool isVideoSetup; //indicates whether video setup and config is finished. sadly, the AVCaptureSession class does not provide such a flag, so we make our own.
}

@property (strong, nonatomic) UIWindow *window;

@property (readonly, strong, nonatomic) NSManagedObjectContext *managedObjectContext;
@property (readonly, strong, nonatomic) NSManagedObjectModel *managedObjectModel;
@property (readonly, strong, nonatomic) NSPersistentStoreCoordinator *persistentStoreCoordinator;
@property (readonly, strong, nonatomic) CLLocationManager *locationManager;
@property (readonly, strong, nonatomic) CLLocation *location;
@property (readonly, strong, nonatomic) NSString *locationAddress;
@property (strong) RCVideoCap *videoCap;
//@property (weak) AVCaptureSession *avSession;
@property (strong) CMMotionManager *motionMan;
@property (strong) RCMotionCap *motionCap;
//@property (strong) AVCaptureVideoPreviewLayer *captureVideoPreviewLayer;
//@property (strong) id<TMAVSessionManager> sessionMan;

- (void)saveContext;
- (NSURL *)applicationDocumentsDirectory;
- (void)startLocationUpdates;
- (void)setupDataCapture;
- (void)setupVideoCapture;
- (void)teardownVideoCapture;
- (struct outbuffer*)getBuffer;

@end
