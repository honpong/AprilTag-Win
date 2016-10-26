#pragma once

#include <functional>

void for_each_file(const char *file_or_dir, std::function<void (const char *file)> call);


