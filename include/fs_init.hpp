#ifndef FS_INIT_HPP
#define FS_INIT_HPP

#include "ofs_types.hpp"
#include <string>

typedef void* OFS_Instance;

int fs_init(OFS_Instance* instance, const std::string& omni_path, 
            const std::string& config_path, const std::string& private_key);

int fs_shutdown(OFS_Instance instance);

OFS_Instance fs_get_current_instance();

#endif
