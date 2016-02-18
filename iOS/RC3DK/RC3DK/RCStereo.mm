//
//  RCStereo.m
//  RCCore
//
//  Created by Eagle Jones on 5/20/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "RCStereo.h"
#import "RCConstants.h"

#import <CoreImage/CoreImage.h>
#import <UIKit/UIKit.h>

#include "../../../corvis/src/numerics/vec4.h"
#include "../../../corvis/src/numerics/quaternion.h"
#include "../../../corvis/src/stereo/stereo.h"

@interface RCStereo ()

@property (nonatomic) NSString * fileBaseName;
@property (nonatomic) NSURL* workingDirectory;
@property (nonatomic) NSString* guid;
@property (nonatomic) UIDeviceOrientation orientation;

@end

@implementation RCStereo
{
    stereo mystereo;
    NSString * texture_path;
}

+ (id) sharedInstance
{
    static RCStereo *instance = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        instance = [[self alloc] init];
    });
    return instance;
}

- (id) init
{
    self = [super init];
    
    if (self)
    {
        LOGME
    }
    
    return self;
}

- (NSString *) timeStampedFilenameWithSuffix:(NSString *)suffix
{
    NSURL * documentURL = [[[NSFileManager defaultManager] URLsForDirectory:NSDocumentDirectory inDomains:NSUserDomainMask] lastObject];
    
    NSDateFormatter *dateFormatter = [[NSDateFormatter alloc] init];
    [dateFormatter setDateFormat:@"yyyy-MM-dd_HH-mm-ss"];
    
    NSDate *date = [NSDate date];
    NSString * formattedDateString = [dateFormatter stringFromDate:date];
    NSMutableString * filename = [[NSMutableString alloc] initWithString:formattedDateString];
    [filename appendString:suffix];
    NSURL *fileUrl = [documentURL URLByAppendingPathComponent:filename];
    
    return [fileUrl path];
}

- (void) writeSampleBuffer:(CMSampleBufferRef)sampleBuffer withFilename:(NSString *)filename
{
    if(!sampleBuffer) return;
    CVImageBufferRef imageBuffer = CMSampleBufferGetImageBuffer(sampleBuffer);
    if(!imageBuffer) return;
    CVPixelBufferLockBaseAddress(imageBuffer, 0);
    CIImage * image = [CIImage imageWithCVPixelBuffer:imageBuffer];
    CIContext *context = [CIContext contextWithOptions:nil];

    CGImageRef cgimage = [context createCGImage:image fromRect:image.extent];
    UIImage * lastImage = [UIImage imageWithCGImage:cgimage];

    
    DLog(@"Writing to %@", filename);
    
    if(![UIImageJPEGRepresentation(lastImage, .8) writeToFile:filename atomically:YES])
    {
        DLog(@"FAILED");
    }

    CFRelease(cgimage);
    CVPixelBufferUnlockBaseAddress(imageBuffer, 0);
}

- (void) processFrame:(RCSensorFusionData *)data withFinal:(bool)final
{
    camera s;
    
    CMSampleBufferRef sampleBuffer = data.sampleBuffer;
    if(!sampleBuffer) return;
    CVPixelBufferRef pixelBuffer = (CVPixelBufferRef)CMSampleBufferGetImageBuffer(sampleBuffer);
    if(!pixelBuffer) return;
    CVPixelBufferLockBaseAddress(pixelBuffer, kCVPixelBufferLock_ReadOnly);
    unsigned char *pixel = (unsigned char *)CVPixelBufferGetBaseAddressOfPlane(pixelBuffer,0);
    
    s.width = (int)CVPixelBufferGetWidth(pixelBuffer);
    s.height = (int)CVPixelBufferGetHeight(pixelBuffer);
    v4 T = v4_from_vFloat(data.cameraTransformation.translation.vector);
    rotation_vector W = to_rotation_vector(quaternion(data.cameraTransformation.rotation.quaternionW, data.cameraTransformation.rotation.quaternionX, data.cameraTransformation.rotation.quaternionY, data.cameraTransformation.rotation.quaternionZ));
    s.focal_length = data.cameraParameters.focalLength;
    s.center_x = data.cameraParameters.opticalCenterX;
    s.center_y = data.cameraParameters.opticalCenterY;
    s.k1 = data.cameraParameters.radialSecondDegree;
    s.k2 = data.cameraParameters.radialFourthDegree;
    s.k3 = 0.;

    list<stereo_feature> features;
    for (RCFeaturePoint *feature in data.featurePoints) {
        features.push_back(stereo_feature(feature.id, v4(feature.x, feature.y, 0., 0.)));
    }
    mystereo.process_frame(s, T, W, pixel, features, final);
    
    if(final) {
        [self writeSampleBuffer:data.sampleBuffer withFilename:texture_path];
    }
    
    CVPixelBufferUnlockBaseAddress(pixelBuffer, kCVPixelBufferLock_ReadOnly);
}

