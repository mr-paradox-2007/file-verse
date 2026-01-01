#include "config_parser.hpp"
#include "logger.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <cmath>

namespace ofs
{

// ============================================================================
// CONFIG CONSTRUCTOR WITH DEFAULT VALUES
// ============================================================================

Config::Config()
    : total_size(104857600),           // 100MB default
      header_size(512),                // OMNIHeader size
      block_size(4096),                // 4KB blocks
      max_files(1000),
      max_filename_length(10),
      max_users(50),
      admin_username("admin"),
      admin_password("admin123"),
      require_auth(true),
      private_key("OFS_PRIVATE_KEY_123"),
      port(8080),
      max_connections(20),
      queue_timeout(30)
{
}

// ============================================================================
// CONFIGPARSER SINGLETON IMPLEMENTATION
// ============================================================================

ConfigParser& ConfigParser::getInstance()
{
    static ConfigParser instance;
    return instance;
}

// ============================================================================
// PUBLIC METHODS
// ============================================================================

OFSErrorCodes ConfigParser::parseConfig(const std::string& config_path, Config& config)
{
    LOG_INFO("CONFIG", 0, "Attempting to parse configuration from: " + config_path);

    std::ifstream config_file(config_path);
    if (!config_file.is_open())
    {
        LOG_WARN("CONFIG", 101,
                "Configuration file not found: " + config_path + 
                ". Using default values.");
        config = getDefaultConfig();
        return OFSErrorCodes::SUCCESS;
    }

    // Initialize with defaults first
    config = getDefaultConfig();

    std::string line;
    std::string current_section;
    int line_number = 0;

    while (std::getline(config_file, line))
    {
        line_number++;
        
        // Trim leading/trailing whitespace
        line = trim(line);

        // Skip empty lines and comments
        if (line.empty() || line[0] == '#' || line[0] == ';')
            continue;

        // Check for section headers [section_name]
        if (line[0] == '[' && line[line.length() - 1] == ']')
        {
            current_section = line.substr(1, line.length() - 2);
            current_section = trim(current_section);
            LOG_DEBUG("CONFIG", 0, "Found section: [" + current_section + "]");
            continue;
        }

        // Parse key-value pairs in current section
        if (!current_section.empty())
        {
            parseSection(current_section, line, config);
        }
    }

    config_file.close();

    // Validate the parsed configuration
    OFSErrorCodes validation_result = validateConfig(config);
    if (validation_result != OFSErrorCodes::SUCCESS)
    {
        LOG_ERROR("CONFIG", 102,
                "Configuration validation failed. Error code: " + 
                std::to_string(static_cast<int>(validation_result)));
        return validation_result;
    }

    LOG_INFO("CONFIG", 0, "Configuration successfully parsed and validated.");
    return OFSErrorCodes::SUCCESS;
}

Config ConfigParser::getDefaultConfig() const
{
    LOG_INFO("CONFIG", 0, "Loading default configuration values");
    return Config();
}

OFSErrorCodes ConfigParser::validateConfig(const Config& config)
{
    // ========== Filesystem validation ==========
    if (config.total_size == 0)
    {
        LOG_ERROR("CONFIG", 103,
                "total_size must be greater than 0");
        return OFSErrorCodes::ERROR_INVALID_CONFIG;
    }

    if (config.header_size != 512)
    {
        LOG_WARN("CONFIG", 104,
                "header_size is " + std::to_string(config.header_size) +
                ", expected 512. This may cause compatibility issues.");
    }

    if (!isPowerOfTwo(config.block_size))
    {
        LOG_ERROR("CONFIG", 105,
                "block_size must be a power of 2. Got: " +
                std::to_string(config.block_size));
        return OFSErrorCodes::ERROR_INVALID_CONFIG;
    }

    if (config.block_size < 512 || config.block_size > 1048576)
    {
        LOG_ERROR("CONFIG", 106,
                "block_size must be between 512 and 1048576 bytes. Got: " +
                std::to_string(config.block_size));
        return OFSErrorCodes::ERROR_INVALID_CONFIG;
    }

    // Calculate usable space: total_size - header - user_table
    // Assuming max 50 users, each user is ~256 bytes
    uint64_t user_table_size = config.max_users * 256;
    uint64_t usable_space = config.total_size - config.header_size - user_table_size;

    if (usable_space < config.block_size)
    {
        LOG_ERROR("CONFIG", 107,
                "Not enough space: total_size - header - user_table < block_size");
        return OFSErrorCodes::ERROR_INVALID_CONFIG;
    }

    if (config.max_files == 0)
    {
        LOG_ERROR("CONFIG", 108,
                "max_files must be greater than 0");
        return OFSErrorCodes::ERROR_INVALID_CONFIG;
    }

    if (config.max_filename_length == 0 || config.max_filename_length > 255)
    {
        LOG_ERROR("CONFIG", 109,
                "max_filename_length must be between 1 and 255");
        return OFSErrorCodes::ERROR_INVALID_CONFIG;
    }

    // ========== Security validation ==========
    if (config.max_users == 0 || config.max_users > 1000)
    {
        LOG_ERROR("CONFIG", 110,
                "max_users must be between 1 and 1000. Got: " +
                std::to_string(config.max_users));
        return OFSErrorCodes::ERROR_INVALID_CONFIG;
    }

    if (config.admin_username.empty())
    {
        LOG_WARN("CONFIG", 111,
                "admin_username is empty. Using default: admin");
    }

    if (config.admin_password.empty())
    {
        LOG_WARN("CONFIG", 112,
                "admin_password is empty. This is insecure!");
    }

    if (config.private_key.empty())
    {
        LOG_WARN("CONFIG", 113,
                "private_key is empty. All operations will be publicly accessible");
    }

    // ========== Server validation ==========
    if (config.port < 1 || config.port > 65535)
    {
        LOG_ERROR("CONFIG", 114,
                "port must be between 1 and 65535. Got: " +
                std::to_string(config.port));
        return OFSErrorCodes::ERROR_INVALID_CONFIG;
    }

    if (config.port < 1024)
    {
        LOG_WARN("CONFIG", 115,
                "port " + std::to_string(config.port) +
                " is reserved (< 1024). May require root privileges.");
    }

    if (config.max_connections == 0)
    {
        LOG_ERROR("CONFIG", 116,
                "max_connections must be greater than 0");
        return OFSErrorCodes::ERROR_INVALID_CONFIG;
    }

    if (config.queue_timeout == 0)
    {
        LOG_WARN("CONFIG", 117,
                "queue_timeout is 0. Queue operations will not timeout.");
    }

    LOG_INFO("CONFIG", 0, "Configuration validation passed");
    return OFSErrorCodes::SUCCESS;
}

// ============================================================================
// PRIVATE METHODS
// ============================================================================

bool ConfigParser::parseSection(const std::string& section_name,
                               const std::string& line,
                               Config& config)
{
    std::string key, value;

    if (!parseKeyValue(line, key, value))
        return false;

    if (section_name == "filesystem")
    {
        if (key == "total_size")
            config.total_size = std::stoull(value);
        else if (key == "header_size")
            config.header_size = std::stoull(value);
        else if (key == "block_size")
            config.block_size = std::stoull(value);
        else if (key == "max_files")
            config.max_files = std::stoul(value);
        else if (key == "max_filename_length")
            config.max_filename_length = std::stoul(value);
        else
        {
            LOG_WARN("CONFIG", 200,
                    "Unknown key in [filesystem]: " + key);
        }
    }
    else if (section_name == "security")
    {
        if (key == "max_users")
            config.max_users = std::stoul(value);
        else if (key == "admin_username")
            config.admin_username = value;
        else if (key == "admin_password")
            config.admin_password = value;
        else if (key == "require_auth")
            config.require_auth = (value == "true" || value == "1" || value == "yes");
        else if (key == "private_key")
            config.private_key = value;
        else
        {
            LOG_WARN("CONFIG", 200,
                    "Unknown key in [security]: " + key);
        }
    }
    else if (section_name == "server")
    {
        if (key == "port")
            config.port = std::stoul(value);
        else if (key == "max_connections")
            config.max_connections = std::stoul(value);
        else if (key == "queue_timeout")
            config.queue_timeout = std::stoul(value);
        else
        {
            LOG_WARN("CONFIG", 200,
                    "Unknown key in [server]: " + key);
        }
    }
    else
    {
        LOG_WARN("CONFIG", 201,
                "Unknown section: " + section_name);
    }

    return true;
}

std::string ConfigParser::trim(const std::string& str)
{
    auto start = str.begin();
    while (start != str.end() && std::isspace(*start))
        start++;

    auto end = str.end();
    do
    {
        end--;
    } while (std::distance(start, end) > 0 && std::isspace(*end));

    return std::string(start, end + 1);
}

bool ConfigParser::isPowerOfTwo(uint64_t value) const
{
    if (value == 0)
        return false;
    return (value & (value - 1)) == 0;
}

bool ConfigParser::parseKeyValue(const std::string& line,
                                std::string& key,
                                std::string& value)
{
    size_t equals_pos = line.find('=');
    if (equals_pos == std::string::npos)
        return false;

    key = trim(line.substr(0, equals_pos));
    value = trim(line.substr(equals_pos + 1));

    // Remove quotes if present
    if (!value.empty() && value.front() == '"' && value.back() == '"')
        value = value.substr(1, value.length() - 2);

    return !key.empty() && !value.empty();
}

}  // namespace ofs
