// logger.h
#ifndef LOGGER_H
#define LOGGER_H

#include <ctime>
#include <cstddef>
#include <string>
#include <vector>
#include <mutex>
#include <fstream>
#include <deque>

class SystemLogger {
public:
    enum class LogLevel {
        DEBUG,
        INFO,
        WARNING,
        ERROR,
        CRITICAL
    };
    
    struct LogEntry {
        time_t timestamp;
        LogLevel level;
        std::string message;
        std::string source;
        
        LogEntry(LogLevel lvl, const std::string& msg, const std::string& src = "")
            : level(lvl), message(msg), source(src) {
            timestamp = std::time(nullptr);
        }
    };
    
    SystemLogger(const std::string& file, size_t max_buffer_size = 1000);
    ~SystemLogger();
    
    // 日志记录方法
    void log(LogLevel level, const std::string& message, 
             const std::string& source = "");
    void debug(const std::string& message, const std::string& source = "");
    void info(const std::string& message, const std::string& source = "");
    void warning(const std::string& message, const std::string& source = "");
    void error(const std::string& message, const std::string& source = "");
    void critical(const std::string& message, const std::string& source = "");
    
    // 日志查询
    std::vector<LogEntry> get_recent_logs(int count) const;
    std::vector<LogEntry> get_logs_by_level(LogLevel level) const;
    std::vector<LogEntry> get_logs_by_timerange(time_t start, time_t end) const;
    
    // 日志管理
    void clear_logs();
    void set_buffer_size(size_t size);
    bool export_logs(const std::string& filename) const;
    
    // 配置
    struct LogConfig {
        bool console_output;
        bool file_output;
        LogLevel min_level;
        size_t rotation_size;
    };
    
    void configure(const LogConfig& config);

    // 工具方法
    std::string level_to_string(LogLevel level) const;
    
private:
    std::string log_file;
    mutable std::mutex log_mutex;
    std::deque<LogEntry> log_buffer;
    size_t buffer_size;
    std::ofstream log_stream;
    
    void write_to_file(const LogEntry& entry);
    void flush_buffer();
};

#endif // LOGGER_H