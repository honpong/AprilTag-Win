// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#ifndef LIBREALSENSE_RS_HPP
#define LIBREALSENSE_RS_HPP

#include "rs.h"
#include "rscore.hpp"

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <exception>
#include <ostream>
#include <iostream>

namespace rs
{
    class error : public std::runtime_error
    {
        std::string function, args;
    public:
        explicit error(rs_error * err) : runtime_error(rs_get_error_message(err))
        {
            function = (nullptr != rs_get_failed_function(err)) ? rs_get_failed_function(err) : std::string();
            args = (nullptr != rs_get_failed_args(err)) ? rs_get_failed_args(err) : std::string();
            rs_free_error(err);
        }
        const std::string & get_failed_function() const
        {
            return function;
        }
        const std::string & get_failed_args() const
        {
            return args;
        }
        static void handle(rs_error * e);
    };

    class recoverable_error : public error
    {
     public:
        recoverable_error(rs_error * e) noexcept
            : error(e)
        {}
    };

    class unrecoverable_error : public error
    {
    public:
       unrecoverable_error(rs_error * e) noexcept
           : error(e)
       {}
    };

    class camera_disconnected_error : public unrecoverable_error
    {
    public:
        camera_disconnected_error(rs_error * e) noexcept
            : unrecoverable_error(e)
        {}
    };

    class backend_error : public unrecoverable_error
    {
    public:
        backend_error(rs_error * e) noexcept
            : unrecoverable_error(e)
        {}
    };

    class invalid_value_error : public recoverable_error
    {
    public:
        invalid_value_error(rs_error * e) noexcept
            : recoverable_error(e)
        {}
    };

    class wrong_api_call_sequence_error : public recoverable_error
    {
    public:
        wrong_api_call_sequence_error(rs_error * e) noexcept
            : recoverable_error(e)
        {}
    };

    class not_implemented_error : public recoverable_error
    {
    public:
        not_implemented_error(rs_error * e) noexcept
            : recoverable_error(e)
        {}
    };

    inline void error::handle(rs_error * e)
    {
        if (e)
        {
            auto h = rs_get_librealsense_exception_type(e);
            switch (h) {
            case RS_LIBREALSENSE_EXCEPTION_TYPE_CAMERA_DISCONNECTED:
                throw camera_disconnected_error(e);
                break;
            case RS_LIBREALSENSE_EXCEPTION_TYPE_BACKEND:
                throw backend_error(e);
                break;
            case RS_LIBREALSENSE_EXCEPTION_TYPE_INVALID_VALUE:
                throw invalid_value_error(e);
                break;
            case RS_LIBREALSENSE_EXCEPTION_TYPE_WRONG_API_CALL_SEQUENCE:
                throw wrong_api_call_sequence_error(e);
                break;
            case RS_LIBREALSENSE_EXCEPTION_TYPE_NOT_IMPLEMENTED:
                throw not_implemented_error(e);
                break;
            default:
                throw error(e);
                break;
            }
        }
    }

    class context;
    class device;

    struct stream_profile
    {
        rs_stream stream;
        int width;
        int height;
        int fps;
        rs_format format;

        bool match(const stream_profile& other) const
        {
            if (stream != rs_stream::RS_STREAM_ANY && other.stream != rs_stream::RS_STREAM_ANY && (stream != other.stream))
                return false;
            if (format != rs_format::RS_FORMAT_ANY && other.format != rs_format::RS_FORMAT_ANY && (format != other.format))
                return false;
            if (fps != 0 && other.fps != 0 && (fps != other.fps))
                return false;
            if (width != 0 && other.width != 0 && (width != other.width))
                return false;
            if (height != 0 && other.height != 0 && (height != other.height))
                return false;
            return true;
        }

        bool contradicts(const std::vector<stream_profile>& requests) const
        {
            for (auto request : requests)
            {
                if (fps != 0 && request.fps != 0 && (fps != request.fps))
                    return true;
                if (width != 0 && request.width != 0 && (width != request.width))
                    return true;
                if (height != 0 && request.height != 0 && (height != request.height))
                    return true;
            }
            return false;
        }

        bool has_wildcards() const
        {
            return (fps == 0 || width == 0 || height == 0 || stream == rs_stream::RS_STREAM_ANY || format == rs_format::RS_FORMAT_ANY);
        }
    };

