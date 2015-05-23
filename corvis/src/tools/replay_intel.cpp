#define _USE_MATH_DEFINES

#include "../filter/rc_intel_interface.h"

#include <iostream>
#include <fstream>
#include <string.h>
#include <assert.h>
#include <sstream>
#include <memory>
#include "../cor/packet.h"

const rc_Pose rc_pose_identity = {1, 0, 0, 0, 
                                  0, 1, 0, 0,
                                  0, 0, 1, 0}; 

const rc_Pose camera_pose      = { 0,-1,  0, 0.064f,
                                  -1, 0,  0, -0.017f,
                                   0, 0, -1, 0}; 


void logger(void * handle, const char * buffer_utf8, size_t length)
{
    fprintf((FILE *)handle, "%s\n", buffer_utf8);
}

int main(int c, char **v)
{
    if(c < 2)
    {
        fprintf(stderr, "specify file name\n");
        return 0;
    }
    const char *name = v[1];
    int width_px = 640;
    int height_px = 480;
    float center_x_px = (width_px-1) / 2.f;
    float center_y_px = (height_px-1) / 2.f;
    bool force_recognition = false;
    double a_bias_stdev = .02 * 9.8 / 2.;
    double w_bias_stdev = 10. / 180. * M_PI / 2.;

    rc_Vector bias_m__s2 = {0.001f, -0.225f, -0.306f};
    float noiseVariance_m2__s4 = a_bias_stdev * a_bias_stdev;
    rc_Vector bias_rad__s = {0.016f, -0.015f, 0.011f};
    float noiseVariance_rad2__s2 = w_bias_stdev * w_bias_stdev;
    rc_Camera camera = rc_EGRAY8;
    int focal_length_x_px = 627;
    int focal_length_y_px = 627;
    int focal_length_xy_px = 0;
    uint64_t shutter_time_100_ns = 16667 * 10;


    rc_Tracker * tracker = rc_create();
    rc_setLog(tracker, logger, false, 0, stderr);
    rc_configureCamera(tracker, camera, camera_pose, width_px, height_px, center_x_px, center_y_px, focal_length_x_px, focal_length_xy_px, focal_length_y_px);
    rc_configureAccelerometer(tracker, rc_pose_identity, bias_m__s2, noiseVariance_m2__s4);
    rc_configureGyroscope(tracker, rc_pose_identity, bias_rad__s, noiseVariance_rad2__s2);

    rc_startTracker(tracker);

    bool first_packet = false;
    bool is_running = true;
    int packets_dispatched = 0;

    std::ifstream file;
    std::ifstream::pos_type size;

    file.open(name, std::ios::binary);
    if(file.bad() || file.eof() || !file.is_open())
    {
        std::cerr << "Couldn't open file " << name << " for reading.\n";
        return false;
    }
    file.seekg(0, std::ios::end);
    auto end = file.tellg();
    file.seekg (0, std::ios::beg);
    auto begin = file.tellg();
    size = end - begin;

    packet_header_t header;
    file.read((char *)&header, 16);
    if(file.bad() || file.eof()) is_running = false;

    while (is_running) {
        auto phandle = std::unique_ptr<void, void(*)(void *)>(malloc(header.bytes), free);
        auto packet = (packet_t *)phandle.get();
        packet->header = header;
        
        file.read((char *)packet->data, header.bytes - 16);
        if(file.bad() || file.eof())
        {
            is_running = false;
        }
        else
        {
            switch(header.type)
            {
                case packet_camera:
                {
                    int camera_width, camera_height;
                    char tmp[17];
                    memcpy(tmp, packet->data, 16);
                    tmp[16] = 0;
                    std::stringstream parse(tmp);
                    //pgm header is "P5 x y"
                    parse.ignore(3, ' ') >> camera_width >> camera_height;
                    assert(width_px == camera_width && height_px == camera_height);
                    const uint8_t * image = packet->data + 16;
                    int stride = camera_width;
                    rc_Timestamp time_100_ns = (header.time + 16667) * 10;
                    rc_receiveImage(tracker, camera, time_100_ns, shutter_time_100_ns, rc_pose_identity, force_recognition, stride, image);
                    break;
                }
                case packet_accelerometer:
                {
                    rc_Timestamp time_100_ns = header.time * 10;
                    rc_Vector acceleration_m__s2;
                    acceleration_m__s2.x = ((float *)packet->data)[0];
                    acceleration_m__s2.y = ((float *)packet->data)[1];
                    acceleration_m__s2.z = ((float *)packet->data)[2];
                    rc_receiveAccelerometer(tracker, time_100_ns, acceleration_m__s2);
                    break;
                }
                case packet_gyroscope:
                {
                    rc_Timestamp time_100_ns = header.time * 10;
                    rc_Vector angular_velocity_rad__s;
                    angular_velocity_rad__s.x = ((float *)packet->data)[0];
                    angular_velocity_rad__s.y = ((float *)packet->data)[1];
                    angular_velocity_rad__s.z = ((float *)packet->data)[2];
                    rc_receiveGyro(tracker, time_100_ns, angular_velocity_rad__s);
                    break;
                }
                case packet_imu:
                {
                    auto imu = (packet_imu_t *)packet;
                    rc_Timestamp time_100_ns = header.time * 10;

                    rc_Vector acceleration_m__s2;
                    acceleration_m__s2.x = imu->w[0];
                    acceleration_m__s2.y = imu->w[1];
                    acceleration_m__s2.z = imu->w[2];
                    rc_receiveAccelerometer(tracker, time_100_ns, acceleration_m__s2);

                    rc_Vector angular_velocity_rad__s;
                    angular_velocity_rad__s.x = imu->a[0];
                    angular_velocity_rad__s.y = imu->a[1];
                    angular_velocity_rad__s.z = imu->a[2];
                    rc_receiveGyro(tracker, time_100_ns, angular_velocity_rad__s);
                    break;
                }
                case packet_filter_control:
                {
                    if(header.user == 1)
                    {
                        //start measuring
                        rc_Timestamp time_100_ns = header.time * 10;
                        rc_reset(tracker, time_100_ns, rc_pose_identity);
                    }
                }
                if(!first_packet) {
                    rc_Timestamp time_100_ns = header.time * 10;
                    rc_reset(tracker, time_100_ns, rc_pose_identity);
                    first_packet = true;
                }
            }
            packets_dispatched++;

            rc_triggerLog(tracker);
        }
        
        file.read((char *)&header, 16);
        if(file.bad() || file.eof()) is_running = false;
    }
    file.close();

    rc_stopTracker(tracker);
    rc_triggerLog(tracker);

    rc_destroy(tracker);

    return 0;
}
