#include "../source/include/logger.hpp"
#include <iostream>
#include <thread>
#include <vector>
#include <string>
#include <fstream>
#include <regex>
#include <filesystem>
#include <cassert>

// Define a test module name
#define TEST_MODULE "LOGGER_TEST"

// Function to read the last N lines of a file
std::vector<std::string> read_last_lines(const std::string& file_path, size_t n) {
    std::vector<std::string> lines;
    std::ifstream file(file_path);
    if (!file.is_open()) {
        return lines;
    }

    // Go to the end of the file
    file.seekg(0, std::ios_base::end);
    std::string line;
    size_t line_count = 0;

    // Read backwards
    for (std::streamoff i = file.tellg(); i > 0; --i) {
        file.seekg(i);
        char c;
        file.get(c);
        if (c == '\n') {
            line_count++;
            if (line_count > n) {
                break;
            }
        }
    }
    file.seekg(file.tellg() + (line_count > 0 ? 1 : 0)); // Adjust position to start of the first desired line

    while (std::getline(file, line)) {
        lines.push_back(line);
    }
    return lines;
}

// Test case 1: Log Rotation
void test_log_rotation() {
    std::cout << "Running Log Rotation Test..." << std::endl;
    std::string test_log_file = "./file-verse/logs/rotation_test.log";
    ofs::Logger::get_instance().set_log_file(test_log_file);
    ofs::Logger::get_instance().set_app_name("RotationTester");

    // Clear previous logs
    if (std::filesystem::exists(test_log_file)) {
        std::filesystem::remove(test_log_file);
    }

    // Write enough data to trigger rotation (max_file_size_bytes_ is 1MB)
    // Each log entry is roughly ~150-200 bytes, so 5000 lines should be > 1MB
    for (int i = 0; i < 7000; ++i) {
        LOG_INFO(TEST_MODULE, 100, "This is a log message to test rotation. Line: " + std::to_string(i));
    }

    // Check if old log file exists
    bool rotated = false;
    for (const auto& entry : std::filesystem::directory_iterator("./file-verse/logs")) {
        if (entry.path().string().find(".log.") != std::string::npos &&
            entry.path().string().find("rotation_test.log") != std::string::npos) {
            rotated = true;
            break;
        }
    }
    assert(rotated && "Log file should have rotated.");
    std::cout << "Log Rotation Test PASSED." << std::endl;
}

