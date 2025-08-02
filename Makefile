CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98

SRC =	src/main.cpp \
		src/Parser/ConfigParser.cpp \
		src/Server/Server.cpp \
		src/Request/Request.cpp \
		src/Client/Client.cpp \
		src/Response/Response.cpp \
		src/CGI/CGI.cpp \
		src/Utils/Logger.cpp

OBJ_DIR = objFiles

OBJ = $(addprefix $(OBJ_DIR)/, $(notdir $(SRC:.cpp=.o)))

HEADERS =	src/Parser/ConfigParser.hpp \
			src/Server/Server.hpp \
			src/Request/Request.hpp \
			src/Client/Client.hpp \
			src/Response/Response.hpp \
			src/CGI/CGI.hpp \
			src/Utils/Logger.hpp

INCLUDES = -I. -Isrc -Isrc/Parser -Isrc/Server -Isrc/Request -Isrc/Client -Isrc/Response -Isrc/CGI -Isrc/Utils

VPATH = src:src/Parser:src/Server:src/Request:src/Client:src/Response:src/CGI:src/Cookies:src/SessionManager:src/Utils

all: $(OBJ_DIR) Webserv

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

Webserv: $(OBJ)
	$(CXX) $(CXXFLAGS) -o Webserv $(OBJ)

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
