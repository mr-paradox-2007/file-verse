#include "../include/logger.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <filesystem>
#include <stdexcept>
#include <sys/stat.h> // For stat struct and function

#include <unistd.h> // For getpid
#include <limits.h> // For PATH_MAX

using std::cout;
using std::cerr;
using std::endl;

namespace ofs
{

// Helper function to get the project root directory
// This function assumes the executable is run from within the project structure
std::string get_project_root()
{
    // Try to find a marker file/directory in parent paths
    std::filesystem::path current_path = std::filesystem::current_path();
    while (current_path.has_parent_path() && current_path != current_path.parent_path()) {
        if (std::filesystem::exists(current_path / "file-verse")) {
            return (current_path / "file-verse").string();
        }
        current_path = current_path.parent_path();
    }
    // Fallback if marker not found, assume file-verse is in current working directory
    if (std::filesystem::exists(std::filesystem::current_path() / "file-verse")) {
        return (std::filesystem::current_path() / "file-verse").string();
    }
    return "./file-verse"; // Default fallback
}


Logger::Logger() : terminal_output_enabled_(true) // Terminal output enabled by default
{
    std::string project_root = get_project_root();
    log_file_path_ = project_root + "/logs/ofs.log"; // Changed log file name

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

    if (gmtime_r(&time, &utc) == nullptr) {} // Linux specific

    std::ostringstream oss;
    oss << std::put_time(&utc, "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

bool Logger::file_was_rotated()
{
    if (!std::filesystem::exists(log_file_path_)) {
        return true;
    }

    struct stat current_stat;
    if (stat(log_file_path_.c_str(), &current_stat) != 0) {
        return true;
    }

    try {
        size_t disk_size = std::filesystem::file_size(log_file_path_);
        auto stream_pos = file_stream_.tellp();

        if (stream_pos > 0 && disk_size < (std::streampos)stream_pos / 2) {
            return true;
        }
    } catch (...) {
        return true;
    }

    return false;
}

void Logger::rotate_if_needed()
{
    if (!file_stream_.is_open()) {
        return;
    }

    if (!std::filesystem::exists(log_file_path_)) return;
    size_t current_size = 0;
    try {
        current_size = std::filesystem::file_size(log_file_path_);
    } catch (const std::filesystem::filesystem_error& e) {
        cerr << "LOG ERROR: Failed to check file size for rotation: " << e.what() << endl;
        return;
    }

    if (current_size < max_file_size_bytes_) return;

    try {
        std::string new_path = log_file_path_ + "." + get_timestamp_utc() + ".log";

        file_stream_.close();

        std::error_code ec;
        std::filesystem::rename(log_file_path_, new_path, ec);

        if (ec) {
            cerr << "LOG WARN: Rename failed (code: " << ec.value() << ", msg: " << ec.message() << "). File possibly locked by another process." << endl;
        }

        file_stream_.open(log_file_path_, std::ios::app);
    } catch (const std::exception& e) {
        cerr << "LOG ERROR: Catastrophic rotation failure: " << e.what() << endl;
    }
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

    add_to_circular_buffer(formatted_log_entry); // Add to circular buffer

    if (file_stream_.is_open() && file_was_rotated()) {
        file_stream_.close();
    }

    if (!file_stream_.is_open()) {
        file_stream_.open(log_file_path_, std::ios::app);
    }

    if (!file_stream_.is_open()) {
        std::string fallback_msg = formatted_log_entry + " (File stream permanently closed)";
        if (level == LogLevel::error || level == LogLevel::fatal) {
            cerr << fallback_msg << endl;
        } else if (terminal_output_enabled_) { // Only cout if terminal output is enabled
            cout << fallback_msg << endl;
        }

        if (level == LogLevel::fatal) {
            dump_circular_buffer(); // Dump buffer on fatal error
            std::exit(code);
        }
        return;
    }

    rotate_if_needed();

    if (file_stream_.is_open()) {
        file_stream_ << formatted_log_entry << "\n";
        file_stream_.flush();
    }

    if (terminal_output_enabled_) { // Only print to console if enabled
        std::ostringstream console_msg;
        console_msg << "[" << timestamp << "]"
                    << " [" << level_str << "]"
                    << " [" << module << "]"
                    << " " << msg;

        if (level == LogLevel::error || level == LogLevel::fatal) {
            cerr << console_msg.str() << endl;
        } else {
            cout << console_msg.str() << endl;
        }
    }

    if (level == LogLevel::fatal)
    {
        if (file_stream_.is_open()) {
            file_stream_.close();
        }
        dump_circular_buffer(); // Dump buffer on fatal error
        std::exit(code);
    }
}

} // namespace ofs
