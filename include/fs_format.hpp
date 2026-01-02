#ifndef FS_FORMAT_HPP
#define FS_FORMAT_HPP

#include "ofs_types.hpp"
#include <string>

int fs_format(const std::string& omni_path, const std::string& config_path);

int fs_validate(const std::string& omni_path);

uint64_t fs_calculate_size(const std::string& config_path);

#endif
