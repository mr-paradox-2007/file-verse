#ifndef CONFIG_PARSER_HPP
#define CONFIG_PARSER_HPP

#include <string>
#include <map>
#include <cstdint>

/**
 * Configuration parser for .uconf files
 * Parses INI-style configuration used to initialize OFS
 */
class ConfigParser {
public:
    /**
     * Load and parse configuration file
     */
    static bool load(const std::string& config_path);
    
    /**
     * Get string value from config
     * section: "[section_name]"
     * key: "key_name"
     * default_val: value if not found
     */
    static std::string get_string(const std::string& section, const std::string& key, 
                                  const std::string& default_val = "");
    
    /**
     * Get integer value from config
     */
    static int64_t get_int(const std::string& section, const std::string& key, 
                          int64_t default_val = 0);
    
    /**
     * Get unsigned integer
     */
    static uint64_t get_uint(const std::string& section, const std::string& key,
                            uint64_t default_val = 0);
    
    /**
     * Get boolean value (true/false, yes/no, on/off, 1/0)
     */
    static bool get_bool(const std::string& section, const std::string& key,
                        bool default_val = false);
    
    /**
     * Print all loaded configuration (for debugging)
     */
    static void print_config();
    
    /**
     * Get validation: check if file system config is valid
     */
    static bool validate();

private:
    static std::map<std::string, std::map<std::string, std::string>> config;
    static bool loaded;
    
    static std::string trim(const std::string& str);
    static std::string to_lower(const std::string& str);
};

#endif
