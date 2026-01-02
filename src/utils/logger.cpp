#include "logger.hpp"
#include <iostream>
#include <iomanip>
#include <ctime>

std::ofstream Logger::log_file;
std::mutex Logger::log_mutex;
bool Logger::initialized = false;

std::string Logger::get_timestamp() {
    auto now = std::time(nullptr);
    auto tm = std::localtime(&now);
    std::stringstream ss;
    ss << std::put_time(tm, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

std::string Logger::level_to_string(Level level) {
    switch (level) {
        case Level::DEBUG: return "DEBUG";
        case Level::INFO:  return "INFO";
        case Level::WARN:  return "WARN";
        case Level::ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

void Logger::init(const std::string& log_file_path) {
    std::lock_guard<std::mutex> lock(log_mutex);
    
    if (initialized) {
        return;
    }
    
    // Try to create logs directory (best effort)
    int ret = system("mkdir -p logs 2>/dev/null");
    (void)ret;  // Suppress unused variable warning
    
    log_file.open(log_file_path, std::ios::app);
    if (!log_file.is_open()) {
        std::cerr << "Failed to open log file: " << log_file_path << std::endl;
        return;
    }
    
    initialized = true;
    // Don't call info() here as it will try to acquire the mutex again (deadlock)
    // Just write directly
    if (log_file.is_open()) {
        std::string timestamp = get_timestamp();
        log_file << "[" << timestamp << "] [INFO] [system] === Logger initialized ===" << std::endl;
        log_file.flush();
    }
}

void Logger::shutdown() {
    std::lock_guard<std::mutex> lock(log_mutex);
    
    if (log_file.is_open()) {
        info("=== Logger shutting down ===");
        log_file.close();
    }
    
    initialized = false;
}

void Logger::log(Level level, const std::string& message, const std::string& username) {
    std::lock_guard<std::mutex> lock(log_mutex);
    
    if (!initialized || !log_file.is_open()) {
        return;
    }
    
    std::string timestamp = get_timestamp();
    std::string level_str = level_to_string(level);
    
    std::string log_line = "[" + timestamp + "] [" + level_str + "] [" + username + "] " + message;
    
    log_file << log_line << std::endl;
    log_file.flush();
    
    // Also print to console for important messages
    if (level >= Level::WARN) {
        std::cout << log_line << std::endl;
    }
}

void Logger::debug(const std::string& msg, const std::string& user) {
    log(Level::DEBUG, msg, user);
}

void Logger::info(const std::string& msg, const std::string& user) {
    log(Level::INFO, msg, user);
}

void Logger::warn(const std::string& msg, const std::string& user) {
    log(Level::WARN, msg, user);
}

void Logger::error(const std::string& msg, const std::string& user) {
    log(Level::ERROR, msg, user);
}

void Logger::log_file_op(const std::string& op_type, const std::string& path,
                        const std::string& username, bool success, const std::string& details) {
    std::string msg = op_type + " " + path + " [" + (success ? "OK" : "FAILED") + "]";
    if (!details.empty()) {
        msg += " " + details;
    }
    log(success ? Level::INFO : Level::WARN, msg, username);
}

void Logger::log_user_op(const std::string& op_type, const std::string& target_user,
                        const std::string& actor_user, bool success, const std::string& details) {
    std::string msg = op_type + " " + target_user + " [" + (success ? "OK" : "FAILED") + "]";
    if (!details.empty()) {
        msg += " " + details;
    }
    log(success ? Level::INFO : Level::WARN, msg, actor_user);
}
