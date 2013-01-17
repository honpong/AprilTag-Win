//
//  TMAppDelegate.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 10/2/12.
//  Copyright (c) 2012 RealityCap. All rights reserved.
//

#import "TMAppDelegate.h"
#import "RCCore/RCCore.h"
#import "TMHistoryVC.h"
#import "TMMeasurement.h"
#import <CoreLocation/CoreLocation.h>
#import "TMDistanceFormatter.h"
#import <RCCore/RCVideoCap.h>
#import "RCCore/RCMotionCap.h"
#import "RCCore/cor.h"
#import <CoreMotion/CoreMotion.h>
#import "TMAvSessionManagerFactory.h"

@implementation TMAppDelegate

@synthesize managedObjectContext = _managedObjectContext;
@synthesize managedObjectModel = _managedObjectModel;
@synthesize persistentStoreCoordinator = _persistentStoreCoordinator;

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
    // Register the preference defaults early.
    NSDictionary *appDefaults = [NSDictionary dictionaryWithObjectsAndKeys:
                                 [NSNumber numberWithInt:UnitsMetric], @"Units", 
                                 [NSNumber numberWithInt:UnitsScaleM], @"Scale", 
                                 [NSNumber numberWithInt:0], @"Fractional",
                                 nil];
    
    [[NSUserDefaults standardUserDefaults] registerDefaults:appDefaults];
        
    isVideoSetup = false;
//    [self performSelectorInBackground:@selector(setupDataCapture) withObject:nil];
    
    return YES;
}

- (void)applicationWillResignActive:(UIApplication *)application
{
    // Sent when the application is about to move from active to inactive state. This can occur for certain types of temporary interruptions (such as an incoming phone call or SMS message) or when the user quits the application and it begins the transition to the background state.
    // Use this method to pause ongoing tasks, disable timers, and throttle down OpenGL ES frame rates. Games should use this method to pause the game.
    NSLog(@"applicationWillResignActive");
    
    [self stopLocationUpdates];
    
    id<TMAVSessionManager> sessionMan = [TMAvSessionManagerFactory getAVSessionManagerInstance];
    
    if (sessionMan.isRunning)
    {
        [self.videoCap stopVideoCap];
        [self.motionCap stopMotionCapture];
//        [self stopAVSession];
    }
}

- (void)applicationDidEnterBackground:(UIApplication *)application
{
    // Use this method to release shared resources, save user data, invalidate timers, and store enough application state information to restore your application to its current state in case it is terminated later.
    // If your application supports background execution, this method is called instead of applicationWillTerminate: when the user quits.
}

- (void)applicationWillEnterForeground:(UIApplication *)application
{
    // Called as part of the transition from the background to the inactive state; here you can undo many of the changes made on entering the background.
}

- (void)applicationDidBecomeActive:(UIApplication *)application
{
    // Restart any tasks that were paused (or not yet started) while the application was inactive. If the application was previously in the background, optionally refresh the user interface.
    NSLog(@"applicationDidBecomeActive");
}

- (void)applicationWillTerminate:(UIApplication *)application
{
    // Saves changes in the application's managed object context before the application terminates.
    NSLog(@"applicationWillTerminate");
    [self saveContext];
    
    [[TMAvSessionManagerFactory getAVSessionManagerInstance] endSession];
}

- (void)saveContext
{
    NSError *error = nil;
    NSManagedObjectContext *managedObjectContext = self.managedObjectContext;
    if (managedObjectContext != nil) {
        if ([managedObjectContext hasChanges] && ![managedObjectContext save:&error]) {
            // Replace this implementation with code to handle the error appropriately.
            // abort() causes the application to generate a crash log and terminate. You should not use this function in a shipping application, although it may be useful during development.
            NSLog(@"Unresolved error %@, %@", error, [error userInfo]);
            abort();
        }
    }
}

#pragma mark - Core Data stack

