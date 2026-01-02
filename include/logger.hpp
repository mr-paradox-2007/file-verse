#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <string>
#include <fstream>
#include <mutex>
#include <ctime>

class Logger {
public:
    enum class Level {
        DEBUG = 0,
        INFO = 1,
        WARN = 2,
        ERROR = 3
    };
    
    static void init(const std::string& log_file = "logs/ofs.log");
    
    static void shutdown();
    
    static void log(Level level, const std::string& message, const std::string& username = "system");
    
    static void debug(const std::string& msg, const std::string& user = "system");
    static void info(const std::string& msg, const std::string& user = "system");
    static void warn(const std::string& msg, const std::string& user = "system");
    static void error(const std::string& msg, const std::string& user = "system");
    
    static void log_file_op(const std::string& op_type, const std::string& path, 
                           const std::string& username, bool success, const std::string& details = "");
    
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
