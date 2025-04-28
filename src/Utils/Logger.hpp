#pragma once

#include <string>

namespace Utils {
	void log(const std::string& message, const std::string& color = "");
	void logError(const std::string& message);
	std::string intToString(int value);
}
