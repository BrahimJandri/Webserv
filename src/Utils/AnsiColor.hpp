#pragma once

namespace AnsiColor {
	// Text colors
	const char* const BLACK = "\033[0;30m";
	const char* const RED = "\033[0;31m";
	const char* const GREEN = "\033[0;32m";
	const char* const YELLOW = "\033[0;33m";
	const char* const BLUE = "\033[0;34m";
	const char* const MAGENTA = "\033[0;35m";
	const char* const CYAN = "\033[0;36m";
	const char* const WHITE = "\033[0;37m";

	// Bold text colors
	const char* const BOLD_BLACK = "\033[1;30m";
	const char* const BOLD_RED = "\033[1;31m";
	const char* const BOLD_GREEN = "\033[1;32m";
	const char* const BOLD_YELLOW = "\033[1;33m";
	const char* const BOLD_BLUE = "\033[1;34m";
	const char* const BOLD_MAGENTA = "\033[1;35m";
	const char* const BOLD_CYAN = "\033[1;36m";
	const char* const BOLD_WHITE = "\033[1;37m";

	// Background colors
	const char* const BG_BLACK = "\033[40m";
	const char* const BG_RED = "\033[41m";
	const char* const BG_GREEN = "\033[42m";
	const char* const BG_YELLOW = "\033[43m";
	const char* const BG_BLUE = "\033[44m";
	const char* const BG_MAGENTA = "\033[45m";
	const char* const BG_CYAN = "\033[46m";
	const char* const BG_WHITE = "\033[47m";

	// Formatting
	const char* const RESET = "\033[0m";
	const char* const BOLD = "\033[1m";
	const char* const UNDERLINE = "\033[4m";
	const char* const INVERSE = "\033[7m";
}
