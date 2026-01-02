#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <string>
#include <fstream>
#include <mutex>
#include <ctime>

/**
 * Logging system for all OFS operations
 * Thread-safe, writes to logs/ofs.log
 * All operations automatically logged with timestamp and user info
 */
class Logger {
public:
    enum class Level {
        DEBUG = 0,
        INFO = 1,
        WARN = 2,
        ERROR = 3
    };
    
    /**
     * Initialize logger (opens log file)
     */
    static void init(const std::string& log_file = "logs/ofs.log");
    
    /**
     * Shutdown logger (closes file)
     */
    static void shutdown();
    
    /**
     * Log a message with level
     */
    static void log(Level level, const std::string& message, const std::string& username = "system");
    
    /**
     * Convenience methods
     */
    static void debug(const std::string& msg, const std::string& user = "system");
    static void info(const std::string& msg, const std::string& user = "system");
    static void warn(const std::string& msg, const std::string& user = "system");
    static void error(const std::string& msg, const std::string& user = "system");
    
    /**
     * Log file operation (automatically includes path, user, and result)
     */
    static void log_file_op(const std::string& op_type, const std::string& path, 
                           const std::string& username, bool success, const std::string& details = "");
    
    /**
     * Log user operation
     */
    static void log_user_op(const std::string& op_type, const std::string& target_user,
                           const std::string& actor_user, bool success, const std::string& details = "");

private:
    static std::ofstream log_file;
    static std::mutex log_mutex;
    static bool initialized;
    
    static std::string level_to_string(Level level);
    static std::string get_timestamp();
};

#endif