// Returns the managed object context for the application.
// If the context doesn't already exist, it is created and bound to the persistent store coordinator for the application.
- (NSManagedObjectContext *)managedObjectContext
{
    if (_managedObjectContext != nil) {
        return _managedObjectContext;
    }
    
    NSPersistentStoreCoordinator *coordinator = [self persistentStoreCoordinator];
    if (coordinator != nil) {
        _managedObjectContext = [[NSManagedObjectContext alloc] init];
        [_managedObjectContext setPersistentStoreCoordinator:coordinator];
    }
    return _managedObjectContext;
}

// Returns the managed object model for the application.
// If the model doesn't already exist, it is created from the application's model.
- (NSManagedObjectModel *)managedObjectModel
{
    if (_managedObjectModel != nil) {
        return _managedObjectModel;
    }
    NSURL *modelURL = [[NSBundle mainBundle] URLForResource:@"TMMeasurementDM" withExtension:@"momd"];
    _managedObjectModel = [[NSManagedObjectModel alloc] initWithContentsOfURL:modelURL];
    return _managedObjectModel;
}

// Returns the persistent store coordinator for the application.
// If the coordinator doesn't already exist, it is created and the application's store added to it.
- (NSPersistentStoreCoordinator *)persistentStoreCoordinator
{
    if (_persistentStoreCoordinator != nil) {
        return _persistentStoreCoordinator;
    }
    
    NSURL *storeURL = [[self applicationDocumentsDirectory] URLByAppendingPathComponent:@"TapeMeasure.sqlite"];
    
    NSError *error = nil;
    _persistentStoreCoordinator = [[NSPersistentStoreCoordinator alloc] initWithManagedObjectModel:[self managedObjectModel]];
    if (![_persistentStoreCoordinator addPersistentStoreWithType:NSSQLiteStoreType configuration:nil URL:storeURL options:nil error:&error]) {
        /*
         Replace this implementation with code to handle the error appropriately.
         
         abort() causes the application to generate a crash log and terminate. You should not use this function in a shipping application, although it may be useful during development.
         
         Typical reasons for an error here include:
         * The persistent store is not accessible;
         * The schema for the persistent store is incompatible with current managed object model.
         Check the error message to determine what the actual problem was.
         
         
         If the persistent store is not accessible, there is typically something wrong with the file path. Often, a file URL is pointing into the application's resources directory instead of a writeable directory.
         
         If you encounter schema incompatibility errors during development, you can reduce their frequency by:
         * Simply deleting the existing store:
         [[NSFileManager defaultManager] removeItemAtURL:storeURL error:nil]
         
         * Performing automatic lightweight migration by passing the following dictionary as the options parameter:
         @{NSMigratePersistentStoresAutomaticallyOption:@YES, NSInferMappingModelAutomaticallyOption:@YES}
         
         Lightweight migration will only work for a limited set of schema changes; consult "Core Data Model Versioning and Data Migration Programming Guide" for details.
         
         */
        NSLog(@"Unresolved error %@, %@", error, [error userInfo]);
        abort();
    }
    
    return _persistentStoreCoordinator;
}

#pragma mark - Application's Documents directory

// Returns the URL to the application's Documents directory.
- (NSURL *)applicationDocumentsDirectory
{
    return [[[NSFileManager defaultManager] URLsForDirectory:NSDocumentDirectory inDomains:NSUserDomainMask] lastObject];
}

#pragma mark - Location services

- (void)startLocationUpdates
{
    NSLog(@"startLocationUpdates");
    
    if (nil == _locationManager) _locationManager = [[CLLocationManager alloc] init];
        
    self.locationManager.delegate = self;
    self.locationManager.desiredAccuracy = kCLLocationAccuracyBest;
        
    // Set a movement threshold for new events.
    self.locationManager.distanceFilter = 500;
        
    [self.locationManager startUpdatingLocation];    
}

- (void)stopLocationUpdates
{
    NSLog(@"stopLocationUpdates");
    [self.locationManager stopUpdatingLocation];
}

