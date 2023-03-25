#include "ircserv.hpp"

int main(int argc, char *argv[])
{
	try
	{
		if (argc != 3)
			throw std::invalid_argument("usage: ./ircserv <port> <password>");
		Server irc(std::atoi(argv[1]), argv[2]);
		irc.event_loop();
	}
	catch (const std::exception &exp)
	{
		std::cout << exp.what() << std::endl;
	}
}