#ifndef __DEBUG_LOG_H__
#define __DEBUG_LOG_H__

#include "spdlog/spdlog.h"
#include <functional>

extern std::shared_ptr<spdlog::logger> debug_log;
void debug_log_set(std::function<void (void *, int, const char *, size_t)> log, int max_log_level, void * handle);

#endif
