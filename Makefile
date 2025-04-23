CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98

SRC = main.cpp ./config/configParser.cpp ./config/serverConfig.cpp ./config/requestParser.cpp
OBJ = $(SRC:.cpp=.o)

all: Webserv

Webserv: $(OBJ)
	$(CXX) $(CXXFLAGS) -o Webserv $(OBJ)

clean:
	rm -f $(OBJ)

fclean: clean
	rm -rf Webserv

re: fclean all

.PHONY: all clean fclean re
