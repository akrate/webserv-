NAME = webserv

CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -Iinclude

OBJ_DIR = obj

SRCS = src/main.cpp \
       src/Server.cpp \
       src/Parser.cpp \
       src/client.cpp \
       src/utils.cpp \
	   src/cgi.cpp \
       src/respons.cpp \
	   src/ParseServer.cpp \
	   src/ParseLocation.cpp \
	   src/ParseUtils.cpp

OBJS = $(SRCS:%.cpp=$(OBJ_DIR)/%.o)

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)
	@echo "✓ $(NAME) compiled successfully"

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re