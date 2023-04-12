#ifndef IRCSERV_HPP
#define IRCSERV_HPP
#include <iostream>
#include <map>
#include <netinet/in.h>
#include <set>
#include <string>
#include <sys/_types/_size_t.h>
#include <sys/poll.h>
#include <vector>

#define BlockingError(func) \
	std::cerr << func << ": " << strerror(EWOULDBLOCK) << std::endl
#define Close(sockfd)        \
	if (close(sockfd) == -1) \
	BlockingError("close")
#define ERR_UNKNOWNCOMMAND(cmd) \
	Message(421).addParam(cmd).addParam("Unknown command")
#define QUIT(usr)                                         \
	Message().setPrefix(usr).setCommand("QUIT").addParam( \
		":User quit unexpectedly")

#define NoExternalMessagesMask 1 << 0
#define InviteOnlyMask 1 << 1
#define ModeratedMask 1 << 2
#define PrivateMask 1 << 3
#define ProtectedTopicMask 1 << 4
#define SecretMask 1 << 5

#define SecretKeyMask 1 << 6
#define SpeakerMask 1 << 7
#define BanMask 1 << 8
#define ChannelLimitMask 1 << 9
#define OperatorMask 1 << 10

class User;
struct Message
{
	std::string command;
	std::string prefix;
	std::string totxt() const;
	std::vector<std::string> params;

	friend class Server;
	friend class Bot;
	Message &addParam(const std::string &prm);
	Message &setCommand(const std::string &cmd);
	Message();
	Message(const int ncmd);
	Message(std::string &msg);
	Message &setPrefix(const User &usr);
};

class User
{
  protected:
	bool hasSecret;
	const int fd;
	size_t nchannels;
	std::string buf;
	std::string hostname;
	std::string nickname;
	std::string realname;
	std::string servername;
	std::string username;
	size_t initTime;

	friend class Server;
	friend struct Message;
	friend class Channel;
	User(const int fd);
	bool isRegistered() const;
	static bool validNick(const std::string &nick);

  public:
	std::string getNickName(void) const
	{
		return this->nickname;
	}
	std::string getLogTime()
	{
		size_t logtime = time(NULL) - this->initTime;
		return std::to_string(logtime / 60) + " minuts and " +
			std::to_string(logtime % 60) + " seconds";
	}
};

class Channel
{
	friend class Server;

	const std::string name;
	int modes;
	size_t limit;
	size_t max_members;
	std::set<User *> speakers;
	std::string chTopic;
	std::string key;
	std::string topic;
	std::vector<std::string> banMasks;
	std::vector<User *> invited;

	struct ChannelMember
	{
		User &usr;
		bool is_oper;
		bool is_banned;
		ChannelMember(User &usr) : usr(usr), is_oper(false)
		{
		}
	};
	std::vector<ChannelMember> members;

  public:
	static unsigned FlagToMask[CHAR_MAX];

  private:
	bool hasNoExternalMessages() const
	{
		return modes & NoExternalMessagesMask;
	}
	bool hasProtectedTopic() const
	{
		return modes & ProtectedTopicMask;
	}
	bool isInviteOnly() const
	{
		return modes & InviteOnlyMask;
	}
	bool hasSecretKey() const
	{
		return modes & SecretKeyMask;
	}
	bool hasLimit() const
	{
		return modes & ChannelLimitMask;
	}
	bool isModerated() const
	{
		return modes & ModeratedMask;
	}
	bool isPrivate() const
	{
		return modes & PrivateMask;
	}
	bool isSecret() const
	{
		return modes & SecretMask;
	}
	friend bool operator==(const ChannelMember &mem, const std::string &nickname)
	{
		return mem.usr.nickname == nickname;
	}

	friend bool operator==(const ChannelMember &mem, const User &user)
	{
		return mem.usr.nickname == user.nickname;
	}

	friend bool operator==(const Channel &chn, const std::string &name)
	{
		return name == chn.name;
	}

  public:
	Channel(const std::string &name, User &usr, std::string key = std::string());
	~Channel(){}

	int join(User &usr, std::string _key);
	std::string get_memebers(void);
	void remove(User *user);
	User *lookUpUser(const std::string &nick);
	std::string getChannelModes() const;
	const Channel &broadcast(const std::string &res, const User &skip) const;

	bool isMember(const User &user) const;
	bool isOperator(const User &usr) const;
	bool isSpeaker(const User &user) const;
	static bool isValidName(const std::string &name);

	const Message addOperator(const std::string &target, const bool add);
	const Message setChannelLimit(const std::string &limit, const bool add);
	const Message setBanMask(const std::string &mask, const bool add);
	const Message setSpeaker(const std::string &user, const bool add);
	const Message setSecret(const std::string &key, const bool add);
};

class Bot : public User
{
  private:
	Message Hello(User &usr)
	{
		return Message().setPrefix(*this).setCommand("PRIVMSG").addParam(
			"Hi " + usr.getNickName() + "! How can I assist you today?");
	}
	Message Logtime(User &usr)
	{
		return Message().setPrefix(*this).setCommand("PRIVMSG").addParam(
			"Your Logtime is: " + usr.getLogTime());
	}

  public:
	Bot() : User(0)
	{
		nickname = "Emet";
		username = "Emet";
		hostname = "irc.42.com";
		servername = "irc";
		realname = "Emet Bot";
	}
	Message botTalk(User &usr, std::string msg)
	{
		std::cout << "botTalk" << std::endl;
		if (msg == "hello")
			return Hello(usr);
		if (msg == "logtime")
			return Logtime(usr);
		return Message();
	}
};

class Server
{
	const int tcpsock;
	const std::string password;
	const std::string name;
	sockaddr_in serv;
	std::map<const int, User> users;
	std::vector<Channel> channels;
	std::vector<pollfd> cons;
	Bot bot;

	bool nickIsUsed(const std::string &nick) const;
	Channel *lookUpChannel(const std::string &chn);
	Message invite(User &usr, const Message &req);
	Message join(User &usr, const Message &req);
	Message kick(User &usr, const Message &req);
	Message list(User &usr, const Message &req);
	Message mode(User &usr, const Message &req);
	Message names(User &usr, const Message &req);
	Message nick(User &usr, const Message &req);
	Message part(User &usr, const Message &req);
	Message pass(User &usr, const Message &req);
	Message privmsg(User &usr, const Message &req);
	Message quit(User &usr, const Message &req);
	Message topic(User &usr, const Message &req);
	Message user(User &usr, const Message &req);
	Server(const int port, const std::string &name);
	~Server();
	User *lookUpUser(const std::string &nick);
	void process(User &usr, const Message &req);
	void receive(std::vector<pollfd>::const_iterator &con);
	void Send(const Message &res, const User &usr);
	void sendChannelMemberList(const Channel *chn, User &usr);

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