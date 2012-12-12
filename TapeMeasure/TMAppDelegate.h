//
//  TMAppDelegate.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 10/2/12.
//  Copyright (c) 2012 RealityCap. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <CoreLocation/CoreLocation.h>

@interface TMAppDelegate : UIResponder <UIApplicationDelegate, CLLocationManagerDelegate>
{
    
}

@property (strong, nonatomic) UIWindow *window;

@property (readonly, strong, nonatomic) NSManagedObjectContext *managedObjectContext;
@property (readonly, strong, nonatomic) NSManagedObjectModel *managedObjectModel;
@property (readonly, strong, nonatomic) NSPersistentStoreCoordinator *persistentStoreCoordinator;
@property (readonly, strong, nonatomic) CLLocationManager *locationManager;
@property (readonly, strong, nonatomic) CLLocation *location;
@property (readonly, strong, nonatomic) NSString *locationAddress;

- (void)saveContext;
- (NSURL *)applicationDocumentsDirectory;
- (void)startLocationUpdates;

@end
