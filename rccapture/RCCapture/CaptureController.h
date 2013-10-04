//
//  CaptureController.h
//  RCCapture
//
//  Created by Brian on 10/3/13.
//  Copyright (c) 2013 Realitycap. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>

enum packet_type {
    packet_none = 0,
    packet_camera = 1,
    packet_imu = 2,
    packet_feature_track = 3,
    packet_feature_select = 4,
    packet_navsol = 5,
    packet_feature_status = 6,
    packet_filter_position = 7,
    packet_filter_reconstruction = 8,
    packet_feature_drop = 9,
    packet_sift = 10,
    packet_plot_info = 11,
    packet_plot = 12,
    packet_plot_drop = 13,
    packet_recognition_group = 14,
    packet_recognition_feature = 15,
    packet_recognition_descriptor = 16,
    packet_map_edge = 17,
    packet_filter_current = 18,
    packet_feature_variance = 19,
    packet_accelerometer = 20,
    packet_gyroscope = 21,
    packet_filter_feature_id_visible = 22,
    packet_filter_feature_id_association = 23,
    packet_feature_intensity = 24,
    packet_filter_control = 25,
    packet_ground_truth = 26,
    packet_core_motion = 27,
};

typedef struct {
    uint32_t bytes; //size of packet including header
    uint16_t type;  //id of packet
    uint16_t user;  //packet-defined data
    uint64_t time;  //time in microseconds
} packet_header_t;

typedef struct {
    packet_header_t header;
    uint8_t data[];
} packet_t;

typedef struct {
    packet_header_t header;
    uint8_t data[];
} packet_camera_t;

typedef struct {
    packet_header_t header;
    float a[3]; // m/s^2
    float w[3]; // rad/s
} packet_imu_t;

typedef struct {
    packet_header_t header;
    float a[3]; // m/s^2
} packet_accelerometer_t;

typedef struct {
    packet_header_t header;
    float w[3]; // rad/s
} packet_gyroscope_t;

@protocol CaptureControllerDelegate <NSObject>

- (void) captureDidStop;
- (void) captureDidFinish;
@optional
- (void) captureDidStart;

@end

@interface CaptureController : NSObject <AVCaptureVideoDataOutputSampleBufferDelegate>

@property (weak, nonatomic) id<CaptureControllerDelegate> delegate;

- (void)startCapture:(NSString *)path withSession:(AVCaptureSession *)session withDelegate:(id<CaptureControllerDelegate>)delegate;
- (void)stopCapture;

@end
