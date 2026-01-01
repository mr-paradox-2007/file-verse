#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <string>
#include <fstream>
#include <mutex>
#include <chrono>
#include <ctime>
#include <vector> // For circular buffer

#define MODULE_NAME_DEFAULT "UNKNOWN_MODULE" // Default module name if not specified by macro

namespace ofs
{

enum class LogLevel
{
    debug,
    info,
    warn,
    error,
    fatal
};

class Logger
{
private:
    std::ofstream file_stream_;
    std::mutex mtx_;
    std::string log_file_path_;
    static constexpr size_t max_file_size_bytes_ = 1024 * 1024;
    std::string app_identifier_;
    int process_id_;
    bool terminal_output_enabled_; // For enabling/disabling console output
    std::vector<std::string> circular_buffer_; // Last 1000 entries
    const size_t circular_buffer_capacity_ = 1000;

    Logger(); // Private constructor for Singleton pattern
    void rotate_if_needed();
    bool file_was_rotated();
    std::string get_timestamp_utc();
    std::string level_to_string(LogLevel level);
    void initialize_app_identifier();
    void add_to_circular_buffer(const std::string& log_entry);

    void write_internal(LogLevel level,
                        const std::string& module,
                        int code,
                        const std::string& msg,
                        const std::string& src_file,
                        int line);

public:

    ~Logger();

    static Logger& get_instance();

    void set_log_file(const std::string& path);
    void set_app_name(const std::string& name);
    void set_terminal_output(bool enabled);
    bool is_terminal_output_enabled() const;
    void dump_circular_buffer(); // For crash debugging

    void log(LogLevel level,
                const std::string& module,
                int code,
                const std::string& msg,
                const std::string& src_file,
                int line)
    {
        write_internal(level, module, code, msg, src_file, line);
    }

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
};

} // namespace ofs

// Usage macros
#define LOG_DEBUG(module, code, msg) \
    ofs::Logger::get_instance().log(ofs::LogLevel::debug, (module), (code), (msg), __FILE__, __LINE__)

#define LOG_INFO(module, code, msg) \
    ofs::Logger::get_instance().log(ofs::LogLevel::info, (module), (code), (msg), __FILE__, __LINE__)

#define LOG_WARN(module, code, msg) \
    ofs::Logger::get_instance().log(ofs::LogLevel::warn, (module), (code), (msg), __FILE__, __LINE__)

#define LOG_ERROR(module, code, msg) \
    ofs::Logger::get_instance().log(ofs::LogLevel::error, (module), (code), (msg), __FILE__, __LINE__)

#define LOG_FATAL(module, code, msg) \
    ofs::Logger::get_instance().log(ofs::LogLevel::fatal, (module), (code), (msg), __FILE__, __LINE__)

#endif // LOGGER_HPP