    inline bool operator==(const stream_profile& a, const stream_profile& b)
    {
        return (a.width == b.width) && (a.height == b.height) && (a.fps == b.fps) && (a.format == b.format) && (a.stream == b.stream);
    }

    class frame
    {
        rs_frame * frame_ref;
        frame(const frame &) = delete;
    public:
        friend class frame_queue;

        frame() : frame_ref(nullptr) {}
        frame(rs_frame * frame_ref) : frame_ref(frame_ref) {}
        frame(frame&& other) : frame_ref(other.frame_ref) { other.frame_ref = nullptr; }
        frame& operator=(frame other)
        {
            swap(other);
            return *this;
        }
        void swap(frame& other)
        {
            std::swap(frame_ref, other.frame_ref);
        }

        /**
        * relases the frame handle
        */
        ~frame()
        {
            if (frame_ref)
            {
                rs_release_frame(frame_ref);
            }
        }

        operator bool() const { return frame_ref != nullptr; }

        /**
        * retrieve the time at which the frame was captured
        * \return            the timestamp of the frame, in milliseconds since the device was started
        */
        double get_timestamp() const
        {
            rs_error * e = nullptr;
            auto r = rs_get_frame_timestamp(frame_ref, &e);
            error::handle(e);
            return r;
        }

        /* retrieve the timestamp domain
        * \return            timestamp domain (clock name) for timestamp values
        */
        rs_timestamp_domain get_frame_timestamp_domain() const
        {
            rs_error * e = nullptr;
            auto r = rs_get_frame_timestamp_domain(frame_ref, &e);
            error::handle(e);
            return r;
        }

        /* retrieve the current value of a single frame_metadata
        * \param[in] frame_metadata  the frame_metadata whose value should be retrieved
        * \return            the value of the frame_metadata
        */
        double get_frame_metadata(rs_frame_metadata frame_metadata) const
        {
            rs_error * e = nullptr;
            auto r = rs_get_frame_metadata(frame_ref, frame_metadata, &e);
            error::handle(e);
            return r;
        }

        /* determine if the device allows a specific metadata to be queried
        * \param[in] frame_metadata  the frame_metadata to check for support
        * \return            true if the frame_metadata can be queried
        */
        bool supports_frame_metadata(rs_frame_metadata frame_metadata) const
        {
            rs_error * e = nullptr;
            auto r = rs_supports_frame_metadata(frame_ref, frame_metadata, &e);
            error::handle(e);
            return r != 0;
        }

        /**
        * retrive frame number (from frame handle)
        * \return               the frame nubmer of the frame, in milliseconds since the device was started
        */
        unsigned long long get_frame_number() const
        {
            rs_error * e = nullptr;
            auto r = rs_get_frame_number(frame_ref, &e);
            error::handle(e);
            return r;
        }

        /**
        * retrive data from frame handle
        * \return               the pointer to the start of the frame data
        */
        const void * get_data() const
        {
            rs_error * e = nullptr;
            auto r = rs_get_frame_data(frame_ref, &e);
            error::handle(e);
            return r;
        }

        /**
        * returns image width in pixels
        * \return        frame width in pixels
        */
        int get_width() const
        {
            rs_error * e = nullptr;
            auto r = rs_get_frame_width(frame_ref, &e);
            error::handle(e);
            return r;
        }

        /**
        * returns image height in pixels
        * \return        frame height in pixels
        */
        int get_height() const
        {
            rs_error * e = nullptr;
            auto r = rs_get_frame_height(frame_ref, &e);
            error::handle(e);
            return r;
        }

        /**
        * retrive frame stride, meaning the actual line width in memory in bytes (not the logical image width)
        * \return            stride in bytes
        */
        int get_stride_in_bytes() const
        {
            rs_error * e = nullptr;
            auto r = rs_get_frame_stride_in_bytes(frame_ref, &e);
            error::handle(e);
            return r;
        }

        /**
        * retrieve bits per pixel
        * \return            number of bits per one pixel
        */
        int get_bits_per_pixel() const
        {
            rs_error * e = nullptr;
            auto r = rs_get_frame_bits_per_pixel(frame_ref, &e);
            error::handle(e);
            return r;
        }

