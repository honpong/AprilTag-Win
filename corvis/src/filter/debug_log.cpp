#include "debug_log.h"

#include "spdlog/sinks/sink.h"
#include "rc_tracker.h"

class threaded_callback_sink : public spdlog::sinks::base_sink < std::mutex >
{
private:
    void * handle;
    int log_level;
    std::function<void (void *, int, const char *, size_t)> callback;

public:
    threaded_callback_sink(std::function<void (void *, int, const char *, size_t)> log, int max_log_level, void * debug_handle) :
        handle(debug_handle), log_level(max_log_level), callback(log) {};

protected:
    void _sink_it(const spdlog::details::log_msg& msg) override
    {
        if(!callback) return;

        int rc_msg_level = rc_MESSAGE_TRACE;
        if(msg.level == spdlog::level::err) rc_msg_level = rc_MESSAGE_ERROR;
        if(msg.level == spdlog::level::warn) rc_msg_level = rc_MESSAGE_WARN;
        if(msg.level == spdlog::level::info) rc_msg_level = rc_MESSAGE_INFO;
        if(rc_msg_level <= log_level)
            callback(handle, rc_msg_level, msg.formatted.c_str(), msg.formatted.size());
    }

    void flush() override
    {
    }
};


std::mutex set_mutex;

std::shared_ptr<spdlog::logger> debug_log = std::make_shared<spdlog::logger>("sensor_fusion", std::make_shared<threaded_callback_sink>(nullptr, rc_MESSAGE_NONE, nullptr));
void debug_log_set(std::function<void (void *, int, const char *, size_t)> log, int max_log_level, void * handle)
{
    std::lock_guard<std::mutex> lock(set_mutex);
    if(debug_log)
        spdlog::drop("rc_tracker");

    auto custom_sink = std::make_shared<threaded_callback_sink>(log, max_log_level, handle);
    debug_log = std::make_shared<spdlog::logger>("rc_tracker", custom_sink);

    spdlog::register_logger(debug_log);
}
