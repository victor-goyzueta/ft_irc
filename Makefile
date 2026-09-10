NAME	=	ircserv

CXX		=	c++

FLAGS	=	-Wall -Werror -Wextra -std=c++98

SRCS	=	main.cpp		\
			src/Server.cpp	\
			src/Channel.cpp	\
			src/Client.cpp	\
			src/Utils.cpp	\

OBJS	=	$(SRCS:.cpp=.o)

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(FLAGS) $(OBJS) -o $(NAME)

%.o: %.cpp
	$(CXX) $(FLAGS) -c $< -o $@

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
