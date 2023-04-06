#ifndef IRCSERV_HPP
#define IRCSERV_HPP

#include <iostream>
#include <list>
#include <map>
#include <netinet/in.h>
#include <set>
#include <sys/poll.h>
#include <vector>

#define BlockingError(func) \
	std::cerr << func << ": " << strerror(EWOULDBLOCK) << std::endl
#define Send(a, b, c)           \
	if (send(a, b, c, 0) == -1) \
	BlockingError("send")
#define Close(sockfd)        \
	if (close(sockfd) == -1) \
	BlockingError("close")
#define ERR_UNKNOWNCOMMAND(cmd) \
	Message(421).addParam(cmd).addParam(":Unknown command")
#define QUIT(usr)                                         \
	Message().setPrefix(usr).setCommand("QUIT").addParam( \
		":User quit unexpectedly")

class User;
struct Message
{
	std::string command;
	std::string prefix;
	std::string totxt() const;
	std::vector<std::string> params;

	friend class Server;
	Message &addParam(const std::string &prm);
	Message &setCommand(const std::string &cmd);
	Message();
	Message(const int fd);
	Message(std::string &msg);
	Message &setPrefix(const User &usr);
};

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
	bool isRegistered() const;
	static bool validNick(const std::string &nick);
};

class Channel
{
	friend class Server;
	const std::string name;
	int limit;
	std::string key;
	std::string topic;
	std::set<User *> members;
	bool priv;
	bool secret;
	bool inviteOnly;
	bool protectedTopic;
	bool externalMessages;
	bool moderated;

	bool isMember(User *user) const;
	bool isOperator(const User &usr) const;
	Channel(const std::string &name, User *usr);
	const Message addOperator(const std::string &target, const bool give);
	const Message setBanMask(const std::string &mask);
	const Message setClientLimit(const std::string &limit);
	const Message setKey(const std::string &key);
	const Message setSpeaker(const std::string &user);
	int join(User *usr);
	std::string getChannelModes() const;
	User *lookUpUser(const std::string &nick);
	void broadcast(const std::string &res);
	void disjoin(User *user);
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
	const Message list(User &usr, const Message &req);
	const Message mode(User &usr, const Message &req);
	const Message names(User &usr, const Message &req);
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