        int get_bytes_per_pixel() const { return get_bits_per_pixel() / 8; }

        /**
        * retrive pixel format of the frame
        * \return               pixel format as described in rs_format enum
        */
        rs_format get_format() const
        {
            rs_error * e = nullptr;
            auto r = rs_get_frame_format(frame_ref, &e);
            error::handle(e);
            return r;
        }

        /**
        * retrive the origin stream type that produced the frame
        * \return               stream type of the frame
        */
        rs_stream get_stream_type() const
        {
            rs_error * e = nullptr;
            auto s = rs_get_frame_stream_type(frame_ref, &e);
            error::handle(e);
            return s;
        }

        /**
        * create additional reference to a frame without duplicating frame data
        * \param[out] result     new frame reference, release by destructor
        * \return                true if cloning was successful
        */
        bool try_clone_ref(frame* result) const
        {
            rs_error * e = nullptr;
            auto s = rs_clone_frame_ref(frame_ref, &e);
            error::handle(e);
            if (!s) return false;
            *result = frame(s);
            return true;
        }
    };

    template<class T>
    class frame_callback : public rs_frame_callback
    {
        T on_frame_function;
    public:
        explicit frame_callback(T on_frame) : on_frame_function(on_frame) {}

        void on_frame(rs_frame * fref) override
        {
            on_frame_function({ fref });
        }

        void release() override { delete this; }
    };

    struct option_range
    {
        float min;
        float max;
        float def;
        float step;
    };

    class advanced
    {
    public:
        explicit advanced(std::shared_ptr<rs_device> dev)
            : _dev(dev) {}

        std::vector<uint8_t> send_and_receive_raw_data(const std::vector<uint8_t>& input) const
        {
            std::vector<uint8_t> results;

            rs_error* e = nullptr;
            std::shared_ptr<rs_raw_data_buffer> list(
                rs_send_and_receive_raw_data(_dev.get(), (void*)input.data(), (uint32_t)input.size(), &e),
                rs_delete_raw_data);
            error::handle(e);

            auto size = rs_get_raw_data_size(list.get(), &e);
            error::handle(e);

            auto start = rs_get_raw_data(list.get(), &e);

            results.insert(results.begin(), start, start + size);

            return results;
        }

    private:
        std::shared_ptr<rs_device> _dev;
    };

    class device
    {
    public:
        /**
        * open subdevice for exclusive access, by commiting to a configuration
        * \param[in] profile    configuration commited by the device
        */
        void open(const stream_profile& profile) const
        {
            rs_error* e = nullptr;
            rs_open(_dev.get(),
                profile.stream,
                profile.width,
                profile.height,
                profile.fps,
                profile.format,
                &e);
            error::handle(e);
        }

        /**
        * open subdevice for exclusive access, by commiting to composite configuration, specifying one or more stream profiles
        * this method should be used for interdendent streams, such as depth and infrared, that have to be configured together
        * \param[in] profiles   vector of configurations to be commited by the device
        */
        void open(const std::vector<stream_profile>& profiles) const
        {
            rs_error* e = nullptr;

            std::vector<rs_format> formats;
            std::vector<int> widths;
            std::vector<int> heights;
            std::vector<int> fpss;
            std::vector<rs_stream> streams;
            for (auto& p : profiles)
            {
                formats.push_back(p.format);
                widths.push_back(p.width);
                heights.push_back(p.height);
                fpss.push_back(p.fps);
                streams.push_back(p.stream);
            }

            rs_open_multiple(_dev.get(),
                streams.data(),
                widths.data(),
                heights.data(),
                fpss.data(),
                formats.data(),
                static_cast<int>(profiles.size()),
                &e);
            error::handle(e);
        }

        /**
        * close subdevice for exclusive access
        * this method should be used for releasing device resource
        */
        void close() const
        {
            rs_error* e = nullptr;
            rs_close(_dev.get(), &e);
            error::handle(e);
        }

        /**
        * start streaming
        * \param[in] callback   stream callback
        */
        template<class T>
        void start(T callback) const
        {
            rs_error * e = nullptr;
            rs_start_cpp(_dev.get(), new frame_callback<T>(std::move(callback)), &e);
            error::handle(e);
        }

