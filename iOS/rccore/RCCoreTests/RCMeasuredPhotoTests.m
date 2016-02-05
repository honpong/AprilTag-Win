//
//  RCMeasuredPhotoTests.m
//  RCCore
//
//  Created by Jordan Miller on 7/1/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCMeasuredPhotoTests.h"

//#define SERVER @"localhost"
#define SERVER @"staging"
#define STAGING_EMAIL @"ben@realitycap.com"
#define STAGING_PASSWORD @"secret"
#define STAGING_BASE_URL @"https://internal.realitycap.com/"
#define LOCALHOST_EMAIL @"test_two@realitycap.com"
#define LOCALHOST_PASSWORD @"passWordOne666666"
#define LOCALHOST_BASE_URL @"http://localhost:8000/"

@implementation RCMeasuredPhotoTests

- (BOOL)waitForCompletion:(NSTimeInterval)timeoutSecs {
    NSDate *timeoutDate = [NSDate dateWithTimeIntervalSinceNow:timeoutSecs];
    
    do {
        [[NSRunLoop currentRunLoop] runMode:NSDefaultRunLoopMode beforeDate:timeoutDate];
        if([timeoutDate timeIntervalSinceNow] < 0.0)
            break;
    } while (!done);
    
    return done;
}

- (void)setUp
{
    [super setUp];
    done = NO;
    // Set-up code here.
}

- (void)tearDown
{
    [super tearDown];
}

- (void) testPngAndJsonUpload
{
    NSString *baseURL = [@"localhost" isEqualToString:SERVER] ? LOCALHOST_BASE_URL : STAGING_BASE_URL;
    NSString *userEmail = [@"localhost" isEqualToString:SERVER] ? LOCALHOST_EMAIL : STAGING_EMAIL;
    NSString *userPassWd = [@"localhost" isEqualToString:SERVER] ? LOCALHOST_PASSWORD : STAGING_PASSWORD;

    [RCHTTPClient initWithBaseUrl:baseURL withAcceptHeader:@"application/vnd.realitycap.json; version=1.0" withApiVersion:1];
    RCUserManager* userMan = [RCUserManager sharedInstance];
    
    [userMan
         loginWithUsername:userEmail
         withPassword:userPassWd
         onSuccess:^()
         {
             done = YES;
         }
         onFailure:^(int statusCode)
         {
             STFail(@"loginWithUsername:withPassword called failure block with status code %i", statusCode);
             done = YES;
         }];
    
    STAssertTrue([self waitForCompletion:10.0], @"Request timed out");
    done = NO;
    
    //create a RCMeasuredPhoto from fixtures.
    //create an array of featurePoints from fixtures
    NSArray *rcFeatureArrayFixture = [self createRCFeaturePointArrayFixture];
    RCMeasuredPhoto *measuredPhoto = [[RCMeasuredPhoto alloc] init];
    measuredPhoto.featurePoints = rcFeatureArrayFixture;
    //add some further fixture information
    measuredPhoto.pngFileName = @"test.png";
    measuredPhoto.fileName = @"test.json";
    [measuredPhoto setIdentifiers];
    measuredPhoto.timestamp = [NSDate date];
    measuredPhoto.jsonRepresntation = [measuredPhoto jsonRepresenation];
    measuredPhoto.imageData = [@"hello world" dataUsingEncoding:NSUTF8StringEncoding];
    
    //call upload
    [measuredPhoto upLoad:^()
     {
         done = YES;
     }
                onFailure:^(int statusCode)
     {
         STFail(@"loginWithStoredCredentials called failure block with status code %i", statusCode);
         done = YES;
     }];
    
    STAssertTrue([self waitForCompletion:10.0], @"Request timed out");
    
    //assert 200 response
    //call measured photo url, see if data available.

    
}

- (void) testJsonSerialization
{
    RCMeasuredPhoto *measuredPhoto = [[RCMeasuredPhoto alloc] init];

    //create an array of featurePoints from fixtures
    NSArray *rcFeatureArrayFixture = [self createRCFeaturePointArrayFixture];
    measuredPhoto.featurePoints = rcFeatureArrayFixture;
    //add some further fixture information
    measuredPhoto.pngFileName = @"test.png";
    measuredPhoto.fileName = @"test.json";
    [measuredPhoto setIdentifiers];
    
    //now look at the json representation
    NSString *measuredPhotoJsonStr = [measuredPhoto jsonRepresenation];

    //DLog(@"%@",measuredPhotoJsonStr);
    
    NSString *expectedJson = @"{ \
            \"featurePoints\" : [ \
                       { \
                           \"y\" : 1.2, \
                           \"id\" : 0, \
                           \"depth\" : { \
                               \"scalar\" : 0.3, \
                               \"standardDeviation\" : 0.7 \
                           }, \
                           \"worldPoint\" : { \
                               \"y\" : 1, \
                               \"stdz\" : 0, \
                               \"stdx\" : 0, \
                               \"z\" : 2, \
                               \"x\" : 1, \
                               \"stdy\" : 0 \
                           }, \
                           \"x\" : 1, \
                           \"initizlized\" : true \
                       }, \
                       { \
                            \"y\" : 1.4, \
                           \"id\" : 0, \
                           \"depth\" : { \
                               \"scalar\" : 0.9, \
                               \"standardDeviation\" : 0.65 \
                           }, \
                           \"worldPoint\" : { \
                               \"y\" : 1.2, \
                               \"stdz\" : 0, \
                               \"stdx\" : 0, \
                               \"z\" : 2.2, \
                               \"x\" : 1.1, \
                               \"stdy\" : 0 \
                           }, \
                           \"x\" : 1.1, \
                           \"initizlized\" : true \
                       } \
                       ], \
        \"fileName\" : \"test.json\", \
        \"bundleID\" : \"null\", \
        \"pngFileName\" : \"test.png\", \
        \"vendorUniqueId\" : \"null\" \
    }";
    
    STAssertTrue( [self noWhiteSpaceComapareString:measuredPhotoJsonStr toString:expectedJson] ,@"JSON serialization did not match expectation");
    
}

- (BOOL) noWhiteSpaceComapareString:(NSString *)stringOne toString:(NSString *)stringTwo
{
    NSArray* words = [stringOne componentsSeparatedByCharactersInSet :[NSCharacterSet whitespaceAndNewlineCharacterSet]];
    NSString* oneNoSpace = [words componentsJoinedByString:@""];

    words = [stringTwo componentsSeparatedByCharactersInSet :[NSCharacterSet whitespaceAndNewlineCharacterSet]];
    NSString* twoNoSpace = [words componentsJoinedByString:@""];

    //DLog(@"%@",oneNoSpace);
    //DLog(@"%@",twoNoSpace);
    
    return [oneNoSpace isEqualToString:twoNoSpace];
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
