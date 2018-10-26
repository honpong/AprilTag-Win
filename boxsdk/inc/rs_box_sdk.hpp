/*******************************************************************************
 
 Copyright 2015-2018 Intel Corporation.
 
 This software and the related documents are Intel copyrighted materials,
 and your use of them is governed by the express license under which
 they were provided to you. Unless the License provides otherwise,
 you may not use, modify, copy, publish, distribute, disclose or transmit
 this software or the related documents without Intel's prior written permission.
 
 This software and the related documents are provided as is, with no express or
 implied warranties, other than those that are expressly stated in the License.
 
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

#define RS2_BOX_SDK_MAJOR_VERSION    2
#define RS2_BOX_SDK_MINOR_VERSION    0
#define RS2_BOX_SDK_PATCH_VERSION    7
#define RS2_BOX_SDK_BUILD_VERSION    0

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
#include "librealsense2/rs_advanced_mode.hpp"

#ifdef __cplusplus
extern "C"
{
#endif
    
    /** \brief Constant definitions for measurement */
    typedef enum rs2_measure_const : int
    {
        RS2_STREAM_CAM_STATE = 0,      /**< mandatory output (data) stream from box measure */
        RS2_STREAM_DEPTH_DENSE = 3,    /**< optional dense depth stream from box measure    */
        RS2_STREAM_PLANE = 4,          /**< optional plane id stream from box measure       */
        RS2_STREAM_BOXCAST = 5,        /**< optional raycasted box depth from box measure   */
        RS2_MEASURE_PARAM_PRESET = 6,  /**< configure preset parameters for box measure     */
        RS2_MEASURE_USE_COLOR = 2,     /**< initialize color stream process in box measure  */
        RS2_MEASURE_BOX_MAXCOUNT = 10, /**< maximum number of boxes from box measure        */
    } rs2_measure_const;
    
    /** \brief Primary data structure for box */
    typedef struct rs2_measure_box
    {
        float center[3];   /**< box center in 3d world coordinate           */
        float axis[3][3];  /**< box axis and lengths in 3d world coordinate */
    } rs2_measure_box;
    
    /** \brief Auxilary data structure to display box on a 2d image */
    typedef struct rs2_measure_box_wireframe
    {
        float end_pt[12][2][2]; /**< 12 lines by 12 pairs of 2d pixel coorindate */
    } rs2_measure_box_wireframe;
    
    /** \brief Camera state info returned by box measure */
    typedef struct rs2_measure_camera_state
    {
        rs2_intrinsics* intrinsics;
        float camera_pose[12];
    } rs2_measure_camera_state;
    
    /** \brief Box measure object internal to a processing_block*/
    typedef struct rs2_box_measure rs2_box_measure;
    
    /**
     * Creates box measure processing block.
     * \param[out] box_measure pointer to box_measure object inside the returned processing_block. Do not delete directly.
     * \param[in]  depth_unit  number of meters represented by a single depth unit, see RS2_OPTION_DEPTH_UNITS enum.
     * \param[in]  custom      array of camera instrinsics for input [0] depth and [1] color image.
     * \param[in]  d2c         depth to color image extrinsics.
     * \param[out] error       if non-null, receives any error that occurs during this call, otherwise, errors are ignored.
     * \return processing_block object newly created. User is responsible for deleting this object.
     */
    RS2_MEASURE_DECL void* rs2_box_measure_create(rs2_box_measure** box_measure, float depth_unit, rs2_intrinsics custom[2], rs2_extrinsics* d2c, rs2_error** e);
    
    /**
     * Configure box measure object. Available configuration includes:
     *  RS2_MEASURE_PARAM_PRESET...flag 1: small, 2: medium, 3: large boxes parameter preset
     *  RS2_MEASURE_USE_COLOR......flag 0: OFF,   1: color edge fitting (must init before 1st frame)
     *  RS2_STREAM_DEPTH_DENSE.....flag 0: OFF,   1: ON dense depth output
     *  RS2_STREAM_PLANE...........flag 0: OFF,   1: ON plane index output
     *
     * to enable specific optional output stream.
     * \param[in]  box_measure pointer to box_measure object inside a processing_block.
     * \param[in]  config      configuration option.
     * \param[in]  value       configuration value.
     * \param[out] error       if non-null, receives any error that occurs during this call, otherwise, errors are ignored.
     */
    RS2_MEASURE_DECL void rs2_box_measure_configure(rs2_box_measure* box_measure, const rs2_measure_const config, double value, rs2_error** e);
    
    /**
     * Reset box measure.
     * \parma[out] error if non-null, receives any error that occurs during this call, otherwise, errors are ignored.
     */
    RS2_MEASURE_DECL void rs2_box_measure_reset(rs2_box_measure* box_measure, rs2_error** e);
    
    /**
     * Get boxes.
     * \param[in]  box_measure pointer to box_measure object inside a processing_block.
     * \param[out] boxes       array of output boxes.
     * \param[out] error       if non-null, receives any error that occurs during this call, otherwise, errors are ignored.
     * \return number of valid boxes in the boxes array.
     */
    RS2_MEASURE_DECL int rs2_box_measure_get_boxes(rs2_box_measure* box_measure, rs2_measure_box* boxes, rs2_error** e);
    
    /**
     * Project box onto a frame.
     * \param[in] box        a box returned from box_measure to be drawn on a target 2d image.
     * \param[in] camera     camera state of the target 2d image.
     * \param[out] wireframe the output box wireframe for rendering.
     * \param[out] error     if non-null, receives any error that occurs during this call, otherwise, errors are ignored.
     */
    RS2_MEASURE_DECL void rs2_box_meausre_project_box_onto_frame(const rs2_measure_box* box, const rs2_measure_camera_state* camera, rs2_measure_box_wireframe* wireframe, rs2_error** e);
    
    /**
     * Create a box raycasting processing block from box_measure. Do not delete directly.
     * The return block will be deleted automatically by the box_measure host.
     * \param[in] box_measure pointer to box_measure object.
     * \param[out] error      if non-null, receives any error that occurs during this call, otherwise, errors are ignored.
     * \return processing block for box raycasting, do not delete directly.
     */
    RS2_MEASURE_DECL void* rs2_box_measure_raycast_create(rs2_box_measure* box_measure, rs2_error** e);
    
    /**
     * Set box raycasting target boxes. rs2_box_measure_raycast_create() must be called before.
     * \param[in]  box_measure pointer to box_measure object contains raycasting object.
     * \param[in]  boxes       array of target boxes.
     * \param[in]  num_box     number of boxes in the boxes array.
     * \param[out] error       if non-null, receives any error that occurs during this call, otherwise, errors are ignored.
     */
    RS2_MEASURE_DECL void rs2_box_measure_raycast_set_boxes(rs2_box_measure* box_measure, const rs2_measure_box* boxes, int num_box, rs2_error** e);
    
    /**
     * Get a Intel(c) RealSense(TM) icon image.
     * \param[out] icon_width  returns width of RealSense icon in number of pixels.
     * \param[out] icon_height returns height of RealSense icon in number of pixels.
     * \param[out] error       if non-null, receives any error that occurs during this call, otherwise, errors are ignored.
     * \return icon image data in RGB format.
     */
    RS2_MEASURE_DECL const char* rs2_measure_get_realsense_icon(int* icon_width, int* icon_height, rs2_format* format, rs2_error** e);
    
