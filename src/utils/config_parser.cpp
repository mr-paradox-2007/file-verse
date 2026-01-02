#include "config_parser.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iostream>

std::map<std::string, std::map<std::string, std::string>> ConfigParser::config;
bool ConfigParser::loaded = false;

std::string ConfigParser::trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, last - first + 1);
}

std::string ConfigParser::to_lower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

bool ConfigParser::load(const std::string& config_path) {
    std::ifstream file(config_path);
    if (!file.is_open()) {
        std::cerr << "Failed to open config file: " << config_path << std::endl;
        return false;
    }
    
    config.clear();
    std::string line;
    std::string current_section;
    
    while (std::getline(file, line)) {
        line = trim(line);
        
        if (line.empty() || line[0] == ';' || line[0] == '#') {
            continue;
        }
        
        if (line[0] == '[' && line[line.length() - 1] == ']') {
            current_section = to_lower(line.substr(1, line.length() - 2));
            config[current_section] = {};
            continue;
        }
        
        size_t eq_pos = line.find('=');
        if (eq_pos != std::string::npos) {
            std::string key = trim(line.substr(0, eq_pos));
            std::string value = trim(line.substr(eq_pos + 1));
            
            key = to_lower(key);
            config[current_section][key] = value;
        }
    }
    
    file.close();
    loaded = true;
    return true;
}

std::string ConfigParser::get_string(const std::string& section, const std::string& key,
                                     const std::string& default_val) {
    std::string sec = to_lower(section);
    std::string k = to_lower(key);
    
    if (config.find(sec) != config.end() && config[sec].find(k) != config[sec].end()) {
        return config[sec][k];
    }
    
    return default_val;
}

int64_t ConfigParser::get_int(const std::string& section, const std::string& key,
                             int64_t default_val) {
    std::string str_val = get_string(section, key, "");
    if (str_val.empty()) {
        return default_val;
    }
    
    try {
        return std::stoll(str_val);
    } catch (...) {
        return default_val;
    }
}

uint64_t ConfigParser::get_uint(const std::string& section, const std::string& key,
                               uint64_t default_val) {
    std::string str_val = get_string(section, key, "");
    if (str_val.empty()) {
        return default_val;
    }
    
    try {
        return std::stoull(str_val);
    } catch (...) {
        return default_val;
    }
}

bool ConfigParser::get_bool(const std::string& section, const std::string& key,
                           bool default_val) {
    std::string str_val = to_lower(get_string(section, key, ""));
    
    if (str_val.empty()) {
        return default_val;
    }
    
    if (str_val == "true" || str_val == "yes" || str_val == "on" || str_val == "1") {
        return true;
    }
    
    if (str_val == "false" || str_val == "no" || str_val == "off" || str_val == "0") {
        return false;
    }
    
    return default_val;
}

void ConfigParser::print_config() {
    std::cout << "=== Configuration ===" << std::endl;
    for (const auto& section : config) {
        std::cout << "[" << section.first << "]" << std::endl;
        for (const auto& item : section.second) {
            std::cout << "  " << item.first << " = " << item.second << std::endl;
        }
    }
    std::cout << "====================" << std::endl;
}

bool ConfigParser::validate() {
    if (!loaded) {
        std::cerr << "Configuration not loaded" << std::endl;
        return false;
    }
    
    uint64_t total_size = get_uint("filesystem", "total_size", 0);
    if (total_size == 0) {
        std::cerr << "Invalid total_size in configuration" << std::endl;
        return false;
    }
    
    uint32_t block_size = get_uint("filesystem", "block_size", 4096);
    if (block_size == 0 || (block_size & (block_size - 1)) != 0) {
        std::cerr << "block_size must be power of 2" << std::endl;
        return false;
    }
    
    return true;
}
