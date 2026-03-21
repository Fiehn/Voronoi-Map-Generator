#pragma once
#include <cstdint>
#include <string_view>
#include <variant>

enum class LogLevel { Info, Warning, Error, Fatal};
enum class LogCategory { General, Map, POP, Generations, Graphics, Culture, Language, Religion};

// Variant type for log payloads (storing multiple types on the heap)
using LogPayload = std::variant<int, unsigned int, unsigned long, uint64_t, float, bool, double, std::string>;

struct LogEvent {
	uint64_t tick = 0; // Simulation tick when the event occurred (0 = non-simulation event)
	LogLevel level = LogLevel::Info;
	LogCategory category = LogCategory::General;
	const char* file;       // Static string pointer (compile time)
	int line;           // Line number in the source file
	const char* message;    // Static format string
	LogPayload data[8]; // Up to 8 arguments
	uint8_t data_count = 0; // Number of arguments
};