// Test case 2: Thread Safety and Format
void test_thread_safety_and_format() {
    std::cout << "Running Thread Safety and Format Test..." << std::endl;
    std::string test_log_file = "./file-verse/logs/thread_test.log";
    ofs::Logger::get_instance().set_log_file(test_log_file);
    ofs::Logger::get_instance().set_app_name("ThreadTester");

    if (std::filesystem::exists(test_log_file)) {
        std::filesystem::remove(test_log_file);
    }

    const int num_threads = 5;
    const int logs_per_thread = 1000;
    std::vector<std::thread> threads;

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([i, logs_per_thread]() {
            for (int j = 0; j < logs_per_thread; ++j) {
                LOG_INFO(TEST_MODULE, 200 + i, "Thread " + std::to_string(i) + " log message " + std::to_string(j));
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    // Verify format of some lines
    std::ifstream log_file(test_log_file);
    std::string line;
    std::regex log_pattern(R"regex_delimiter(\[(\d{4})-(\d{2})-(\d{2})T(\d{2}):(\d{2}):(\d{2})Z\] level=(DEBUG|INFO|WARN|ERROR|FATAL) module=([A-Za-z_]+) code=(\d+) msg="([^"]*)" file="([^"]*)" line=(\d+))regex_delimiter");
    int checked_lines = 0;
    while (std::getline(log_file, line) && checked_lines < 100) { // Check first 100 lines
        assert(std::regex_match(line, log_pattern) && "Log format is incorrect.");
        checked_lines++;
    }
    std::cout << "Thread Safety and Format Test PASSED." << std::endl;
}

// Test case 3: Circular Buffer
void test_circular_buffer() {
    std::cout << "Running Circular Buffer Test..." << std::endl;
    std::string test_log_file = "./file-verse/logs/buffer_test.log";
    ofs::Logger::get_instance().set_log_file(test_log_file);
    ofs::Logger::get_instance().set_app_name("BufferTester");
    ofs::Logger::get_instance().set_terminal_output(false); // Disable terminal output for this test

    if (std::filesystem::exists(test_log_file)) {
        std::filesystem::remove(test_log_file);
    }

    const int total_logs = 1500; // More than circular_buffer_capacity_ (1000)
    for (int i = 0; i < total_logs; ++i) {
        LOG_INFO(TEST_MODULE, 300, "Buffer test message " + std::to_string(i));
    }

    // Temporarily redirect cerr to a stringstream to capture dump_circular_buffer output
    std::ostringstream oss;
    std::streambuf* old_cerr = std::cerr.rdbuf();
    std::cerr.rdbuf(oss.rdbuf());

    ofs::Logger::get_instance().dump_circular_buffer();

    std::cerr.rdbuf(old_cerr); // Restore cerr

    std::string buffer_dump = oss.str();
    // Check if the dump contains the last 1000 messages correctly (messages 500-1499)
    for (int i = 500; i < total_logs; ++i) {
        std::string expected_msg = "msg=\"Buffer test message " + std::to_string(i) + "\"";
        assert(buffer_dump.find(expected_msg) != std::string::npos && "Circular buffer missing expected message.");
    }
    // Check that early messages are not present
    std::string early_msg = "msg=\"Buffer test message 0\"";
    assert(buffer_dump.find(early_msg) == std::string::npos && "Circular buffer contains old messages.");

    std::cout << "Circular Buffer Test PASSED." << std::endl;
}

// Test case 4: Terminal Output Control
void test_terminal_output_control() {
    std::cout << "Running Terminal Output Control Test..." << std::endl;
    std::string test_log_file = "./file-verse/logs/terminal_test.log";
    ofs::Logger::get_instance().set_log_file(test_log_file);
    ofs::Logger::get_instance().set_app_name("TerminalTester");

    if (std::filesystem::exists(test_log_file)) {
        std::filesystem::remove(test_log_file);
    }

    // Enable terminal output (default)
    ofs::Logger::get_instance().set_terminal_output(true);
    std::ostringstream cout_buffer_on;
    std::streambuf* old_cout = std::cout.rdbuf();
    std::cout.rdbuf(cout_buffer_on.rdbuf());
    LOG_INFO(TEST_MODULE, 400, "This should appear on terminal.");
    std::cout.rdbuf(old_cout);
    assert(cout_buffer_on.str().find("This should appear on terminal.") != std::string::npos && "Terminal output should be enabled.");

    // Disable terminal output
    ofs::Logger::get_instance().set_terminal_output(false);
    std::ostringstream cout_buffer_off;
    old_cout = std::cout.rdbuf();
    std::cout.rdbuf(cout_buffer_off.rdbuf());
    LOG_INFO(TEST_MODULE, 401, "This should NOT appear on terminal.");
    std::cout.rdbuf(old_cout);
    assert(cout_buffer_off.str().find("This should NOT appear on terminal.") == std::string::npos && "Terminal output should be disabled.");

    std::cout << "Terminal Output Control Test PASSED." << std::endl;
}


int main() {
    std::cout << "Starting Logger Tests..." << std::endl;

    // Ensure logs directory exists
    std::filesystem::create_directories("./file-verse/logs");

    test_log_rotation();
    test_thread_safety_and_format();
    test_circular_buffer();
    test_terminal_output_control();

    std::cout << "All Logger Tests PASSED." << std::endl;
    return 0;
}
