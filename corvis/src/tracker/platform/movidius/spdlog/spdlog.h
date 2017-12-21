#pragma once

#include <memory>
#include <string>
#include <spdlog/fmt/fmt.h>

#include "sinks/sink.h"

#ifdef SLAM_LOG
    #include "logger/logger.h"
    #define MODULE_ID HMD_TRACKING
    #define SLAM_LOG_EVENT(level, level_str, message)                        \
    {                                                                   \
        uint64_t now = TimestampNs();                                   \
        if(level == LOG_ERR || level == WARNING)                        \
            log_event((level), (__LINE__), MODULE_ID, FREE, message, now);  \
        if(JTAG_LOG_LEVEL >= level) {                                   \
            printf("[%s] %s\n", level_str, message);                    \
        }                                                               \
    }
#else
    #define JTAG_LOG_LEVEL DEBUG

    //List of verbosity levels
    enum log_level {
            NO_LOG   = 0x0000,
            LOG_ERR  = 0x0001,
            INFO     = 0x0002,
            WARNING  = 0x0003,
            DEBUG    = 0x0004,
            VERBOSE  = 0x0005,
            TRACE    = 0x0006,
            MAX_LOG_LEVEL
    };

    #define SLAM_LOG_EVENT(level, level_str, message)        \
    {                                                   \
        if(JTAG_LOG_LEVEL >= level) {                   \
            printf("[%s] %s\n", level_str, message);    \
        }                                               \
    }
#endif
#define SLAM_LOG_ERR(message) SLAM_LOG_EVENT(LOG_ERR, "ERR", message)
#define SLAM_LOG_INF(message) SLAM_LOG_EVENT(INFO,    "INF", message)
#define SLAM_LOG_WRN(message) SLAM_LOG_EVENT(WARNING, "WRN", message)
#define SLAM_LOG_DBG(message) SLAM_LOG_EVENT(DEBUG,   "DBG", message)
#define SLAM_LOG_VRB(message) SLAM_LOG_EVENT(VERBOSE, "VRB", message)
#define SLAM_LOG_TRC(message) SLAM_LOG_EVENT(TRACE,   "TRC", message)

namespace spdlog {
    namespace level {

        typedef enum
        {
            trace = 0,
            debug = 1,
            info = 2,
            warn = 3,
            err = 4,
            critical = 5,
            off = 6
        } level_enum;

        __attribute__((unused))
        static const char* level_names[] { "trace", "debug", "info",  "warning", "error", "critical", "off" };
    }

    namespace details
    {
        struct log_msg
        {
            level::level_enum level;
            std::string formatted;
        };
    }

    using sink_ptr = std::shared_ptr < sinks::sink >;

    class logger {

    public:

        logger(const std::string&, sink_ptr) {};

        void set_level(level::level_enum) {};
        void set_pattern(const char *) {};


    template <typename... Args> void trace(const char* fmt, const Args&... args) {
        SLAM_LOG_TRC(fmt::format(fmt, args...).c_str());
    }

    template <typename... Args> void debug(const char* fmt, const Args&... args) {
        SLAM_LOG_DBG(fmt::format(fmt, args...).c_str());
    }

    template <typename... Args> void info(const char* fmt, const Args&... args) {
        SLAM_LOG_INF(fmt::format(fmt, args...).c_str());
    }

    template <typename... Args> void warn(const char* fmt, const Args&... args) {
        SLAM_LOG_WRN(fmt::format(fmt, args...).c_str());
    }

    template <typename... Args> void error(const char* fmt, const Args&... args) {
        SLAM_LOG_ERR(fmt::format(fmt, args...).c_str());
    }

    template <typename... Args> void critical(const char* fmt, const Args&... args) {
       SLAM_LOG_ERR(fmt::format(fmt, args...).c_str());
    }

    template <typename T> void trace(const T&) {};
    template <typename T> void debug(const T&) {};
    template <typename T> void info(const T&) {};
    template <typename T> void warn(const T&) {};
    template <typename T> void error(const T&) {};
    template <typename T> void critical(const T&) {};

    };

    namespace details {
        class null_mutex {
        };
    }

}

#include "sinks/null_sink.h"
#include "sinks/base_sink.h"
