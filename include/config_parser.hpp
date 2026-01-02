#ifndef CONFIG_PARSER_HPP
#define CONFIG_PARSER_HPP

#include <string>
#include <map>
#include <cstdint>

class ConfigParser {
public:
    static bool load(const std::string& config_path);
    
    static std::string get_string(const std::string& section, const std::string& key, 
                                  const std::string& default_val = "");
    
    static int64_t get_int(const std::string& section, const std::string& key, 
                          int64_t default_val = 0);
    
    static uint64_t get_uint(const std::string& section, const std::string& key,
                            uint64_t default_val = 0);
    
    static bool get_bool(const std::string& section, const std::string& key,
                        bool default_val = false);
    
    static void print_config();
    
    static bool validate();

private:
    static std::map<std::string, std::map<std::string, std::string>> config;
    static bool loaded;
    
    static std::string trim(const std::string& str);
    static std::string to_lower(const std::string& str);
};

#endif