#ifdef __cplusplus
}

#include <cmath>
#include <iomanip>
namespace rs2
{
    static std::string f_str(float v, int p = 2) {
        std::ostringstream ss; ss << std::fixed << std::setprecision(p) << v; return ss.str();
    }
    
    static std::string stri(float v, int w, int p = 0) {
        std::string s0 = f_str(v, p), s = ""; while ((int)(s + s0).length() < w) { s += " "; } return s + s0;
    }
    
    /** \brief output data structure from box_measure class.
     * box_frameset[RS2_STREAM_CAM_STATE]  stores metadata of this box_frameset.
     * box_frameset[RS2_STREAM_DEPTH] forwards the depth input of box_measure::process().
     * box_frameset[RS2_STREAM_COLOR] forwards the color input of box_measure::process().
     * box_frameset[RS2_STREAM_DEPTH_DENSE] shares newly created dense depth image (optional), \see box_measure::configure().
     *                                      if RS2_STREAM_DEPTH_DENSE not configured, it repeats RS2_STREAM_DEPTH.
     * box_frameset[RS2_STREAM_PLANE] shares newly created plane id image (optional), \see box_measure::configure().
     *                                if RS2_STREAM_PLANE not configured, it repeats RS2_STREAM_COLOR.
     * box_frameset[RS2_STREAM_BOXCAST] shares newly created raycasted box depth image.
     *                                  \see box_measure::raycast_box_onto_frame().
     */
    class box_frameset : public frameset
    {
    public:
        typedef rs2_measure_camera_state camera_state;
        
