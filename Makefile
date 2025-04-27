CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++11

SRC = main.cpp ./config/configParser.cpp ./config/serverConfig.cpp ./config/requestParser.cpp
OBJ_DIR = objFiles
OBJ = $(addprefix $(OBJ_DIR)/, $(notdir $(SRC:.cpp=.o)))

all: $(OBJ_DIR) Webserv

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(OBJ_DIR)/%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: ./config/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

Webserv: $(OBJ)
	$(CXX) $(CXXFLAGS) -o Webserv $(OBJ)

clean:
	rm -f $(OBJ)
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -rf Webserv

re: fclean all

.SECONDARY:
.PHONY: all clean fclean re $(OBJ_DIR)