        /**
        * stop streaming
        */
        void stop() const
        {
            rs_error * e = nullptr;
            rs_stop(_dev.get(), &e);
            error::handle(e);
        }

        /**
        * read option value from the device
        * \param[in] option   option id to be queried
        * \return value of the option
        */
        float get_option(rs_option option) const
        {
            rs_error* e = nullptr;
            auto res = rs_get_option(_dev.get(), option, &e);
            error::handle(e);
            return res;
        }

        /**
        * retrieve the available range of values of a supported option
        * \return option  range containing minimum and maximum values, step and default value
        */
        option_range get_option_range(rs_option option) const
        {
            option_range result;
            rs_error* e = nullptr;
            rs_get_option_range(_dev.get(), option,
                &result.min, &result.max, &result.step, &result.def, &e);
            error::handle(e);
            return result;
        }

        /**
        * write new value to device option
        * \param[in] option     option id to be queried
        * \param[in] value      new value for the option
        */
        void set_option(rs_option option, float value) const
        {
            rs_error* e = nullptr;
            rs_set_option(_dev.get(), option, value, &e);
            error::handle(e);
        }

        /**
        * check if particular option is supported by a subdevice
        * \param[in] option     option id to be checked
        * \return true if option is supported
        */
        bool supports(rs_option option) const
        {
            rs_error* e = nullptr;
            auto res = rs_supports_option(_dev.get(), option, &e);
            error::handle(e);
            return res > 0;
        }

        /**
        * get option description
        * \param[in] option     option id to be checked
        * \return human-readable option description
        */
        const char* get_option_description(rs_option option) const
        {
            rs_error* e = nullptr;
            auto res = rs_get_option_description(_dev.get(), option, &e);
            error::handle(e);
            return res;
        }

        /**
        * get option value description (in case specific option value hold special meaning)
        * \param[in] option     option id to be checked
        * \param[in] value      value of the option
        * \return human-readable description of a specific value of an option or null if no special meaning
        */
        const char* get_option_value_description(rs_option option, float val) const
        {
            rs_error* e = nullptr;
            auto res = rs_get_option_value_description(_dev.get(), option, val, &e);
            error::handle(e);
            return res;
        }

        /**
        * check if physical subdevice is supported
        * \return   list of stream profiles that given subdevice can provide, should be released by rs_delete_profiles_list
        */
        std::vector<stream_profile> get_stream_modes() const
        {
            std::vector<stream_profile> results;

            rs_error* e = nullptr;
            std::shared_ptr<rs_stream_modes_list> list(
                rs_get_stream_modes(_dev.get(), &e),
                rs_delete_modes_list);
            error::handle(e);

            auto size = rs_get_modes_count(list.get(), &e);
            error::handle(e);

            for (auto i = 0; i < size; i++)
            {
                stream_profile profile;
                rs_get_stream_mode(list.get(), i,
                    &profile.stream,
                    &profile.width,
                    &profile.height,
                    &profile.fps,
                    &profile.format,
                    &e);
                error::handle(e);
                results.push_back(profile);
            }

            return results;
        }

        /*
        * retrive stream intrinsics
        * \param[in] profile the stream profile to calculate the intrinsics for
        * \return intrinsics object
        */
        rs_intrinsics get_intrinsics(stream_profile profile) const
        {
            rs_error* e = nullptr;
            rs_intrinsics intrinsics;
            rs_get_stream_intrinsics(_dev.get(),
                profile.stream,
                profile.width,
                profile.height,
                profile.fps,
                profile.format, &intrinsics, &e);
            error::handle(e);
            return intrinsics;
        }

        /**
        * check if specific camera info is supported
        * \param[in] info    the parameter to check for support
        * \return                true if the parameter both exist and well-defined for the specific device
        */
        bool supports(rs_camera_info info) const
        {
            rs_error* e = nullptr;
            auto is_supported = rs_supports_camera_info(_dev.get(), info, &e);
            error::handle(e);
            return is_supported > 0;
        }

        /**
        * retrieve camera specific information, like versions of various internal componnents
        * \param[in] info     camera info type to retrieve
        * \return             the requested camera info string, in a format specific to the device model
        */
        const char* get_camera_info(rs_camera_info info) const
        {
            rs_error* e = nullptr;
            auto result = rs_get_camera_info(_dev.get(), info, &e);
            error::handle(e);
            return result;
        }

