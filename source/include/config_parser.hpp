#ifndef CONFIG_PARSER_HPP
#define CONFIG_PARSER_HPP

#include <string>
#include <unordered_map>
#include <cstdint>
#include "ofs_types.hpp"

namespace ofs
{

/**
 * Configuration structure holding parsed .uconf file data
 * Organized by sections: filesystem, security, server
 */
struct Config
{
    // ========== [filesystem] section ==========
    uint64_t total_size;           // Total filesystem size in bytes
    uint64_t header_size;          // Header size (should be 512)
    uint64_t block_size;           // Block size in bytes (must be power of 2)
    uint32_t max_files;            // Maximum number of files
    uint32_t max_filename_length;  // Maximum filename length (excluding null terminator)

    // ========== [security] section ==========
    uint32_t max_users;            // Maximum number of users
    std::string admin_username;    // Default admin username
    std::string admin_password;    // Default admin password
    bool require_auth;             // Whether authentication is required
    std::string private_key;       // Private key for operations

    // ========== [server] section ==========
    uint16_t port;                 // Server listening port
    uint32_t max_connections;      // Maximum simultaneous connections
    uint32_t queue_timeout;        // Queue timeout in seconds

    // Constructor with default values
    Config();
};

/**
 * ConfigParser class for parsing and validating .uconf files
 * Singleton pattern to ensure single instance across the application
 */
class ConfigParser
{
public:
    /**
     * Get singleton instance of ConfigParser
     */
    static ConfigParser& getInstance();

    /**
     * Delete copy constructor and assignment operator (Singleton)
     */
    ConfigParser(const ConfigParser&) = delete;
    ConfigParser& operator=(const ConfigParser&) = delete;

    /**
     * Parse configuration from file
     * @param config_path Path to .uconf file
     * @param config Output configuration structure
     * @return OFSErrorCodes::SUCCESS if parsing succeeded, error code otherwise
     */
    OFSErrorCodes parseConfig(const std::string& config_path, Config& config);

    /**
     * Get default configuration values
     * Used when config file is missing or parsing fails
     * @return Configuration with default values
     */
    Config getDefaultConfig() const;

    /**
     * Validate configuration values
     * Checks constraints like:
     * - block_size is power of 2
     * - block_size <= total_size
     * - max_users > 0
     * - port is within valid range (1-65535)
     * @param config Configuration to validate
     * @return OFSErrorCodes::SUCCESS if valid, error code otherwise
     */
    OFSErrorCodes validateConfig(const Config& config);

private:
    ConfigParser() = default;  // Private constructor for Singleton

    /**
     * Parse a single section of the configuration file
     * @param section_name Name of section (e.g., "filesystem")
     * @param line Current line from file
     * @param config Configuration structure to populate
     * @return true if line was part of section, false otherwise
     */
    bool parseSection(const std::string& section_name, 
                     const std::string& line, 
                     Config& config);

    /**
     * Trim whitespace from string
     */
    std::string trim(const std::string& str);

    /**
     * Check if a number is a power of 2
     */
    bool isPowerOfTwo(uint64_t value) const;

    /**
     * Parse a single key-value pair
     */
    bool parseKeyValue(const std::string& line, 
                      std::string& key, 
                      std::string& value);
};

}  // namespace ofs

#endif  // CONFIG_PARSER_HPP
