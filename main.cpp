#include "ircserv.hpp"

User::User(const int fd) : hasSecret(false), fd(fd)
{
}

int main(int argc, char *argv[])
{
	try
	{
		if (argc != 3)
			throw std::invalid_argument("usage: ./ircserv <port> <password>");
		Server::getInstance(std::atoi(argv[1]), argv[2]).eventloop();
	}
	catch (const std::exception &exp)
	{
		std::cerr << exp.what() << std::endl;
	}
}