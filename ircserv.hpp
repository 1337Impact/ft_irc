#ifndef IRCSERV_HPP
#define IRCSERV_HPP

#include "message.hpp"
#include <exception>
#include <map>
#include <netinet/in.h>
#include <set>
#include <string>
#include <sys/poll.h>
#include <vector>

class User
{
	bool hasSecret;
	const int fd;
	std::string hostname;
	std::string nickname;
	std::string realname;
	std::string servername;
	std::string username;

	friend class Server;
	friend struct Message;
	friend class Channel;
	User(const int fd);
	bool isRegistered() const
	{
		return !nickname.empty() && !username.empty();
	};
	static bool validNick(const std::string &nick)
	{
		if (!isalpha(nick[0]))
			return false;
		for (size_t i = 1; i < nick.size(); i++)
			if (!isalnum(nick[i]) && nick[i] != '-' && nick[i] != '|' &&
				nick[i] != '[' && nick[i] != ']' && nick[i] != '\\' &&
				nick[i] != '`' && nick[i] != '^' && nick[i] != '{' &&
				nick[i] != '}')
				return false;
		return true;
	}
};

class Channel
{
	const  std::string name;
	std::set<User *> members;
	public:
	Channel(const std::string &name);
	void join(User *user){
		members.insert(user);
	}
	void broadcast(const std::string &res){
		std::set<User *>::iterator it;
		for (it = members.begin(); it != members.end(); it++)
			send((*it)->fd, res.data(), res.size(), 0);
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
	bool nickIsUsed(const std::string &nick) const;
	const Message nick(User &usr, const Message &req);
	const Message notice(User &usr, const Message &req);
	const Message pass(User &usr, const Message &req);
	const Message privmsg(User &usr, const Message &req);
	const Message user(User &usr, const Message &req);
	Server(const int port, const std::string &name);
	User *lookUpUser(const std::string &nick);
	void process(User &usr, const Message &req);
	void receive(std::vector<pollfd>::const_iterator &con);

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

#define BlockingError(func) \
	(std::cerr << func << ": " << strerror(EWOULDBLOCK) << std::endl)
#endif