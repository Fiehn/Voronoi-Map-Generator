#pragma once
#include <atomic>
#include <thread>
#include <vector>
#include <string>
#include "LogCommon.h"

// Asynchronous Flight Recorder Logger

constexpr size_t BUFFER_SIZE = 4096;

// Log rotation settings
constexpr size_t MAX_LOG_FILE_SIZE = 10 * 1024 * 1024; // 10 MB
constexpr size_t MAX_LOG_FILES = 5; // Keep last 5 log files

class Logger {
public:
	static Logger& Get() {
		static Logger instance;
		return instance;
	}

	// The main thread calls this. no locks. fast.
	template<typename... Args>
	void Log(LogLevel level, LogCategory cat, const char* file, int line, const char* fmt, Args... args)
	{
		// 1. Reserve a slot index atomically
		uint64_t current_write_index = write_index.fetch_add(1, std::memory_order_relaxed);
		uint64_t slot = current_write_index & (BUFFER_SIZE - 1);

		// 2. Write data to the slot
		LogEvent& event = ring_buffer[slot];
		event.tick = current_sim_tick.load(std::memory_order_relaxed);
		event.level = level;
		event.category = cat;
		event.file = file;
		event.line = line;
		event.message = fmt;

		// Packs arguments
		event.data_count = 0;
		((event.data[event.data_count++] = LogPayload(args)), ...);

		// 3. Mark slot as readable (publish)
		flags[slot].store(1, std::memory_order_release);
	}
	void SetTick(uint64_t tick) {
		current_sim_tick.store(tick, std::memory_order_relaxed);
	}
	void Flush(); // Force to write to file
	void SetMaxFileSize(size_t size_bytes) { max_file_size = size_bytes; }
	void SetMaxLogFiles(size_t max_files) { max_log_files = max_files; }

private:
	Logger();
	~Logger();

	void WorkerLoop();
	void RotateLogFile();
	void CleanupOldLogs();

	std::vector<LogEvent> ring_buffer;
	std::atomic<int>* flags;  // Changed to raw pointer
	std::atomic<uint64_t> write_index{ 0 };
	std::atomic<uint64_t> read_index{ 0 };

	std::atomic<bool> is_running{ true };
	std::thread worker_thread;
	std::atomic<uint64_t> current_sim_tick{ 0 };

	std::atomic<size_t> current_file_size{ 0 };
	size_t max_file_size{ MAX_LOG_FILE_SIZE };
	size_t max_log_files{ MAX_LOG_FILES };
	std::string current_log_path;
};

// Macro for easy logging
#define LOG_INFO(cat, msg, ...) Logger::Get().Log(LogLevel::Info, LogCategory::cat, __FILE__, __LINE__, msg, ##__VA_ARGS__)
#define LOG_WARNING(cat, msg, ...) Logger::Get().Log(LogLevel::Warning, LogCategory::cat, __FILE__, __LINE__, msg, ##__VA_ARGS__)
#define LOG_ERROR(cat, msg, ...) Logger::Get().Log(LogLevel::Error, LogCategory::cat, __FILE__, __LINE__, msg, ##__VA_ARGS__)