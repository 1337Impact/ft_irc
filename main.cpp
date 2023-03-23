#include "main.hpp"
#include <iostream>
#include <sys/fcntl.h>
#include <sys/poll.h>
#include <unistd.h>

Server::~Server()
{
	for (std::vector<pollfd>::const_iterator con = cons.cbegin();
		 con != cons.cend();
		 con++)
		if (close(con->fd) == -1)
			perror("close");
}

Server::Server(const int port, const std::string &pass)
	: sockfd(socket(AF_INET, SOCK_STREAM, 0)), pass(pass)
{
	if (sockfd == -1)
		throw std::system_error(std::error_code(errno, std::system_category()),
								"socket");
	if (fcntl(sockfd, F_SETFL, O_NONBLOCK) == -1)
		throw std::system_error(std::error_code(errno, std::system_category()),
								"fcntl");
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	if (bind(sockfd, (sockaddr *)&addr, sizeof(addr)) == -1)
		throw std::system_error(std::error_code(errno, std::system_category()),
								"bind");
	if (listen(sockfd, 1) == -1)
		throw std::system_error(std::error_code(errno, std::system_category()),
								"listen");
	cons.push_back((pollfd){.fd = sockfd, .events = POLLIN});
}

void Server::read_sock(std::vector<pollfd>::const_iterator &con)
{
	char buf[513];
	bzero(buf, sizeof(buf));
	int nbytes = recv(con->fd, buf, sizeof(buf), 0);
	if (nbytes <= 0 || nbytes == 513)
	{
		if (nbytes == -1)
			perror("recv");
		if (nbytes == 513)
			std::cerr << "message too long, connection is be closed" << std::endl;
		if (close(con->fd) == -1)
			perror("close");
		cons.erase(con, con + 1);
		bufs.erase(con->fd);
	}
	else
	{
		bufs[con->fd].append(buf);
		if (nbytes > 1 && buf[nbytes - 1] == '\n' && buf[nbytes - 2] == '\r')
		{
			for (std::vector<pollfd>::const_iterator oth = cons.cbegin() + 1;
				 oth != cons.cend();
				 oth++)
				if (oth->fd != con->fd)
					send(oth->fd, bufs[con->fd].data(), bufs[con->fd].size(), 0);
			bufs.erase(con->fd);
		}
	}
}

void Server::event_loop()
{
	while (int evts = poll(cons.data(), cons.size(), -1))
	{
		if (evts == -1)
		{
			perror("poll");
			throw std::system_error(
				std::error_code(errno, std::system_category()), "poll");
		}
		if (cons.front().revents & POLLIN)
		{
			unsigned addr_size = sizeof(oth_addr);
			pollfd con = {.fd = accept(sockfd, (sockaddr *)&oth_addr, &addr_size),
						  .events = POLLIN};
			if (con.fd == -1)
				perror("accept");
			else
			{
				cons.push_back(con);
				std::cout << "new connection has been set" << std::endl;
			}
			evts--;
		}
		for (std::vector<pollfd>::const_iterator con = cons.cbegin() + 1;
			 evts && con != cons.cend();
			 con++)
			if (con->revents & POLLIN)
			{
				evts--;
				read_sock(con);
			}
	}
}

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