//for iOS 6
- (void)locationManager:(CLLocationManager *)manager didUpdateLocations:(NSArray *)locations
{
    [self updateStoredLocation:locations.lastObject];
}

//for iOS 5
- (void)locationManager:(CLLocationManager *)manager didUpdateToLocation:(CLLocation *)newLocation fromLocation:(CLLocation *)oldLocation
{
    [self updateStoredLocation:newLocation];
}

- (void)updateStoredLocation:(CLLocation*)newLocation
{
    // If it's a relatively recent event, turn off updates to save power
    _location = newLocation;
    
    NSLog(@"lat %+.4f, long %+.4f, acc %.0fm", self.location.coordinate.latitude, self.location.coordinate.longitude, self.location.horizontalAccuracy);
    
    NSDate* eventDate = self.location.timestamp;
    NSTimeInterval howRecent = [eventDate timeIntervalSinceNow];
    
    if (abs(howRecent) < 15.0) {
        if(self.location.horizontalAccuracy <= 65)
        {
            [self reverseGeocode];
            [self stopLocationUpdates];
        }
    }
}

- (void)reverseGeocode
{
    CLGeocoder *geocoder = [[CLGeocoder alloc] init];
    
    [geocoder reverseGeocodeLocation:self.location completionHandler:^(NSArray *placemarks, NSError *error) {
        if (error){
            NSLog(@"Geocode failed with error: %@", error);
            return;
        }
        if(placemarks && placemarks.count > 0)
        {
            //do something
            CLPlacemark *topResult = [placemarks objectAtIndex:0];
            
            _locationAddress = [NSString stringWithFormat:@"%@ %@, %@, %@",
                                    [topResult subThoroughfare],[topResult thoroughfare],
                                    [topResult locality], [topResult administrativeArea]];
//            NSLog(self.locationAddress);
        }
    }];
}

#pragma mark - Data capture setup

- (void)setupDataCapture
{
    NSLog(@"setupDataCapture");
    
    [self setupVideoSession];
    if (_motionCap == nil) [self setupMotionCapture];
}

- (void)setupMotionCapture
{
	NSLog(@"setupMotionCapture");
    
    _motionMan = [[CMMotionManager alloc] init];
    _motionCap = [[RCMotionCap alloc] initWithMotionManager:self.motionMan withOutput:&_databuffer];
}

- (void)setupVideoSession
{
	NSLog(@"setupVideoSession");
    
    id<TMAVSessionManager> sessionMan = [TMAvSessionManagerFactory getAVSessionManagerInstance];
    
    [sessionMan startSession];
    
    [self setupVideoCapture];
    
    isVideoSetup = true;
}

- (void)setupVideoCapture
{
    NSLog(@"setupVideoCapture");
    
    id<TMAVSessionManager> sessionMan = [TMAvSessionManagerFactory getAVSessionManagerInstance];
    _videoCap = [[RCVideoCap alloc] initWithSession:sessionMan.session withOutput:&_databuffer];
}

- (void)teardownVideoCapture
{
    NSLog(@"teardownVideoCapture");
    
    if (_videoCap != nil)
    {
        [_videoCap stopVideoCap];
        _videoCap = nil;
    }
}

- (AVCaptureDevice *) cameraWithPosition:(AVCaptureDevicePosition) position
{
    NSArray *devices = [AVCaptureDevice devicesWithMediaType:AVMediaTypeVideo];
    for (AVCaptureDevice *device in devices) {
        if ([device position] == position) {
            return device;
        }
    }
    return nil;
}

- (struct outbuffer*)getBuffer
{
    return &_databuffer;
}

//- (void)startAVSession
//{
//    NSLog(@"startAVSession");
//    [_sessionMan startSession];
//}
//
//- (void)stopAVSession
//{
//    NSLog(@"stopAVSession");
//    [_sessionMan endSession];
//}



@end
