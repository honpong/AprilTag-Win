#pragma once

#include "sink.h"

namespace spdlog {
    namespace sinks {
        template <class Mutex>
        class base_sink : public sink {
            virtual void _sink_it(const spdlog::details::log_msg &msg) {};
            virtual void flush() {};
        };
    }
}
