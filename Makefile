NAME=ircserv

FLAGS= -Wall -Wextra -Werror

SRCS= main.cpp ircserv.cpp command.cpp message.cpp

HEADER= ircserv.hpp message.hpp

OBJS= $(SRCS:.cpp=.o)

all: $(NAME)

$(NAME) : $(OBJS)
	c++ $(OBJS) -o $(NAME)

%.o: %.cpp
	c++ -c $(FLAGS) $< -o $@

clean:
	@rm -f $(OBJS)
	# echo $(OBJS)

fclean: clean
	@rm -f $(NAME)

re: fclean all