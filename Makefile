NAME=ircserv

FLAGS= -Wall -Wextra -Werror -std=c++98 -g

SRCS= ircserv.cpp command.cpp message.cpp channel.cpp

HEADER= ircserv.hpp message.hpp

OBJS= $(SRCS:.cpp=.o)

all: $(NAME)

$(NAME) : $(OBJS)
	@c++ $(FLAGS) $(OBJS) -o $(NAME)

%.o: %.cpp
	@c++ -c $(FLAGS) $< -o $@

clean:
	@rm -f $(OBJS)

fclean: clean
	@rm -f $(NAME)

re: fclean all

run: re
	./ircserv 6667 pass