- (RCPoint *) triangulatePoint:(CGPoint)point
{
    v4 world;
    bool success = mystereo.triangulate(point.x, point.y, world);
    if(!success)
        return NULL;

    RCPoint * feature = [[RCPoint alloc] initWithX:world[0] withY:world[1] withZ:world[2]];
    return feature;
}


- (RCPoint *) triangulatePointWithMesh:(CGPoint)point
{
    v4 world;
    bool success = mystereo.triangulate_mesh(point.x, point.y, world);
    if(!success)
        return NULL;

    RCPoint * feature = [[RCPoint alloc] initWithX:world[0] withY:world[1] withZ:world[2]];
    return feature;
}

static void sensor_fusion_stereo_progress(float progress)
{
    static RCStereo * stereo;
    if(!stereo)
        stereo = [RCStereo sharedInstance];

    if(stereo.delegate && [stereo.delegate respondsToSelector:@selector(stereoDidUpdateProgress:)]) {
        dispatch_async(dispatch_get_main_queue(), ^{
            [stereo.delegate stereoDidUpdateProgress:progress];
        });
    }
}

- (void) setOrientation:(UIDeviceOrientation) orientation
{
    _orientation = orientation;
    
    enum stereo_orientation sensor_orientation;
    switch(orientation) {
        case UIDeviceOrientationLandscapeLeft:
            sensor_orientation = STEREO_ORIENTATION_LANDSCAPE_LEFT;
            break;
        case UIDeviceOrientationPortrait:
            sensor_orientation = STEREO_ORIENTATION_PORTRAIT;
            break;
        case UIDeviceOrientationLandscapeRight:
            sensor_orientation = STEREO_ORIENTATION_LANDSCAPE_RIGHT;
            break;
        case UIDeviceOrientationPortraitUpsideDown:
            sensor_orientation = STEREO_ORIENTATION_PORTRAIT_UPSIDE_DOWN;
            break;
        default:
            sensor_orientation = STEREO_ORIENTATION_LANDSCAPE_LEFT;
    }
    mystereo.set_orientation(sensor_orientation);
}

// Preprocesses the stereo data with the current frame
- (bool) preprocess
{
    bool success;
    success = mystereo.preprocess(false, sensor_fusion_stereo_progress);
    if(success) {
        success = mystereo.preprocess_mesh(sensor_fusion_stereo_progress);
    }

    dispatch_async(dispatch_get_main_queue(), ^{
        if(_delegate && [_delegate respondsToSelector:@selector(stereoDidFinish)])
            [_delegate stereoDidFinish];
    });
    return success;

}

- (void) setWorkingDirectory:(NSURL *)workingDirectory
{
    _workingDirectory = workingDirectory;
}

- (void) setGuid:(NSString *)guid
{
    _guid = guid;
}

- (void) setWorkingDirectory:(NSURL *)workingDirectory andGuid:(NSString*)guid andOrientation:(UIDeviceOrientation)orientation
{
    [self setWorkingDirectory:workingDirectory];
    [self setGuid:guid];
    [self setOrientation:orientation];
    [self setupFileBaseName];
}

- (void) setupFileBaseName
{
    if (self.guid && self.workingDirectory)
    {
        self.fileBaseName = [NSString stringWithFormat:@"%@/%@-stereo", self.workingDirectory.resourceSpecifier, self.guid];
        texture_path = [self.fileBaseName stringByAppendingString:@".jpg"];
        NSString * texture_filename = [[[NSURL URLWithString:texture_path] pathComponents] lastObject];
        
        mystereo.set_debug_basename([self.fileBaseName UTF8String]);
        mystereo.set_debug_texture_filename([texture_filename UTF8String]);
    }
}

-(void) reset
{
    mystereo.reset();
}
@end