        /**
        * returns the list of adjacent devices, sharing the same physical parent composite device
        * \return            the list of adjacent devices
        */
        std::vector<device> get_adjacent_devices() const
        {
            rs_error* e = nullptr;
            std::shared_ptr<rs_device_list> list(
                rs_query_adjacent_devices(_dev.get(), &e),
                rs_delete_device_list);
            error::handle(e);

            auto size = rs_get_device_count(list.get(), &e);
            error::handle(e);

            std::vector<device> results;
            for (auto i = 0; i < size; i++)
            {
                std::shared_ptr<rs_device> dev(
                    rs_create_device(list.get(), i, &e),
                    rs_delete_device);
                error::handle(e);

                device rs_dev(dev);
                results.push_back(rs_dev);
            }

            return results;
        }

        rs_extrinsics get_extrinsics_to(const device& to_device) const
        {
            rs_error* e = nullptr;
            rs_extrinsics extrin;
            rs_get_extrinsics(_dev.get(), to_device._dev.get(), &extrin, &e);
            error::handle(e);
            return extrin;
        }

        /**
        * retrieve mapping between the units of the depth image and meters
        * \return            depth in meters corresponding to a depth value of 1
        */
        float get_depth_scale() const
        {
            rs_error* e = nullptr;
            auto result = rs_get_device_depth_scale(_dev.get(), &e);
            error::handle(e);
            return result;
        }
        advanced& debug() { return _debug; }

        device() : _dev(nullptr), _debug(nullptr) {}
    private:
        friend context;

        explicit device(std::shared_ptr<rs_device> dev)
            : _dev(dev), _debug(dev)
        {
        }

        std::shared_ptr<rs_device> _dev;
        std::shared_ptr<rs_context> _context;
        advanced _debug;
    };

    inline bool operator==(const device& a, const device& b)
    {
        return (std::string(a.get_camera_info(RS_CAMERA_INFO_DEVICE_NAME)) == b.get_camera_info(RS_CAMERA_INFO_DEVICE_NAME)) &&
               (std::string(a.get_camera_info(RS_CAMERA_INFO_MODULE_NAME)) == b.get_camera_info(RS_CAMERA_INFO_MODULE_NAME)) &&
               (std::string(a.get_camera_info(RS_CAMERA_INFO_DEVICE_SERIAL_NUMBER)) == b.get_camera_info(RS_CAMERA_INFO_DEVICE_SERIAL_NUMBER));
    }

    inline bool operator!=(const device& a, const device& b)
    {
        return !(a == b);
    }

    /**
    * default librealsense context class
    * includes realsense API version as provided by RS_API_VERSION macro
    */
    class context
    {
    public:
        context()
        {
            rs_error* e = nullptr;
            _context = std::shared_ptr<rs_context>(
                rs_create_context(RS_API_VERSION, &e),
                rs_delete_context);
            error::handle(e);
        }

        /**
        * create a static snapshot of all connected devices at the time of the call
        * \return            the list of devices connected devices at the time of the call
        */
        std::vector<device> query_devices() const
        {
            rs_error* e = nullptr;
            std::shared_ptr<rs_device_list> list(
                rs_query_devices(_context.get(), &e),
                rs_delete_device_list);
            error::handle(e);

            auto size = rs_get_device_count(list.get(), &e);
            error::handle(e);

            std::vector<device> results;
            for (auto i = 0; i < size; i++)
            {
                std::shared_ptr<rs_device> dev(
                    rs_create_device(list.get(), i, &e),
                    rs_delete_device);
                error::handle(e);

                device rs_dev(dev);
                results.push_back(rs_dev);
            }

            return results;
        }

        rs_extrinsics get_extrinsics(const device& from_device, const device& to_device) const
        {
            rs_error* e = nullptr;
            rs_extrinsics extrin;
            rs_get_extrinsics(from_device._dev.get(), to_device._dev.get(), &extrin, &e);
            error::handle(e);
            return extrin;
        }

    protected:
        std::shared_ptr<rs_context> _context;
    };

