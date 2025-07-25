CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98

# Source files
SRC =	src/main.cpp \
		src/Parser/ConfigParser.cpp \
		src/Server/Server.cpp \
		src/Request/Request.cpp \
		src/Client/Client.cpp \
		src/Response/Response.cpp \
		src/CGI/CGI.cpp \
		src/Utils/Logger.cpp

# Object directory
OBJ_DIR = objFiles

# Object files with directory prefix
OBJ = $(addprefix $(OBJ_DIR)/, $(notdir $(SRC:.cpp=.o)))

# Header files
HEADERS =	src/Parser/ConfigParser.hpp \
			src/Server/Server.hpp \
			src/Request/Request.hpp \
			src/Client/Client.hpp \
			src/Response/Response.hpp \
			src/CGI/CGI.hpp \
			src/Utils/Logger.hpp

# Include directories
INCLUDES = -I. -Isrc -Isrc/Parser -Isrc/Server -Isrc/Request -Isrc/Client -Isrc/Response -Isrc/CGI -Isrc/Utils

# Source directories for vpath
VPATH = src:src/Parser:src/Server:src/Request:src/Client:src/Response:src/CGI:src/Cookies:src/SessionManager:src/Utils

all: $(OBJ_DIR) Webserv

# Create object directory if it doesn't exist
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# Rule to link the executable
Webserv: $(OBJ)
	$(CXX) $(CXXFLAGS) -o Webserv $(OBJ)

# Rules for compiling source files with header dependencies
$(OBJ_DIR)/%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(OBJ)
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -rf Webserv

re: fclean all

.SECONDARY:
.PHONY: all clean fclean re
