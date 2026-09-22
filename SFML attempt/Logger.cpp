#include "Logger.h"
#include <fstream>
#include <filesystem>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace fs = std::filesystem;

static std::string GetTimestampedFilename()
{
	auto now = std::chrono::system_clock::now();
	auto time_t_now = std::chrono::system_clock::to_time_t(now);
	std::tm tm_now;
	localtime_s(&tm_now, &time_t_now);

	std::ostringstream oss;
	oss << "Logs/log_"
		<< std::put_time(&tm_now, "%Y%m%d_%H%M%S")
		<< ".txt";
	return oss.str();
}

static const char* LogLevelToString(LogLevel level)
{
	switch (level) {
	case LogLevel::Info: return "INFO";
	case LogLevel::Warning: return "WARNING";
	case LogLevel::Error: return "ERROR";
	case LogLevel::Fatal: return "FATAL";
	default: return "UNKNOWN";
	}
}

static const char* LogCategortToString(LogCategory category)
{
	switch (category)
	{
	case LogCategory::General:
		return "GENERAL";
	case LogCategory::Map:
		return "MAP";
	case LogCategory::POP:
		return "POP";
	case LogCategory::Generations:
		return "GENERATIONS";
	case LogCategory::Graphics:
		return "GRAPHICS";
	case LogCategory::Culture:
		return "CULTURE";
	case LogCategory::Language:
		return "LANGUAGE";
	default:
		return "UNKNOWN";
	}
}

static std::string FormatLogEvent(const LogEvent& event)
{
	std::ostringstream oss;

	// Format: [TICK] [LEVEL] [CATEGORY] file:line - message {args}
	oss << "[" << event.tick << "] "
		<< "[" << LogLevelToString(event.level) << "] "
		<< "[" << LogCategortToString(event.category) << "] "
		<< event.file << ":" << event.line << " - "
		<< event.message;

	// Append arguments if any
	for (uint8_t i = 0; i < event.data_count; ++i)
	{
		oss << " {";
		std::visit([&oss](auto&& arg) {
			oss << arg;
			}, event.data[i]);
		oss << "}";
	}
	return oss.str();
}

void Logger::Flush()
{
	// Signal the worker thread to flush
	is_running.store(false, std::memory_order_relaxed);
	if (worker_thread.joinable()) {
		worker_thread.join();
	}

	// Restart the worker thread
	is_running.store(true, std::memory_order_relaxed);
	worker_thread = std::thread(&Logger::WorkerLoop, this);
}

Logger::Logger()
{
	fs::create_directories("Logs");

	ring_buffer.resize(BUFFER_SIZE);

	// Allocate raw array of atomics
	flags = new std::atomic<int>[BUFFER_SIZE];
	for (size_t i = 0; i < BUFFER_SIZE; ++i) {
		flags[i].store(0, std::memory_order_relaxed);
	}

	current_log_path = GetTimestampedFilename();
	CleanupOldLogs();

	worker_thread = std::thread(&Logger::WorkerLoop, this);
}

Logger::~Logger()
{
	is_running.store(false, std::memory_order_relaxed);
	if (worker_thread.joinable()) {
		worker_thread.join();
	}

	// Clean up the flags array
	delete[] flags;
}

void Logger::RotateLogFile()
{
	// Generate new filename
	current_log_path = GetTimestampedFilename();
	current_file_size.store(0, std::memory_order_relaxed);

	CleanupOldLogs();
}

void Logger::CleanupOldLogs()
{
	try {
		if (!fs::exists("Logs") || !fs::is_directory("Logs")) {
			return;
		}
		// Collect log files
		struct LogFileInfo {
			fs::path path;
			fs::file_time_type time;
		};

		std::vector<LogFileInfo> log_files;

		for (const auto& entry : fs::directory_iterator("Logs"))
		{
			if (entry.is_regular_file())
			{
				const auto& path = entry.path();
				if (path.extension() == ".txt" && path.filename().string().find("log_") == 0)
				{
					log_files.push_back({ path, fs::last_write_time(entry) });
				}
			}
		}
		// Sort by modification time (newest first)
		std::sort(log_files.begin(), log_files.end(),
			[](const LogFileInfo& a, const LogFileInfo& b) {
				return a.time > b.time;
			});
		// Remove files exceeding max_log_files
		if (log_files.size() > max_log_files)
		{
			for (size_t i = max_log_files; i < log_files.size(); ++i)
			{
				try {
					fs::remove(log_files[i].path);
				}
				catch (const fs::filesystem_error& e) {
					fprintf(stderr, "Logger: Failed to delete old log file %s: %s\n",
						log_files[i].path.string().c_str(), e.what());
				}
			}
		}
	}
	catch (const fs::filesystem_error& e) {
		fprintf(stderr, "Logger: Filesystem error during log cleanup: %s\n", e.what());
	}
}

void Logger::WorkerLoop()
{
	std::ofstream log_file(current_log_path, std::ios::out | std::ios::app);
	if (!log_file.is_open()) {
		fprintf(stderr, "Logger: Failed to open log file.\n");
		return;
	}
	while (true)
	{
		// Check if we should stop
		bool running = is_running.load(std::memory_order_relaxed);

		// Process all available log events
		bool processed_any = false;
		while (read_index.load(std::memory_order_relaxed) < write_index.load(std::memory_order_acquire))
		{
			uint64_t current_read_index = read_index.load(std::memory_order_relaxed);
			uint64_t slot = current_read_index & (BUFFER_SIZE - 1);

			// Wait until the slot is ready
			if (flags[slot].load(std::memory_order_acquire) == 1) {
				const LogEvent& event = ring_buffer[slot];
				std::string formatted_message = FormatLogEvent(event);

				// Check if rotation is needed
				size_t message_size = formatted_message.size() + 1; // +1 for newline
				if (current_file_size.load(std::memory_order_relaxed) + message_size > max_file_size) {
					log_file.flush();
					log_file.close();
					RotateLogFile();
					log_file.open(current_log_path, std::ios::out | std::ios::app);
					if (!log_file.is_open()) {
						fprintf(stderr, "Logger: Failed to open log file after rotation.\n");
						return;
					}
				}

				log_file << formatted_message << std::endl;
				current_file_size.fetch_add(message_size, std::memory_order_relaxed);

				flags[slot].store(0, std::memory_order_release); // Mark slot as free

				read_index.fetch_add(1, std::memory_order_relaxed);
				processed_any = true;
			}
			else {
				// Slot not ready yet
				break;
			}
		}
		if (processed_any) {
			log_file.flush();
		}
		if (!running && read_index.load(std::memory_order_relaxed) >= write_index.load(std::memory_order_acquire)) {
			// No more logs to process and stop requested
			break;
		}
		// Sleep briefly to avoid busy waiting
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	// Final flush
	log_file.flush();
	log_file.close();
}