    class recording_context : public context
    {
    public:
        /**
        * create librealsense context that will try to record all operations over librealsense into a file
        * \param[in] filename string representing the name of the file to record
        */
        recording_context(const std::string& filename,
                          const std::string& section = "",
                          rs_recording_mode mode = RS_RECORDING_MODE_BEST_QUALITY)
        {
            rs_error* e = nullptr;
            _context = std::shared_ptr<rs_context>(
                rs_create_recording_context(RS_API_VERSION, filename.c_str(), section.c_str(), mode, &e),
                rs_delete_context);
            error::handle(e);
        }

        recording_context() = delete;
    };

    class mock_context : public context
    {
    public:
        /**
        * create librealsense context that given a file will respond to calls exactly as the recording did
        * if the user calls a method that was either not called during recording or voilates causality of the recording error will be thrown
        * \param[in] filename string of the name of the file
        */
        mock_context(const std::string& filename,
                     const std::string& section = "")
        {
            rs_error* e = nullptr;
            _context = std::shared_ptr<rs_context>(
                rs_create_mock_context(RS_API_VERSION, filename.c_str(), section.c_str(), &e),
                rs_delete_context);
            error::handle(e);
        }

        mock_context() = delete;
    };

    class frame_queue
    {
    public:
        /**
        * create frame queue. frame queues are the simplest x-platform syncronization primitive provided by librealsense
        * to help developers who are not using async APIs
        * param[in] capacity size of the frame queue
        */
        explicit frame_queue(unsigned int capacity)
        {
            rs_error* e = nullptr;
            _queue = std::shared_ptr<rs_frame_queue>(
                rs_create_frame_queue(capacity, &e),
                rs_delete_frame_queue);
            error::handle(e);
        }

        frame_queue() : frame_queue(1) {}

        ~frame_queue()
        {
            flush();
        }

        /**
        * release all frames inside the queue
        */
        void flush() const
        {
            rs_error* e = nullptr;
            rs_flush_queue(_queue.get(), &e);
            error::handle(e);
        }

        /**
        * enqueue new frame into a queue
        * \param[in] f - frame handle to enqueue (this operation passed ownership to the queue)
        */
        void enqueue(frame f) const
        {
            rs_enqueue_frame(f.frame_ref, _queue.get()); // noexcept
            f.frame_ref = nullptr; // frame has been essentially moved from
        }

        /**
        * wait until new frame becomes available in the queue and dequeue it
        * \return frame handle to be released using rs_release_frame
        */
        frame wait_for_frame() const
        {
            rs_error* e = nullptr;
            auto frame_ref = rs_wait_for_frame(_queue.get(), &e);
            error::handle(e);
            return{ frame_ref };
        }

        /**
        * poll if a new frame is available and dequeue if it is
        * \param[out] f - frame handle
        * \return true if new frame was stored to f
        */
        bool poll_for_frame(frame* f) const
        {
            rs_error* e = nullptr;
            rs_frame* frame_ref = nullptr;
            auto res = rs_poll_for_frame(_queue.get(), &frame_ref, &e);
            error::handle(e);
            if (res) *f = { frame_ref };
            return res > 0;
        }

        void operator()(frame f) const
        {
            enqueue(std::move(f));
        }

    private:
        std::shared_ptr<rs_frame_queue> _queue;
    };

    inline void log_to_console(rs_log_severity min_severity)
    {
        rs_error * e = nullptr;
        rs_log_to_console(min_severity, &e);
        error::handle(e);
    }

    inline void log_to_file(rs_log_severity min_severity, const char * file_path = nullptr)
    {
        rs_error * e = nullptr;
        rs_log_to_file(min_severity, file_path, &e);
        error::handle(e);
    }
}

inline std::ostream & operator << (std::ostream & o, rs_stream stream) { return o << rs_stream_to_string(stream); }
inline std::ostream & operator << (std::ostream & o, rs_format format) { return o << rs_format_to_string(format); }
inline std::ostream & operator << (std::ostream & o, rs_distortion distortion) { return o << rs_distortion_to_string(distortion); }
inline std::ostream & operator << (std::ostream & o, rs_option option) { return o << rs_option_to_string(option); }

#endif // LIBREALSENSE_RS_HPP
