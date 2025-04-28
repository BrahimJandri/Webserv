/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   WebservUtils.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: abamksa <abamksa@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/28 10:07:40 by abamksa           #+#    #+#             */
/*   Updated: 2025/04/28 10:08:27 by abamksa          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Headers/WebservUtils.hpp"

namespace Utils {
	// Convert integer to string (for C++98 compatibility)
	std::string intToString(int number) {
		std::stringstream ss;
		ss << number;
		return ss.str();
	}

	// Trim whitespace from both ends of a string
	void trim(std::string &str) {
		size_t start = str.find_first_not_of(" \t\n\r\f\v");
		if (start == std::string::npos) {
			str = "";
			return;
		}
		
		size_t end = str.find_last_not_of(" \t\n\r\f\v");
		str = str.substr(start, end - start + 1);
	}

	// Set socket to non-blocking mode
	bool setNonBlocking(int socketFd) {
		int flags = fcntl(socketFd, F_GETFL, 0);
		if (flags == -1) {
			logError("Failed to get socket flags");
			return false;
		}
		
		if (fcntl(socketFd, F_SETFL, flags | O_NONBLOCK) == -1) {
			logError("Failed to set non-blocking flag");
			return false;
		}
		
		return true;
	}

	// Get current timestamp in milliseconds
	long long getCurrentTimeMillis() {
		struct timeval tv;
		gettimeofday(&tv, NULL);
		return (long long)(tv.tv_sec) * 1000 + (long long)(tv.tv_usec) / 1000;
	}

	// Log message with optional color
	void log(const std::string &message, const char* color) {
		std::cout << color << "[INFO] " << message << AnsiColor::RESET << std::endl;
	}

	// Log error message with backtrace
	void logError(const std::string &message) {
		std::cerr << AnsiColor::RED << "[ERROR] " << message;
		// Include error code if applicable
		if (errno != 0) {
			std::cerr << " (Error " << errno << ": " << strerror(errno) << ")";
		}
		std::cerr << AnsiColor::RESET << std::endl;
	}
}
