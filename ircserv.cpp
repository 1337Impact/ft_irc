#include "ircserv.hpp"
#include "message.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <sstream>
#include <unistd.h>

Channel *Server::lookUpChannel(const std::string &name)
{
	for (std::list<Channel>::iterator chn = channels.begin();
		 chn != channels.end();
		 chn++)
		if (name == chn->name)
			return &*chn;
	return (nullptr);
}

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
	const std::string res = req.command == "PASS" ? pass(usr, req).totxt()
		// : req.command == "INVITE"                 ? invite(usr, req).totxt()
		// : req.command == "JOIN"                   ? join(usr, req).totxt()
		// : req.command == "KICK"                   ? kick(usr, req).totxt()
		: req.command == "MODE"                   ? mode(usr, req).totxt()
		: req.command == "NICK"                   ? nick(usr, req).totxt()
		: req.command == "NOTICE"                 ? notice(usr, req).totxt()
		: req.command == "PART"                   ? part(usr, req).totxt()
		: req.command == "PRIVMSG"                ? privmsg(usr, req).totxt()
		// : req.command == "TOPIC"                  ? topic(usr, req).totxt()
		: req.command == "USER"                   ? user(usr, req).totxt()
								: ERR_UNKNOWNCOMMAND(req.command).totxt();
	if (!res.empty())
		Send(usr.fd, res.data(), res.size());
}

Server::~Server()
{
	for (std::vector<pollfd>::const_iterator con = cons.cbegin();
		 con != cons.cend();
		 con++)
		Close(con->fd);
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
	User &usr = users.at(con->fd);
	if (nbytes <= 0)
	{
		if (nbytes == -1)
			BlockingError("recv");
		quit(usr, QUIT(usr));
		cons.erase(con);
	}
	else if (usr.buf.append(buf).size() >= 512)
	{
		std::cerr << "Message is too large" << std::endl;
		usr.buf.clear();
	}
	else if (!usr.buf.compare(nbytes - 2, 2, "\r\n"))
	{
		try
		{
			const Message req = Message(usr.buf).setPrefix(usr);
			req.command == "QUIT" ? (quit(usr, req), (void)cons.erase(con))
								  : process(usr, req);
		}
		catch (const char *err)
		{
			std::cerr << err << std::endl;
		}
		usr.buf.clear();
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