        const camera_state& state(const rs2_stream& s) const { return ((camera_state*)((*this)[RS2_STREAM_CAM_STATE].get_data()))[s]; }
        
        operator bool() const { return size() > RS2_STREAM_PLANE; }
        
    protected:
        box_frameset(frame& f) : frameset(f) {}
        
        friend class box_measure;
    };
    
    struct box : public rs2_measure_box
    {
        typedef rs2_measure_box_wireframe wireframe;
        
        box(rs2_measure_box& ref) : rs2_measure_box(ref) {}
        
        /** Squared box dimension along axis a in meter
         * \param[in] a  either 0, 1 or 2 selection of dimension. Order of dimension is arbitary.
         * \return squared box dimension algon axis a.
         */
        inline float dim_sqr(int a) const
        {
            return axis[a][0] * axis[a][0] + axis[a][1] * axis[a][1] + axis[a][2] * axis[a][2];
        }
        
        /** Box dimension along axis a in default millimeter
         * \param[in] a     either 0, 1 or 2 selection of dimension. Order of dimension is arbitary.
         * \param[in] scale factor to change the output unit.
         * \return box dimension along axis a.
         */
        inline float dim(int a, float scale = 1000.0f) const
        {
            return std::sqrt(dim_sqr(a)) * scale;
        }
        
        /** Box dimension in millimeter in string format. */
        inline std::string str() const
        {
            return stri(dim(0), 4) + "x" + stri(dim(1), 4) + "x" + stri(dim(2), 4) + " ";
        }
        
        /**
         * Project this box onto a frame to get a 2d wireframe.
         * \param[in] fs         target frameset where the box to be drawn.
         * \param[in] s          stream enum selects the stream in target frameset, e.g. RS2_STREAM_COLOR or RS2_STREAM_DEPTH.
         * \return the output box wireframe for rendering on the target frame.
         */
        inline wireframe project_box_onto_frame(box_frameset& fs, const int& s) const
        {
            return project_box_onto_frame(fs.state((rs2_stream)s));
        }
        
        /**
         * Project this box onto a frame to get a 2d wireframe.
         * \param[in] camera     state of the camera of the target frame.
         * \return the output box wireframe for rendering on the target frame.
         */
        wireframe project_box_onto_frame(const rs2_measure_camera_state& camera) const
        {
            rs2_error* e = nullptr;
            
            wireframe box_frame;
            rs2_box_meausre_project_box_onto_frame(this, &camera, &box_frame, &e);
            error::handle(e);
            
            return box_frame;
        }
    };
    
    struct box_vector : public std::vector<box>
    {
        template<typename T>
        box_vector(T a, T b) : std::vector<box>(a, b) {}
        
        operator bool() const { return size() > 0; }
    };
    
    /** \brief box raycasting utility for box_measure only. */
    class box_raycast
    {
        /**
         * Construct a box raycasting processing object by a box_measure only.
         * \param[in] host parent box_measure handler.
         */
        box_raycast(rs2_box_measure* host) : _host(host)
        {
            rs2_error* e = nullptr;
            _block = (processing_block*)rs2_box_measure_raycast_create(_host, &e);
            error::handle(e);
            
            _block->start(_queue);
        }
        
        /**
         * Perform raycasting of a box detected from a frameset to create an ideal box depth image.
         * The output depth image is in the RS_STREAM_BOXCAST channel of the input frameset.
         * New depth buffer will be created if not provided by input frameset.
         * This function is for box_measure only.
         * \param[in] b  target box.
         * \param[in] fs box frameset from where the box was detected.
         * \return frameset where RS_STREAM_BOXCAST channel contains the output box depth image.
         */
        frameset proc(const box& b, box_frameset& fs)
        {
            rs2_error* e = nullptr;
            rs2_box_measure_raycast_set_boxes(_host, &b, 1, &e);
            error::handle(e);
            
            _block->invoke(fs);
            
            frame dst;
            _queue.poll_for_frame(&dst);
            return frameset(dst);
        }
        
        rs2_box_measure* _host;
        processing_block* _block;
        frame_queue _queue;
        
