NAME = webserv
SRC_DIR = src
HTTP_DIR = $(SRC_DIR)/http
SERV_DIR = $(SRC_DIR)/server
TEST_DIR = tests
SOURCES = $(SRC_DIR)/main.cpp $(SERV_DIR)/webService.cpp $(SERV_DIR)/server.cpp $(TEST_DIR)/testMultiServer.cpp $(HTTP_DIR)/httpRequest.cpp $(HTTP_DIR)/requestParser.cpp $(HTTP_DIR)/httpResponse.cpp $(HTTP_DIR)/responseHandler.cpp $(HTTP_DIR)/mimeTypeMapper.cpp $(SERV_DIR)/Parser.cpp
		
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