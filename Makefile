NAME = webserv
SRC_DIR = src
HTTP_DIR = $(SRC_DIR)/http
TEST_DIR = tests
SOURCES = $(SRC_DIR)/main.cpp $(TEST_DIR)/testParser.cpp $(HTTP_DIR)/requestParser.cpp $(HTTP_DIR)/httpRequest.cpp
		
OBJS = $(SOURCES:.cpp=.o)

CXX = c++
RM = rm -f
CXXFLAGS = -g -Wall -Wextra -Werror -std=c++98

all: $(NAME)	

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	$(RM) $(OBJS)

fclean: clean
	$(RM) $(NAME)

re: fclean all

.PHONY: all clean fclean re