        friend class box_measure;
    };
    
    class box_measure
    {
    public:
        
        /** \brief calibration data for box_measure. */
        struct calibration
        {
            rs2_intrinsics intrinsics[2];  /**< intrinsics of [0] depth and [1] color streams. */
            rs2_extrinsics color_to_depth; /**< color to depth stream extrinsics.              */
        };
        
        /**
         * Construct a box_measure object.
         * Width and height of depth and color streams are assumed to be the same.
         * \param[in] dev    device source of the depth and color streams.
         * \param[in] custom optional calibration data override the hardware calibrations.
         */
        box_measure(device dev = device(), calibration* custom = nullptr) : _queue(1), _stream_w(0), _stream_h(0)
        {
            rs2_error* e = nullptr;
            
            float depth_unit = try_get_depth_scale(dev);
            auto mode = set_sensor_options(dev);
            
            printf("box sdk:     %d.%d.%d,\ndepth unit:  %f,\nsensor mode: %s\n", RS2_BOX_SDK_MAJOR_VERSION, RS2_BOX_SDK_MINOR_VERSION, RS2_BOX_SDK_PATCH_VERSION, depth_unit, mode.c_str());
            
            _block = std::shared_ptr<processing_block>((processing_block*)rs2_box_measure_create(&_box_measure, depth_unit,
                                                                                                 custom ? custom->intrinsics : nullptr, custom ? &custom->color_to_depth : nullptr, &e));
            if (_use_color){ configure(RS2_MEASURE_USE_COLOR,1); }
            error::handle(e);
            
            _block->start(_queue);
        }
        
        /** Reset box_measure */
        void reset()
        {
            rs2_error *e = nullptr;
            
            rs2_box_measure_reset(_box_measure, &e);
            error::handle(e);
        }
        
        /**
         * Configure box measure object. Available configuration includes:
         *  RS2_MEASURE_PARAM_PRESET...flag 1: small, 2: medium, 3: large boxes parameter preset
         *  RS2_STREAM_DEPTH_DENSE.....flag 0: OFF,   1: ON dense depth output
         *  RS2_STREAM_PLANE...........flag 0: OFF,   1: ON plane index output
         *
         * to enable specific optional output stream.
         * \param[in]  box_measure pointer to box_measure object inside a processing_block.
         * \param[in]  config      configuration option.
         * \param[in]  value       configuration value.
         */
        void configure(const rs2_measure_const& config, double value)
        {
            rs2_error *e = nullptr;
            rs2_box_measure_configure(_box_measure, config, value, &e);
            error::handle(e);
        }
        inline void configure(const rs2_measure_const& config, bool flag){ configure(config, flag ? 1.0 : 0.0); }
        inline void configure(const rs2_measure_const& config, int value){ configure(config, (double)value); }
        
        
        /**
         * Get the required camera configuration for librealsense camera pipeline.
         * \return required camera configuration.
         */
        config get_camera_config() const
        {
            config config;
            config.enable_stream(RS2_STREAM_DEPTH, 0, _stream_w, _stream_h, RS2_FORMAT_Z16);
            
            if (_camera_name == "Intel RealSense 410" ||
                _camera_name == "Intel RealSense D410")
                config.enable_stream(RS2_STREAM_INFRARED, 0, _stream_w, _stream_h, RS2_FORMAT_RGB8);
            else if (_camera_name == "Intel RealSense D430 with Tracking Module")
                config.enable_stream(RS2_STREAM_INFRARED, 1, _stream_w, _stream_h, RS2_FORMAT_Y8);
            else
                config.enable_stream(RS2_STREAM_COLOR, 0, _stream_w, _stream_h, RS2_FORMAT_RGB8);
            
            return config;
        }
        
        /** \return width of the box_measure frame. */
        int stream_w() const { return _stream_w; }
        /** \return height of the box_measure frame. */
        int stream_h() const { return _stream_h; }
        
        /**
         * Process a new frameset which aligns the stream definitions in get_camera_config().
         * The output frameset is a shared copy of the input image pair plus additional stream enabled by configure().
         *
         * \param[in] frames new frameset to be processed by box_measure, must follow get_camera_config() definitions.
         * \return output frameset from box_measure.
         */
        box_frameset process(frameset frames)
        {
            _block->invoke(frames);
            frame f;
            _queue.poll_for_frame(&f);
            return box_frameset(f);
        }
        
