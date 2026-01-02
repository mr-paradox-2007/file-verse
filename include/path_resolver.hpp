#ifndef PATH_RESOLVER_HPP
#define PATH_RESOLVER_HPP

#include <string>
#include <vector>
#include "ofs_types.hpp"

class PathResolver {
public:
    static int validate_path(const std::string& path);
    
    static std::string normalize(const std::string& path);
    
    static std::vector<std::string> split(const std::string& path);
    
    static std::string get_parent(const std::string& path);
    
    static std::string get_filename(const std::string& path);
    
    static std::string get_directory(const std::string& path);
    
    static bool is_root(const std::string& path);
    
    static bool is_valid_filename(const std::string& filename);
    
    static std::string combine(const std::string& dir, const std::string& file);

private:
    static const int MAX_PATH_LENGTH = 512;
    static const int MAX_FILENAME_LENGTH = 256;
    
    static bool contains_null_bytes(const std::string& path);
    static bool contains_path_traversal(const std::string& path);
};

#endif
