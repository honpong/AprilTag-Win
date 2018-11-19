#pragma once

/*
+ * This file exists because TM2 eeprom's calibration table format
+ * copied rc_tracker.h but then modified virtually every piece in a
+ * slightly different way (e.g. changed doubles to floats, or added an
+ * extra field) and so in order to parse it without including the
+ * entire TM2 project we had to copy all the structure definitions
+ * back here.
+ */

#define HEADER_MAJOR 1
#define HEADER_MINOR 0

typedef struct {
        uint8_t header_size;
        uint8_t header_major;
        uint8_t header_minor;
        uint8_t reserved0;
        uint16_t table_size;
        uint16_t table_id;
        uint8_t table_major;
        uint8_t table_minor;
        uint8_t reserved1[6];
} config_table_header;

#define QS_CAL_MAJOR 4
#define QS_CAL_MINOR 0
#define QS_CAL_MAJOR_VR 4
#define QS_CAL_MINOR_VR 1

#define MAC_ADDRESS_LENGTH 6
#define BYTES_LENGTH_PER_CONTROLLER 12
#define CONTROLLERS_TABLE_MAJOR 2
#define CONTROLLERS_TABLE_MINOR 0
typedef struct  {
        config_table_header header;
        uint8_t length1;
        uint8_t mac_address1[MAC_ADDRESS_LENGTH];
        uint8_t address_type1;
        uint32_t reserved1;
        uint8_t length2;
        uint8_t mac_address2[MAC_ADDRESS_LENGTH];
        uint8_t address_type2;
        uint32_t reserved2;
} controllers_devices_table;

#pragma pack(push, 1)
// Controller physical information. data is sent using controller_event_report_t structures with report type = REPORT_TYPE_CONTROLLER_PHYS_INFO
// size must be a multiple of 6
typedef struct {
    uint8_t report_version;         // must be 0 in this version
    uint8_t flags;                  // bit 0: 0- visible light, 1 ? IR
    // other bits are reserved to 0
    uint8_t orb_radius;             // millimeters
    uint8_t accel_hz;               // accelerometer frequency in Hz
    uint8_t gyro_hz;                // gyroscope frequency in Hz
    float   imu_rotation[4];        // IMU rotation from canonical system
    float   imu_translation[3];     // IMU translation from controller center of mass
    float   orb_to_imu_offset[3];   // ORB position (x,y,z) in IMU coordinate system
    uint8_t     imu_model;        // an enum that indicates the IMU part
    uint8_t     accel_bw;         // the accelerometer bandwidth
    uint8_t     gyro_bw;          // the gyroscope badnwidth
    uint16_t    accel_group_delay_us; // the group delay of the accelerometer, in uS
    uint16_t    gyro_group_delay_us; // the group delay of the gyroscope, in uS
    uint8_t     marker_radius;    // millimeters. 0 means there is no marker
    float       marker_to_imu_offset[3]; //LED position (x,y,z) in IMU coordinate system
    float       gyro_bias[3]; //the biases of conntrollers in 3 axes
    uint8_t     reserved;
} controller_phys_info;
#pragma pack(pop)

#define CONTROLLERS_PARAMS_TABLE_MAJOR 1
#define CONTROLLERS_PARAMS_TABLE_MINOR 0
typedef struct {
        config_table_header header;
        controller_phys_info info;
} controllers_params_table;

// Private copy of rc_tracker definitions
typedef struct { float v[3][3]; } eeprom_Matrix;
typedef union { struct { float x, y, z; }; float v[3]; } eeprom_Vector;
typedef union { struct { float x, y, z, w; }; float v[4]; } eeprom_Quaternion;

