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

@interface TMAppDelegate : UIResponder <UIApplicationDelegate, CLLocationManagerDelegate>
{
    struct outbuffer _databuffer;
}

@property (strong, nonatomic) UIWindow *window;

@property (readonly, strong, nonatomic) NSManagedObjectContext *managedObjectContext;
@property (readonly, strong, nonatomic) NSManagedObjectModel *managedObjectModel;
@property (readonly, strong, nonatomic) NSPersistentStoreCoordinator *persistentStoreCoordinator;
@property (readonly, strong, nonatomic) CLLocationManager *locationManager;
@property (readonly, strong, nonatomic) CLLocation *location;
@property (readonly, strong, nonatomic) NSString *locationAddress;
@property (strong) RCVideoCap *videoCap;
@property (strong) AVCaptureSession *avSession;
@property (strong) CMMotionManager *motionMan;
@property (strong) RCMotionCap *motionCap;
@property (strong) AVCaptureVideoPreviewLayer *captureVideoPreviewLayer;

- (void)saveContext;
- (NSURL *)applicationDocumentsDirectory;
- (void)startLocationUpdates;
- (void)setupDataCapture;
- (struct outbuffer*)getBuffer;

@end
