#ifndef IRCSERV_HPP
#define IRCSERV_HPP

#include <iostream>
#define BlockingError(func) \
	std::cerr << func << ": " << strerror(EWOULDBLOCK) << std::endl
#define Send(a, b, c)           \
	if (send(a, b, c, 0) == -1) \
	BlockingError("send")
#define Close(sockfd)        \
	if (close(sockfd) == -1) \
	BlockingError("close")

#include "message.hpp"
#include <algorithm>
#include <exception>
#include <list>
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
	std::string buf;
	bool is_oper;
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
	friend class Server;
	const std::string name;
	int limit;
	std::string key;
	std::set<User *> members;
	bool privateMode;
	bool secretMode;
	bool inviteOnlyMode;
	bool protectedTopicMode;
	bool externalMessagesMode;
	bool moderatedMode;

  public:
	Channel(const std::string &name, User *usr)
		: name(name), limit(100), privateMode(false), secretMode(false),
		  inviteOnlyMode(false), protectedTopicMode(false),
		  externalMessagesMode(false), moderatedMode(false)
	{
		members.insert(usr);
	}
	int join(User *usr)
	{
		members.insert(usr);
		return (0);
	}
	void broadcast(const std::string &res)
	{
		std::set<User *>::iterator it;
		for (it = members.begin(); it != members.end(); it++)
			Send((*it)->fd, res.data(), res.size());
	}

	User *lookUpUser(const std::string &nick)
	{
		for (std::set<User *>::iterator usr = members.begin();
			 usr != members.end();
			 usr++)
			if (nick == (*usr)->nickname)
				return *usr;
		return (nullptr);
	}
	void disjoin(User *user)
	{
		members.erase(std::find(members.cbegin(), members.cend(), user));
	}
	bool isMember(User *user)
	{
		return members.cend() ==
			std::find(members.cbegin(), members.cend(), user);
	}
	std::string getChannelModes() const
	{
		std::string modes;
		return modes;
	}
	bool isOperator(const User &usr) const
	{
		for (std::set<User *>::const_iterator mem = members.cbegin();
			 mem != members.cend();
			 mem++)
			if (usr.username == (*mem)->username)
				return true;
		// this commit do not have ChannelModes struct yet
		return false;
	}
	const Message addOperator(const std::string &target, const bool give)
	{
		User *usr = lookUpUser(target);
		if (!usr)
			return Message(441).addParam(target).addParam(name).addParam(":They aren't on that channel");
		(void)give;
		return Message();
	}
	const Message setClientLimit(const std::string &limit)
	{
		(void)limit;
		return Message();
	}
	const Message setBanMask(const std::string &mask)
	{
		(void)mask;
		return Message();
	}
	const Message setSpeaker(const std::string &user)
	{
		(void)user;
		return Message();
	}
	const Message setKey(const std::string &key)
	{
		(void)key;
		return Message();
	}
};

class Server
{
	const int tcpsock;
	const std::string password;
	sockaddr_in serv;
	std::list<Channel> channels;
	std::map<const int, User> users;
	std::vector<pollfd> cons;

	~Server();
	bool nickIsUsed(const std::string &nick) const;
	const Message invite(User &usr, const Message &req);
	const Message kick(User &usr, const Message &req);
	const Message mode(User &usr, const Message &req);
	const Message nick(User &usr, const Message &req);
	const Message notice(User &usr, const Message &req);
	const Message part(User &usr, const Message &req);
	const Message pass(User &usr, const Message &req);
	const Message privmsg(User &usr, const Message &req);
	const Message quit(User &usr, const Message &req);
	const Message topic(User &usr, const Message &req);
	const Message user(User &usr, const Message &req);
	Channel *lookUpChannel(const std::string &chn);
	const Message join(User &usr, const Message &req);
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
#endif