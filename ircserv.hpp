#ifndef IRCSERV_HPP
#define IRCSERV_HPP

#include "message.hpp"
#include <algorithm>
#include <exception>
#include <list>
#include <map>
#include <netinet/in.h>
#include <set>
#include <sstream>
#include <string>
#include <sys/_types/_size_t.h>
#include <sys/poll.h>
#include <vector>

class User
{
	bool hasSecret;
	const int fd;
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
	const std::string name;
	const std::string key;
	bool is_invite_only;
	std::string chTopic;
	size_t max_members;

	std::list<User *> invited;
	struct ChannelMember
	{
		User &usr;
		bool is_oper;
		bool is_banned;
		ChannelMember(User &usr) : usr(usr), is_oper(false)
		{
		}
	};
	std::set<ChannelMember> members;

	friend class Server;

  public:
	Channel(const std::string &name, User &usr, std::string key = std::string())
		: name(name), key(key), max_members(100)
	{
		ChannelMember newMember(usr);
		newMember.is_oper = true;
		members.insert(newMember);
	}

	int join(User &usr, std::string _key)
	{
		if (is_invite_only &&
			std::find(invited.begin(), invited.end(), &usr) == invited.end())
			return (1);
		if (!key.empty() && key != _key) //still need to no if key is ignored when it is not set in channel
			  return (2);
		if (members.size() == max_members)
			return (3);
		else
		{
			members.insert(ChannelMember(usr));
			return (0);
		}
	}

	void broadcast(const std::string &res)
	{
		std::set<ChannelMember>::iterator it;
		for (it = members.begin(); it != members.end(); it++)
			send(it->usr.fd, res.data(), res.size(), 0);
	}

	std::string get_memebers(void)
	{
		std::string membersList;
		std::set<ChannelMember>::iterator it;
		for (it = members.begin(); it != members.end(); it++)
		{
			if (it->is_oper)
				membersList += '@';
			membersList += it->usr.nickname;
			membersList += ' ';
		}
		return (membersList);
	}
};

class Server
{
	const int tcpsock;
	const std::string password;
	sockaddr_in serv;
	std::list<Channel> channels;
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

#define BlockingError(func) \
	(std::cerr << func << ": " << strerror(EWOULDBLOCK) << std::endl)
#endif