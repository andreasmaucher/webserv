NAME = webserv
SRC_DIR = src
HTTP_DIR = $(SRC_DIR)/http
SERV_DIR = $(SRC_DIR)/server
CGI_DIR = $(SRC_DIR)/cgi
TEST_DIR = tests
SOURCES = $(SRC_DIR)/main.cpp $(SERV_DIR)/server.cpp $(TEST_DIR)/testResponseHandler.cpp $(HTTP_DIR)/httpRequest.cpp \
			$(HTTP_DIR)/requestParser.cpp $(HTTP_DIR)/httpResponse.cpp $(HTTP_DIR)/responseHandler.cpp $(HTTP_DIR)/mimeTypeMapper.cpp \
			$(CGI_DIR)/cgi.cpp $(SERV_DIR)/Parser.cpp $(SERV_DIR)/webService.cpp\
		
OBJS = $(SOURCES:.cpp=.o)

CXX = c++
RM = rm -f
CXXFLAGS = -g -Wall -Wextra -Werror -std=c++98

all: $(NAME)	

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -g -c $< -o $@

clean:
	$(RM) $(OBJS)

fclean: clean
	$(RM) $(NAME)

re: fclean all

.PHONY: all clean fclean re