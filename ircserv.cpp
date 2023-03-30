#include "ircserv.hpp"
#include "message.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>

bool Server::nickIsUsed(const std::string &nick) const
{
	for (std::map<const int, User>::const_iterator usr = users.cbegin();
		 usr != users.cend();
		 usr++)
		if (nick == usr->second.nickname)
			return true;
	return false;
}

User *Server::lookUpUser(const std::string &nick)
{
	for (std::map<const int, User>::iterator usr = users.begin();
		 usr != users.end();
		 usr++)
		if (nick == usr->second.nickname)
			return &usr->second;
	return (nullptr);
}

Server &Server::getInstance(const int port, const std::string &pass)
{
	static Server instance(port, pass);
	return instance;
}

void Server::process(User &usr, const Message &req)
{
	std::string res;
	if (req.command == "PASS")
		res = pass(usr, req).totxt();
	else if (req.command == "USER")
		res = user(usr, req).totxt();
	else if (req.command == "NICK")
		res = nick(usr, req).totxt();
	else if (req.command == "PRIVMSG")
		res = privmsg(usr, req).totxt();
	else if (req.command == "NOTICE")
		res = notice(usr, req).totxt();
	else
		res =
			Message(421).addParam(req.command).addParam(":Unknown command").totxt();
	if (!res.empty() && send(usr.fd, res.data(), res.size(), 0) == -1)
		BlockingError("send");
}

Server::~Server()
{
	for (std::vector<pollfd>::const_iterator con = cons.cbegin();
		 con != cons.cend();
		 con++)
		if (close(con->fd) == -1)
			BlockingError("close");
}

Server::Server(const int port, const std::string &pass)
	: tcpsock(socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)), password(pass)
{
	if (tcpsock == -1)
		throw SystemException("socket");
	if (fcntl(tcpsock, F_SETFL, O_NONBLOCK) == -1)
		throw SystemException("fcntl");
	serv.sin_family = AF_INET;
	serv.sin_addr.s_addr = htonl(INADDR_ANY);
	serv.sin_port = htons(port);
	if (bind(tcpsock, (sockaddr *)&serv, sizeof(serv)) == -1)
		throw SystemException("bind");
	if (listen(tcpsock, 1) == -1)
		throw SystemException("listen");
	std::cout << "Listening on localhost:" << port << std::endl;
	cons.push_back((pollfd){.fd = tcpsock, .events = POLLIN});
}

void Server::receive(std::vector<pollfd>::const_iterator &con)
{
	char buf[512] = "";
	int nbytes = recv(con->fd, buf, sizeof(buf), MSG_DONTWAIT);
	if (nbytes <= 0)
	{
		if (nbytes == -1)
			BlockingError("recv");
		if (close(con->fd) == -1)
			BlockingError("close");
		cons.erase(con, con + 1);
		bufs.erase(con->fd);
	}
	else
	{
		bufs[con->fd].append(buf);
		if (nbytes > 1)
		{
			if (!bufs[con->fd].compare(nbytes - 2, 2, "\r\n"))
			{
				User &usr = users.at(con->fd);
				process(usr, Message(bufs[con->fd]).addPrefix(usr));
				bufs.erase(con->fd);
			}
			else if (nbytes == 512)
			{
				// segfault on this one
				std::cerr << "Message is too large" << std::endl;
				bufs.erase(con->fd);
			}
		}
	}
}

void Server::eventloop()
{
	sockaddr_storage cdata;
	unsigned size = sizeof(cdata);
	pollfd con;
	con.events = POLLIN;
	while (int evts = poll(cons.data(), cons.size(), -1))
	{
		if (evts == -1)
			throw SystemException("poll");
		if (cons.front().revents & POLLIN)
		{
			con.fd = accept(tcpsock, (sockaddr *)&cdata, &size);
			if (con.fd < 0)
				BlockingError("accept");
			else
			{
				cons.push_back(con);
				users.insert(std::pair<const int, User>(con.fd, User(con.fd)));
				std::cout << "new accepted connection" << std::endl;
			}
			evts--;
		}
		for (std::vector<pollfd>::const_iterator con = cons.cbegin() + 1;
			 evts && con != cons.cend();
			 con++)
			if (con->revents & POLLIN)
			{
				evts--;
				receive(con);
			}
	}
}