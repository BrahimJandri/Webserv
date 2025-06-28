CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++11

# Source files
SRC =	src/main.cpp \
		src/Config/ConfigParser.cpp \
		src/Server/Server.cpp \
		src/Server/Client.cpp \
		src/HTTP/Request.cpp \
		src/HTTP/Response.cpp \
		src/CGI/CGIHandler.cpp \
		src/HTTP/ResponseBuilder.cpp \
		src/Utils/Logger.cpp

# Object directory
OBJ_DIR = objFiles

# Object files with directory prefix
OBJ = $(addprefix $(OBJ_DIR)/, $(notdir $(SRC:.cpp=.o)))

# Header files
HEADERS =	src/Config/ConfigParser.hpp \
			src/Server/Server.hpp \
			src/Server/Client.hpp \
			src/HTTP/Request.hpp \
			src/HTTP/Response.hpp \
			src/CGI/CGIHandler.hpp \
			src/Utils/Logger.hpp

# Include directories
INCLUDES = -I. -Isrc -Isrc/Config -Isrc/Server -Isrc/HTTP -Isrc/CGI -Isrc/Utils

# Source directories for vpath
VPATH = src:src/Config:src/Server:src/HTTP:src/CGI:src/Utils

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
