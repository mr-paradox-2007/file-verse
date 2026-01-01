#include "../include/logger.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <filesystem>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>

using std::cerr;
using std::endl;

namespace ofs
{

std::string get_project_root()
{
    std::filesystem::path current_path = std::filesystem::current_path();
    while (current_path.has_parent_path() && current_path != current_path.parent_path()) {
        if (std::filesystem::exists(current_path / "file-verse")) {
            return (current_path / "file-verse").string();
        }
        current_path = current_path.parent_path();
    }
    if (std::filesystem::exists(std::filesystem::current_path() / "file-verse")) {
        return (std::filesystem::current_path() / "file-verse").string();
    }
    return "./file-verse";
}

Logger::Logger() : terminal_output_enabled_(false)
{
    std::string project_root = get_project_root();
    log_file_path_ = project_root + "/logs/ofs.log";

    initialize_app_identifier();

    std::filesystem::create_directories(std::filesystem::path(log_file_path_).parent_path());

    file_stream_.open(log_file_path_, std::ios::app);
    if (!file_stream_.is_open())
    {
        cerr << "FATAL: Logger failed to open log file: " << log_file_path_ << endl;
    }
}

Logger::~Logger()
{
    if (file_stream_.is_open()) {
        file_stream_.close();
    }
}

Logger& Logger::get_instance()
{
    static Logger instance;
    return instance;
}

void Logger::initialize_app_identifier()
{
    process_id_ = getpid();

    char path[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (len != -1) {
        path[len] = '\0';
        std::filesystem::path exe_path(path);
        app_identifier_ = exe_path.filename().string();
    } else {
        app_identifier_ = "unknown_app";
    }
}

void Logger::set_app_name(const std::string& name)
{
    std::lock_guard<std::mutex> lock(mtx_);
    app_identifier_ = name;
}

void Logger::set_log_file(const std::string& path)
{
    std::lock_guard<std::mutex> lock(mtx_);
    if (file_stream_.is_open()) {
        file_stream_.close();
    }
    log_file_path_ = path;
    std::filesystem::create_directories(std::filesystem::path(path).parent_path());
    file_stream_.open(log_file_path_, std::ios::app);
    if (!file_stream_.is_open()) {
        cerr << "CRITICAL: Failed to open new log file at " << log_file_path_ << endl;
    }
}

void Logger::set_terminal_output(bool enabled)
{
    std::lock_guard<std::mutex> lock(mtx_);
    terminal_output_enabled_ = enabled;
}

bool Logger::is_terminal_output_enabled() const
{
    return terminal_output_enabled_;
}

std::string Logger::level_to_string(LogLevel level)
{
    switch (level)
    {
        case LogLevel::debug: return "DEBUG";
        case LogLevel::info:  return "INFO";
        case LogLevel::warn:  return "WARN";
        case LogLevel::error: return "ERROR";
        case LogLevel::fatal: return "FATAL";
    }
    return "UNKNOWN";
}

std::string Logger::get_timestamp_utc()
{
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::tm utc {};

    if (gmtime_r(&time, &utc) == nullptr) {}

    std::ostringstream oss;
    oss << std::put_time(&utc, "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

bool Logger::file_was_rotated()
{
    return false;
}

void Logger::rotate_if_needed()
{
    // Disabled for final submission - keep single log file
}

void Logger::add_to_circular_buffer(const std::string& log_entry)
{
    if (circular_buffer_.size() >= circular_buffer_capacity_) {
        circular_buffer_.erase(circular_buffer_.begin());
    }
    circular_buffer_.push_back(log_entry);
}

void Logger::dump_circular_buffer()
{
    std::lock_guard<std::mutex> lock(mtx_);
    cerr << "--- Circular Buffer Dump (Last " << circular_buffer_.size() << " entries) ---" << endl;
    for (const auto& entry : circular_buffer_) {
        cerr << entry << endl;
    }
    cerr << "----------------------------------------------------" << endl;
}

void Logger::write_internal(LogLevel level,
                            const std::string& module,
                            int code,
                            const std::string& msg,
                            const std::string& src_file,
                            int line)
{
    std::lock_guard<std::mutex> lock(mtx_);

    std::string level_str = level_to_string(level);
    std::string timestamp = get_timestamp_utc();

    std::ostringstream formatted_log_entry_stream;
    formatted_log_entry_stream << "[" << timestamp << "]"
            << " level=" << level_str
            << " module=" << module
            << " code=" << code
            << " msg=\"" << msg << "\""
            << " file=\"" << src_file << "\""
            << " line=" << line;
    
    std::string formatted_log_entry = formatted_log_entry_stream.str();

    add_to_circular_buffer(formatted_log_entry);

    if (!file_stream_.is_open()) {
        file_stream_.open(log_file_path_, std::ios::app);
    }

    if (file_stream_.is_open()) {
        file_stream_ << formatted_log_entry << "\n";
        file_stream_.flush();
    }

    if (level == LogLevel::fatal)
    {
        if (file_stream_.is_open()) {
            file_stream_.close();
        }
        dump_circular_buffer();
        std::exit(code);
    }
}

}