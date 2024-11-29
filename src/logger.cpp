// logger.cpp
#include "../include/logger.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <ctime>
#include <iomanip>

SystemLogger::SystemLogger(const std::string& file, size_t max_buffer_size)
: log_file(file), buffer_size(max_buffer_size) {
	
	log_stream.open(log_file, std::ios::app);
	if(!log_stream) {
		std::cerr << "Failed to open log file: " << log_file << std::endl;
	}
}

SystemLogger::~SystemLogger() {
	flush_buffer();
	if(log_stream.is_open()) {
		log_stream.close();
	}
}

std::string SystemLogger::level_to_string(LogLevel level) const {
	switch(level) {
		case LogLevel::DEBUG:   return "DEBUG";
		case LogLevel::INFO:    return "INFO";
		case LogLevel::WARNING: return "WARNING";
		case LogLevel::ERROR:   return "ERROR";
		case LogLevel::CRITICAL:return "CRITICAL";
		default:               return "UNKNOWN";
	}
}

void SystemLogger::write_to_file(const LogEntry& entry) {
	if(!log_stream) return;
	
	// 格式化时间戳
	char timestamp[64];
	struct tm* timeinfo = localtime(&entry.timestamp);
	strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", timeinfo);
	
	// 写入日志条目
	log_stream << "[" << timestamp << "] "
	<< "[" << level_to_string(entry.level) << "] ";
	
	if(!entry.source.empty()) {
		log_stream << "[" << entry.source << "] ";
	}
	
	log_stream << entry.message << std::endl;
	log_stream.flush();
}

void SystemLogger::flush_buffer() {
	std::lock_guard<std::mutex> lock(log_mutex);
	
	while(!log_buffer.empty()) {
		write_to_file(log_buffer.front());
		log_buffer.pop_front();
	}
}

void SystemLogger::log(LogLevel level, const std::string& message,
	const std::string& source) {
		std::lock_guard<std::mutex> lock(log_mutex);
		
		LogEntry entry(level, message, source);
		log_buffer.push_back(entry);
		
		if(log_buffer.size() >= buffer_size) {
			flush_buffer();
		}
	}

void SystemLogger::debug(const std::string& message, const std::string& source) {
	log(LogLevel::DEBUG, message, source);
}

void SystemLogger::info(const std::string& message, const std::string& source) {
	log(LogLevel::INFO, message, source);
}

void SystemLogger::warning(const std::string& message, const std::string& source) {
	log(LogLevel::WARNING, message, source);
}

void SystemLogger::error(const std::string& message, const std::string& source) {
	log(LogLevel::ERROR, message, source);
}

void SystemLogger::critical(const std::string& message, const std::string& source) {
	log(LogLevel::CRITICAL, message, source);
	flush_buffer(); // 立即写入文件
}

std::vector<SystemLogger::LogEntry> SystemLogger::get_recent_logs(int count) const {
	std::lock_guard<std::mutex> lock(log_mutex);
	
	std::vector<LogEntry> recent_logs;
	auto it = log_buffer.rbegin();
	int num_logs = std::min(count, static_cast<int>(log_buffer.size()));
	
	for(int i = 0; i < num_logs; ++i) {
		recent_logs.push_back(*it++);
	}
	
	return recent_logs;
}

std::vector<SystemLogger::LogEntry> SystemLogger::get_logs_by_level(
	LogLevel level) const {
		
		std::lock_guard<std::mutex> lock(log_mutex);
		
		std::vector<LogEntry> filtered_logs;
		for(const auto& entry : log_buffer) {
			if(entry.level == level) {
				filtered_logs.push_back(entry);
			}
		}
		
		return filtered_logs;
	}

std::vector<SystemLogger::LogEntry> SystemLogger::get_logs_by_timerange(
	time_t start, time_t end) const {
		
		std::lock_guard<std::mutex> lock(log_mutex);
		
		std::vector<LogEntry> filtered_logs;
		for(const auto& entry : log_buffer) {
			if(entry.timestamp >= start && entry.timestamp <= end) {
				filtered_logs.push_back(entry);
			}
		}
		
		return filtered_logs;
	}

void SystemLogger::clear_logs() {
	std::lock_guard<std::mutex> lock(log_mutex);
	
	log_buffer.clear();
	
	// 清空日志文件
	log_stream.close();
	log_stream.open(log_file, std::ios::out | std::ios::trunc);
}

void SystemLogger::set_buffer_size(size_t size) {
	std::lock_guard<std::mutex> lock(log_mutex);
	
	buffer_size = size;
	if(log_buffer.size() >= buffer_size) {
		flush_buffer();
	}
}

bool SystemLogger::export_logs(const std::string& filename) const {
	std::ofstream export_file(filename);
	if(!export_file) {
		return false;
	}
	
	std::lock_guard<std::mutex> lock(log_mutex);
	
	for(const auto& entry : log_buffer) {
		// 格式化时间戳
		char timestamp[64];
		struct tm* timeinfo = localtime(&entry.timestamp);
		strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", timeinfo);
		
		export_file << "[" << timestamp << "] "
		<< "[" << level_to_string(entry.level) << "] ";
		
		if(!entry.source.empty()) {
			export_file << "[" << entry.source << "] ";
		}
		
		export_file << entry.message << std::endl;
	}
	
	return true;
}

void SystemLogger::configure(const LogConfig& config) {
	std::lock_guard<std::mutex> lock(log_mutex);
	// TODO: 实现日志配置功能
}
