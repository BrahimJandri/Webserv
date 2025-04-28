/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   WebservUtils.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: abamksa <abamksa@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/28 10:07:23 by abamksa           #+#    #+#             */
/*   Updated: 2025/04/28 10:07:27 by abamksa          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <poll.h>
#include <sys/time.h>
#include "AnsiColor.hpp"

// Common constants
#define MAX_CONNECTIONS 1024
#define DEFAULT_BUFFER_SIZE 8192
#define CONNECTION_TIMEOUT 60  // seconds

// Utility functions
namespace Utils {
	// Convert integer to string (for C++98 compatibility)
	std::string intToString(int number);

	// Trim whitespace from both ends of a string
	void trim(std::string &str);

	// Set socket to non-blocking mode
	bool setNonBlocking(int socketFd);

	// Get current timestamp in milliseconds
	long long getCurrentTimeMillis();

	// Log message with optional color
	void log(const std::string &message, const char* color = AnsiColor::RESET);

	// Log error message with backtrace
	void logError(const std::string &message);
}