        /**
         * Get boxes.
         * \return vector of boxes from box_measure.
         */
        box_vector get_boxes()
        {
            rs2_error* e = nullptr;
            int nbox = rs2_box_measure_get_boxes(_box_measure, _box, &e);
            error::handle(e);
            
            return box_vector(_box, _box + nbox);
        }
        
        /**
         * Get an ideal depth image of box.
         *
         * \param[in] b  box to be raycasted.
         * \param[in] fs box frameset given by box_measure as output buffer.
         * \return box depth image.
         */
        box_frameset raycast_box_onto_frame(const box& b, box_frameset fs)
        {
            if (!_boxcast) { _boxcast.reset(new box_raycast(this->_box_measure)); }
            frame dst = _boxcast->proc(b,fs);
            return box_frameset(dst);
        }
        
        /** Get image data of a Intel(c) RealSense(TM) icon. */
        static const char* get_icon(int& w, int &h, rs2_format& format)
        {
            rs2_error* e = nullptr;
            auto icon = rs2_measure_get_realsense_icon(&w, &h, &format, &e);
            error::handle(e);
            return icon;
        }
        
    protected:
        
        float try_get_depth_scale(device dev) try
        {
            // get the device name
            _camera_name = dev.get_info(RS2_CAMERA_INFO_NAME);
            
            // assign stream image sizes
            if (_camera_name == "Intel RealSense 410" ||
                _camera_name == "Intel RealSense D410") { _stream_w = 640; _stream_h = 480; }
            else if (_camera_name == "Intel RealSense 415" ||
                     _camera_name == "Intel RealSense D415") { _stream_w = 640; _stream_h = 480; _use_color = true; }
            else if (_camera_name == "Intel RealSense 435" ||
                     _camera_name == "Intel RealSense D435") { _stream_w = 640; _stream_h = 480; _use_color = true; }
            else if (_camera_name == "Intel RealSense SR300") { _stream_w = 640; _stream_h = 480; _depth_unit = 0.000125f; }
            else if (_camera_name == "Intel RealSense D430 with Tracking Module") { _stream_w = 640; _stream_h = 480; }
            else {  _stream_w = 640; _stream_h = 480; /**throw std::runtime_error(_camera_name + " not supported by Box SDK!");*/ }
            
            if (_depth_unit != 0.0f) return _depth_unit;
            
            // Go over the device's sensors
            for (sensor& sensor : dev.query_sensors())
            {
                // Check if the sensor if a depth sensor
                if (depth_sensor dpt = sensor.as<depth_sensor>())
                {
                    return _depth_unit = std::fmaxf(0.001f, dpt.get_depth_scale());
                }
            }
            return 0.001f;
        }
        catch (...) { return 0.001f; }
        
        /**
         * Set better depth generation parameters for the depth camera.
         * \param[in] mode : "Custom","Default","Hand","High Accuracy" or "High Density"
         */
        std::string set_sensor_options(device dev, const std::string& mode = "High Accuracy") const try
        {
            if (dev.is<rs400::advanced_mode>()){
                auto advanced_mode_dev = dev.as<rs400::advanced_mode>();
                if (!advanced_mode_dev.is_enabled()){ //advanced mode not enabled
                    advanced_mode_dev.toggle_advanced_mode(true); // enable advanced mode
                }
            }
            
            for (auto& s : dev.query_sensors())
                if (auto ds = s.as<depth_sensor>())
                {
                    auto range = ds.get_option_range(RS2_OPTION_VISUAL_PRESET);
                    for (auto i = range.min; i < range.max; i += range.step)
                        if (std::string(ds.get_option_value_description(RS2_OPTION_VISUAL_PRESET, i)) == mode)
                            ds.set_option(RS2_OPTION_VISUAL_PRESET, i);
                            }
            return mode;
        }
        catch (...) { return "Unavailable"; }
        
    private:
        
        std::shared_ptr<processing_block> _block;
        frame_queue _queue;
        
        rs2_box_measure* _box_measure;
        rs2_measure_box _box[RS2_MEASURE_BOX_MAXCOUNT];
        std::string _camera_name;
        int _stream_w, _stream_h;
        float _depth_unit = 0.0f;
        bool _use_color = false;
        
        std::unique_ptr<box_raycast> _boxcast;
    };
}
#endif //__cplusplus

#endif /* rs_box_sdk_hpp */

