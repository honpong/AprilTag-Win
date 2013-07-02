//
//  RCMeasuredPhotoTests.m
//  RCCore
//
//  Created by Jordan Miller on 7/1/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCMeasuredPhotoTests.h"

@implementation RCMeasuredPhotoTests


- (void)setUp
{
    [super setUp];
    // Set-up code here.
}

- (void)tearDown
{
    [super tearDown];
}

- (void) testPngAndJsonUpload
{
    //create a RCMeasuredPhoto from fixtures.
    //call upload
    //assert 200 response
    //call measured photo url, see if data available.

    
}

- (void) testJsonSerialization
{
    //create an array of featurePoints from fixtures
    NSArray *rcFeatureArrayFixture = [self createRCFeaturePointArrayFixture];
    RCFeaturePoint *feature = [rcFeatureArrayFixture objectAtIndex:0];
    NSDictionary *featureDic = [feature dictionaryRepresenation];
    //call serialize json on it.
    NSError *error;
    NSData *featureDicJsonData = [NSJSONSerialization dataWithJSONObject:featureDic
                                                       options:NSJSONWritingPrettyPrinted
                                                                   error:&error];
    NSString* featureJsonStr;
    featureJsonStr = [[NSString alloc] initWithData:featureDicJsonData encoding:NSUTF8StringEncoding];
    //print JSON TODO-> test that JSON matches expectations. 
    NSLog(@"%@",featureJsonStr);
}

- (NSArray*) createRCFeaturePointArrayFixture
{
    RCFeaturePoint* feature = [[RCFeaturePoint alloc]
                               initWithId:0
                               withX:1.0
                               withY:1.2
                               withDepth:[
                                          [RCScalar alloc]
                                          initWithScalar:0.3
                                          withStdDev:0.7]
                               withWorldPoint:[
                                               [RCPoint alloc]
                                               initWithX:1.0
                                               withY:1.0
                                               withZ:2.0]
                               withInitialized:YES
                               ];
    RCFeaturePoint* feature1 = [[RCFeaturePoint alloc]
                                initWithId:1
                                withX:1.1
                                withY:1.4
                                withDepth:[
                                           [RCScalar alloc]
                                           initWithScalar:0.9
                                           withStdDev:0.65]
                                withWorldPoint:[
                                                [RCPoint alloc]
                                                initWithX:1.1
                                                withY:1.2
                                                withZ:2.2]
                                withInitialized:YES
                                ];
    return [NSArray arrayWithObjects:feature, feature1, nil];
}

@end
