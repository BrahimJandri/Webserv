#include "Logger.hpp"
#include "AnsiColor.hpp"
#include <iostream>
#include <sstream>

namespace Utils {
	void log(const std::string& message, const std::string& color) {
		std::cout << color << message << AnsiColor::RESET << std::endl;
	}

	void logError(const std::string& message) {
		std::cerr << AnsiColor::RED << "ERROR: " << message << AnsiColor::RESET << std::endl;
	}

	std::string intToString(int value) {
		std::stringstream ss;
		ss << value;
		return ss.str();
	}
}
