#include "path_resolver.hpp"
#include <algorithm>
#include <sstream>

int PathResolver::validate_path(const std::string& path) {
    // Check basic requirements
    if (path.empty() || path[0] != '/') {
        return static_cast<int>(OFSErrorCodes::ERROR_INVALID_PATH);
    }
    
    if (path.length() > MAX_PATH_LENGTH) {
        return static_cast<int>(OFSErrorCodes::ERROR_INVALID_PATH);
    }
    
    // Check for null bytes
    if (contains_null_bytes(path)) {
        return static_cast<int>(OFSErrorCodes::ERROR_INVALID_PATH);
    }
    
    // Check for path traversal attacks (..)
    if (contains_path_traversal(path)) {
        return static_cast<int>(OFSErrorCodes::ERROR_INVALID_PATH);
    }
    
    // Check for double slashes (except leading /)
    if (path.find("//") != std::string::npos) {
        return static_cast<int>(OFSErrorCodes::ERROR_INVALID_PATH);
    }
    
    return static_cast<int>(OFSErrorCodes::SUCCESS);
}

std::string PathResolver::normalize(const std::string& path) {
    if (path.empty() || path[0] != '/') {
        return "/";
    }
    
    std::vector<std::string> parts = split(path);
    std::string result = "/";
    
    for (const auto& part : parts) {
        if (!part.empty() && part != ".") {
            result += part + "/";
        }
    }
    
    // Remove trailing slash (except for root)
    if (result.length() > 1 && result.back() == '/') {
        result.pop_back();
    }
    
    return result;
}

std::vector<std::string> PathResolver::split(const std::string& path) {
    std::vector<std::string> parts;
    std::stringstream ss(path);
    std::string item;
    
    while (std::getline(ss, item, '/')) {
        if (!item.empty()) {
            parts.push_back(item);
        }
    }
    
    return parts;
}

std::string PathResolver::get_parent(const std::string& path) {
    if (is_root(path)) {
        return "/";
    }
    
    std::string normalized = normalize(path);
    size_t last_slash = normalized.find_last_of('/');
    
    if (last_slash == 0) {
        return "/";
    }
    
    return normalized.substr(0, last_slash);
}

std::string PathResolver::get_filename(const std::string& path) {
    if (is_root(path)) {
        return "";
    }
    
    std::string normalized = normalize(path);
    size_t last_slash = normalized.find_last_of('/');
    
    if (last_slash == std::string::npos) {
        return normalized;
    }
    
    return normalized.substr(last_slash + 1);
}

std::string PathResolver::get_directory(const std::string& path) {
    return get_parent(path);
}

bool PathResolver::is_root(const std::string& path) {
    std::string normalized = normalize(path);
    return normalized == "/";
}

bool PathResolver::is_valid_filename(const std::string& filename) {
    if (filename.empty() || filename.length() > MAX_FILENAME_LENGTH) {
        return false;
    }
    
    // Cannot start with dot
    if (filename[0] == '.') {
        return false;
    }
    
    // Must contain at least one alphanumeric character
    bool has_alnum = false;
    for (char c : filename) {
        if (std::isalnum(c)) {
            has_alnum = true;
            break;
        }
    }
    
    return has_alnum;
}

std::string PathResolver::combine(const std::string& dir, const std::string& file) {
    std::string normalized_dir = normalize(dir);
    
    if (is_root(normalized_dir)) {
        return "/" + file;
    }
    
    return normalized_dir + "/" + file;
}

bool PathResolver::contains_null_bytes(const std::string& path) {
    return path.find('\0') != std::string::npos;
}

bool PathResolver::contains_path_traversal(const std::string& path) {
    // Check for .. sequences
    if (path.find("..") != std::string::npos) {
        return true;
    }
    
    return false;
}