typedef struct { eeprom_Quaternion Q; eeprom_Vector T; eeprom_Matrix R; } eeprom_Pose; // both Q and R are always output, the closer to 1 of |Q| and |R| is used for input
typedef struct { eeprom_Vector W; eeprom_Vector T; } eeprom_PoseVariance; // this is not the full variance yet

                                                                                                                                                  /**
                                                                                                                                                  @param T Translation to the origin
                                                                                                                                                  @param W Rotation vector specifying the rotation to the origin
                                                                                                                                                  @param T_variance Variance for the translation
                                                                                                                                                  @param W_variance Variance for the rotation vector
                                                                                                                                                  */
typedef struct eeprom_Extrinsics {
        eeprom_Pose pose_m;
        eeprom_PoseVariance variance_m2;
} eeprom_Extrinsics;

typedef enum eeprom_CalibrationType {
        eeprom_CALIBRATION_TYPE_UNKNOWN,        // rd = ???
        eeprom_CALIBRATION_TYPE_FISHEYE,        // rd = arctan(2 * ru * tan(w / 2)) / w
        eeprom_CALIBRATION_TYPE_POLYNOMIAL3,    // rd = ru * (k1 * ru^2 + k2 * ru^4 + k3 * ru^6)
        eeprom_CALIBRATION_TYPE_UNDISTORTED,    // rd = ru
                                                // theta=arctan(ru)
        eeprom_CALIBRATION_TYPE_KANNALA_BRANDT4 // rd = theta + k1*theta^3 + k2*theta^5 + k3*theta^7 + k4*theta^9
} eeprom_CalibrationType;

/**
@param format Image format
@param width_px Image width in pixels
@param height_px Image height in pixels
@param f_x_px Focal length of camera in pixels
@param f_y_px Focal length of camera in pixels
@param c_x_px Horizontal principal point of camera in pixels
@param c_y_px Veritical principal point of camera in pixels
@param k1,k2,k3 Polynomial distortion parameters
@param w Fisheye camera field of view in radians (half-angle FOV)
*/
typedef struct eeprom_CameraIntrinsics {
        eeprom_CalibrationType type;
        uint32_t width_px, height_px;
        uint32_t reserved;
        float f_x_px, f_y_px;
        float c_x_px, c_y_px;
        union {
                float distortion[4];
                struct { float k1, k2, k3, k4; };
                float w;
        };
} eeprom_CameraIntrinsics;

typedef struct {
        eeprom_Matrix scale_and_alignment;
        eeprom_Vector bias_m__s2;
        eeprom_Vector bias_variance_m2__s4;
        float measurement_variance_m2__s4;
        eeprom_Vector temperature_bias_m__s2__C;
        float temperature_bias_variance_m2__s4__C2;
        eeprom_Vector temperature_scale__C;
        float temperature_scale_variance__C2;
        uint32_t decimate_by;
} eeprom_AccelerometerIntrinsics;

typedef struct {
        eeprom_Matrix scale_and_alignment;
        eeprom_Vector bias_rad__s;
        eeprom_Vector bias_variance_rad2__s2;
        float measurement_variance_rad2__s2;
        eeprom_Vector temperature_bias_rad__s__C;
        float temperature_bias_variance_rad2__s2__C2;
        eeprom_Vector temperature_scale__C;
        float temperature_scale_variance__C2;
        uint32_t decimate_by;
} eeprom_GyroscopeIntrinsics;

typedef struct {
        float measurement_variance_C2;
        float calibraiton_temperature_C;
} eeprom_ThermometerIntrinsics;

// end of rc_tracker structs

typedef struct {

        eeprom_Extrinsics Extrinsics;

        eeprom_CameraIntrinsics Intrinsics;

} CameraCalibration;

typedef struct {

        eeprom_Extrinsics Extrinsics;

        eeprom_AccelerometerIntrinsics AccIntrinsics;

        eeprom_GyroscopeIntrinsics GyroIntrinsics;

        eeprom_ThermometerIntrinsics ThermometerIntrinsics;

} ImuCalibration;

typedef struct {
        config_table_header header;
        CameraCalibration cam0;
        CameraCalibration cam1;
        ImuCalibration imu;
} qs_calibration_table;

typedef qs_calibration_table tm2_calibration_table;
