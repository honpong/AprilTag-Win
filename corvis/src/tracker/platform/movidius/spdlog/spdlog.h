#pragma once

#include <memory>
#include <string>

#include "sinks/sink.h"

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

        logger(const std::string& logger_name, sink_ptr single_sink) {};

        void set_level(level::level_enum) {};
        void set_pattern(const char *) {};


    template <typename... Args> void trace(const char*, const Args&...) {};
    template <typename... Args> void debug(const char*, const Args&...) {};
    template <typename... Args> void info(const char*, const Args&...) {};
    template <typename... Args> void warn(const char*, const Args&...) {};
    template <typename... Args> void error(const char*, const Args&...) {};
    template <typename... Args> void critical(const char*, const Args&...) {};

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
