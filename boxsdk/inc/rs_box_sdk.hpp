/*******************************************************************************

INTEL CORPORATION PROPRIETARY INFORMATION
This software is supplied under the terms of a license agreement or nondisclosure
agreement with Intel Corporation and may not be copied or disclosed except in
accordance with the terms of that agreement
Copyright(c) 2017 Intel Corporation. All Rights Reserved.

*******************************************************************************/
//
//  rs_box_sdk.hpp
//  boxsdk
//
//  Created by Hon Pong (Gary) Ho
//
#pragma once

#ifndef rs_box_sdk_hpp
#define rs_box_sdk_hpp

#ifdef RS2_MEASURE_EXPORTS
#if (defined(WIN32) || defined(WIN64) || defined(_WIN32) || defined(_WIN64))
#define RS2_MEASURE_DECL __declspec(dllexport)
#else
#define RS2_MEASURE_DECL __attribute__((visibility("default")))
#endif
#else
#define RS2_MEASURE_DECL
#endif

#include "librealsense2/rs.hpp"

#ifdef __cplusplus
extern "C"
{
#endif
    
    typedef enum rs2_measure_const 
    {
        RS2_MEASURE_BOX_MAXCOUNT = 10
    } rs2_measure_const;
        
    struct rs2_measure_box
    {
        float center[3];   /**< box center in 3d world coordinate           */
        float axis[3][3];  /**< box axis and lengths in 3d world coordinate */
    };

    struct rs2_box_measure;

    RS2_MEASURE_DECL void* rs2_box_measure_create(rs2_box_measure** box_measure, rs2_error** e);
    RS2_MEASURE_DECL void rs2_box_measure_set_depth_unit(rs2_box_measure* box_measure, float depth_unit, rs2_error** e);
    RS2_MEASURE_DECL int rs2_box_measure_get_boxes(rs2_box_measure* box_measure, rs2_measure_box* boxes, rs2_error** e);
    
#ifdef __cplusplus
}

#include <cmath>
namespace rs2
{
    struct box : public rs2_measure_box
    {
        box(rs2_measure_box& ref) : rs2_measure_box(ref) {}

        inline float dim_sqr(int a) const /**< squared box dimension along axis a in meter */
        {
            return axis[a][0] * axis[a][0] + axis[a][1] * axis[a][1] + axis[a][2] * axis[a][2];
        }

        inline float dim(int a, float scale = 1000.0f) const /**< box dimension alogn axis a in meter */
        {
            return std::sqrt(dim_sqr(a)) * scale;
        }

        inline std::string str() const
        {
            return std::to_string(int(dim(0))) + " x " + std::to_string(int(dim(1))) + " x " + std::to_string(int(dim(2))) + " mm^3";
        }
    };

    class box_measure
    {
    public:

        typedef std::vector<box> box_vector;

        box_measure(device dev = device()) : _queue(1)
        {
            rs2_error* e = nullptr;

            _block = std::shared_ptr<processing_block>((processing_block*)rs2_box_measure_create(&_box_measure, &e));
            error::handle(e);

            if (dev.get()) { try_set_depth_scale(dev); }

            _block->start(_queue);
        }

        frame process(frameset frame)
        {
            (*_block)(frame);
            rs2::frame f;
            _queue.poll_for_frame(&f);
            return f;
        }

        box_vector get_boxes()
        {
            rs2_error* e = nullptr;
            int nbox = rs2_box_measure_get_boxes(_box_measure, box, &e);
            error::handle(e);

            return box_vector(box, box + nbox);
        }

        bool try_set_depth_scale(device dev) try
        {
            rs2_error* e = nullptr;

            // Go over the device's sensors
            for (rs2::sensor& sensor : dev.query_sensors())
            {
                // Check if the sensor if a depth sensor
                if (rs2::depth_sensor dpt = sensor.as<rs2::depth_sensor>())
                {                   
                    rs2_box_measure_set_depth_unit(_box_measure, dpt.get_depth_scale(), &e);
                    error::handle(e);

                    return true;
                }
            }
            return false;
        }
        catch(...){ return false; }

    private:
        std::shared_ptr<processing_block> _block;
        frame_queue _queue;

        rs2_box_measure* _box_measure;
        rs2_measure_box box[RS2_MEASURE_BOX_MAXCOUNT];
    };
}


#endif

#endif /* rs_box_sdk_hpp */
