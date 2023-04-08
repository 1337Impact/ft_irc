#include "ircserv.hpp"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <sstream>
#include <strings.h>
#include <unistd.h>

User::User(const int fd) : hasSecret(false), fd(fd)
{
}

bool User::isRegistered() const
{
	return !nickname.empty() && !username.empty();
}

bool User::validNick(const std::string &nick)
{
	if (!isalpha(nick[0]))
		return false;
	for (size_t i = 1; i < nick.size(); i++)
		if (!isalnum(nick[i]) && nick[i] != '-' && nick[i] != '[' &&
			nick[i] != ']' && nick[i] != '\\' && nick[i] != '`' &&
			nick[i] != '^' && nick[i] != '{' && nick[i] != '}')
			return false;
	return true;
}

Channel *Server::lookUpChannel(const std::string &name)
{
	for (std::vector<Channel>::iterator chn = channels.begin();
		 chn != channels.end();
		 chn++)
		if (name == chn->name)
			return &*chn;
	return (nullptr);
}

bool Server::nickIsUsed(const std::string &nick) const
{
	for (std::map<const int, User>::const_iterator usr = users.begin();
		 usr != users.end();
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
	Message res;
	if (req.command == "PASS")
		res = pass(usr, req);
	else if (req.command == "USER")
		res = user(usr, req);
	else if (req.command == "NICK")
		res = nick(usr, req);
	else if (req.command == "PRIVMSG")
		res = privmsg(usr, req);
	else if (req.command == "NOTICE")
		res = notice(usr, req);
	else if (req.command == "JOIN")
		res = join(usr, req);
	else if (req.command == "MODE")
		res = mode(usr, req);
	else if (req.command == "INVITE")
		res = invite(usr, req);
	else if (req.command == "KICK")
		res = kick(usr, req);
	else if (req.command == "LIST")
		res = list(usr, req);
	else if (req.command == "NAMES")
		res = names(usr, req);
	else if (req.command == "PART")
		res = part(usr, req);
	else if (req.command == "QUIT")
		res = quit(usr, req);
	else if (req.command == "TOPIC")
		res = topic(usr, req);
	else if (!req.command.empty())
		res = Message(421).addParam(req.command).addParam(":Unknown command");
	Send(res, usr);
}

Server::~Server()
{
	for (std::vector<pollfd>::const_iterator con = cons.begin();
		 con != cons.end();
		 con++)
		Close(con->fd);
	std::cout << "Closing all left connections" << std::endl;
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
	else if (usr.buf.size() + nbytes > 512)
	{
		usr.buf.clear();
		Send(Message(417).addParam(":Input line was too long"), usr);
	}
	else if (usr.buf.append(buf), usr.buf.size() >= 2)
		while (true)
		{
			size_t end = usr.buf.find("\r\n");
			if (end == std::string::npos)
			{
				if (!usr.buf.empty())
					std::cout << "[incomplete message] => " << '"' << usr.buf
							  << '"' << std::endl;
				break;
			}
			std::string rest(usr.buf.begin() + end + 2,
							 usr.buf.begin() + usr.buf.size());
			usr.buf.resize(end);
			std::cout << "[complete message] => " << '"' << usr.buf << '"'
					  << std::endl;
			try
			{
				const Message req = Message(usr.buf).setPrefix(usr);
				std::cout << "[prefix]: " << req.prefix << std::endl;
				std::cout << "[command]: " << req.command << std::endl;
				for (unsigned i = 0; i < req.params.size(); i++)
					std::cout << "[param]: " << req.params[i] << std::endl;
				req.command == "QUIT" ? (quit(usr, req), (void)cons.erase(con))
									  : process(usr, req);
			}
			catch (const char *err)
			{
				std::cerr << "Message parsing error: " << err << std::endl;
			}
			usr.buf = rest;
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
				std::cout << "New accepted connection" << std::endl;
			}
			evts--;
		}
		for (std::vector<pollfd>::const_iterator con = cons.begin() + 1;
			 evts && con != cons.end();
			 con++)
			if (con->revents & POLLIN)
			{
				evts--;
				receive(con);
			}
	}
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