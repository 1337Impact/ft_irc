#ifndef IRCSERV_HPP
#define IRCSERV_HPP

#include "message.hpp"
#include <exception>
#include <map>
#include <netinet/in.h>
#include <string>
#include <sys/poll.h>
#include <vector>

class User
{
	bool hasSecret;
	std::string hostname;
	std::string nickname;
	std::string realname;
	std::string servername;
	std::string username;

	friend class Server;
	User();
	bool isRegistered() const
	{
		return !nickname.empty() && !username.empty();
	};
	static bool validNick(const std::string &nick)
	{
		return nick.empty(), true;
	}
};

class Server
{
	const int tcpsock;
	const std::string password;
	sockaddr_in serv;
	std::map<const int, std::string> bufs;
	std::map<const int, User> users;
	std::vector<pollfd> cons;

	~Server();
	const Message nick(User &usr, const std::vector<std::string> &prms);
	const Message pass(User &usr, const std::vector<std::string> &prms);
	const Message user(User &usr, const std::vector<std::string> &prms);
	Server(const int port, const std::string &name);
	void process(const int sockfd, const Message &msg);
	void receive(std::vector<pollfd>::const_iterator &con);
	bool nickIsUsed(const std::string &nick) const;

  public:
	static Server &getInstance(const int port, const std::string &pass);
	void eventloop();
};

class SystemException : public std::system_error
{
  public:
	SystemException(const char *err)
		: system_error(std::error_code(errno, std::system_category()), err)
	{
	}
};

#endif