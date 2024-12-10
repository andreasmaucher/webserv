NAME = webserv
SRC_DIR = src
HTTP_DIR = $(SRC_DIR)/http
SERV_DIR = $(SRC_DIR)/server
TEST_DIR = tests
SOURCES = $(SRC_DIR)/main.cpp $(SERV_DIR)/server.cpp $(SERV_DIR)/serverConfig.cpp $(TEST_DIR)/testResponseHandler.cpp $(HTTP_DIR)/httpRequest.cpp $(HTTP_DIR)/requestParser.cpp $(HTTP_DIR)/httpResponse.cpp $(HTTP_DIR)/responseHandler.cpp $(HTTP_DIR)/mimeTypeMapper.cpp
		
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