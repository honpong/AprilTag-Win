#pragma once

#include <functional>
#include <ostream>

struct benchmark_result { struct { double reference, measured; } length_cm, path_length_cm; void * user_data; };

void benchmark_run(std::ostream &stream, const char *directory,
        std::function<bool (const char *file, struct benchmark_result &result)> measure_file,
        std::function<void (const char *file, struct benchmark_result &result)> measure_done);