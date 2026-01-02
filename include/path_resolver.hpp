#ifndef PATH_RESOLVER_HPP
#define PATH_RESOLVER_HPP

#include <string>
#include <vector>
#include "ofs_types.hpp"

/**
 * Path resolver for OFS
 * - Validates and normalizes paths
 * - Splits paths into components
 * - Resolves relative/absolute paths
 * - Detects path traversal attacks
 */
class PathResolver {
public:
    /**
     * Validate a file path
     * Must start with /, no .. sequences, no double slashes
     * Returns: OFS_SUCCESS if valid, error code if invalid
     */
    static int validate_path(const std::string& path);
    
    /**
     * Normalize path: remove duplicate slashes, resolve ., etc.
     * Input:  "/dir1//dir2/./file.txt"
     * Output: "/dir1/dir2/file.txt"
     */
    static std::string normalize(const std::string& path);
    
    /**
     * Split path into components
     * Input:  "/dir1/dir2/file.txt"
     * Output: ["dir1", "dir2", "file.txt"]
     */
    static std::vector<std::string> split(const std::string& path);
    
    /**
     * Get parent directory path
     * Input:  "/dir1/dir2/file.txt"
     * Output: "/dir1/dir2"
     * Root:   "/" returns "/"
     */
    static std::string get_parent(const std::string& path);
    
    /**
     * Get filename from path
     * Input:  "/dir1/dir2/file.txt"
     * Output: "file.txt"
     */
    static std::string get_filename(const std::string& path);
    
    /**
     * Get directory from path
     * Input:  "/dir1/dir2/file.txt"
     * Output: "/dir1/dir2"
     */
    static std::string get_directory(const std::string& path);
    
    /**
     * Check if path points to root
     */
    static bool is_root(const std::string& path);
    
    /**
     * Check if path is valid filename (not directory)
     * Must have characters, not start with ., not too long
     */
    static bool is_valid_filename(const std::string& filename);
    
    /**
     * Combine directory path with filename
     * dir: "/dir1/dir2"
     * file: "file.txt"
     * Result: "/dir1/dir2/file.txt"
     */
    static std::string combine(const std::string& dir, const std::string& file);

private:
    static const int MAX_PATH_LENGTH = 512;
    static const int MAX_FILENAME_LENGTH = 256;
    
    static bool contains_null_bytes(const std::string& path);
    static bool contains_path_traversal(const std::string& path);
};

#endif
