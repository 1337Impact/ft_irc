#ifndef IRCSERV_HPP
#define IRCSERV_HPP

#include <climits>
#include <iostream>
#include <map>
#include <netinet/in.h>
#include <set>
#include <sstream>
#include <string>
#include <sys/_types/_size_t.h>
#include <sys/poll.h>
#include <vector>
// #include <limits>

#define BlockingError(func) \
	std::cerr << func << ": " << strerror(EWOULDBLOCK) << std::endl
#define Close(sockfd)        \
	if (close(sockfd) == -1) \
	BlockingError("close")
#define ERR_UNKNOWNCOMMAND(cmd) \
	Message(421).addParam(cmd).addParam(":Unknown command")
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
	Message &addParam(const std::string &prm);
	Message &setCommand(const std::string &cmd);
	Message();
	Message(const int ncmd);
	Message(std::string &msg);
	Message &setPrefix(const User &usr);
};

class User
{
	bool hasSecret;
	const int fd;
	size_t nchannels;
	std::string buf;
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
	size_t limit;
	size_t max_members;
	std::vector<std::string> banMasks;
	std::vector<User *> invited;
	std::set<User *> speakers;
	std::string chTopic;
	std::string key;
	std::string topic;
	int modes;

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

	friend class Server;

  public:
	Channel(const std::string &name, User &usr, std::string key = std::string())
		: name(name), limit(100), key(key),
		  modes(ProtectedTopicMask | NoExternalMessagesMask)
	{
		ChannelMember newMember(usr);
		newMember.is_oper = true;
		usr.nchannels++;
		members.push_back(newMember);
	}

	int join(User &usr, std::string _key)
	{
		if (this->isMember(usr))
		{
			std::cout << "User is already a member" << std::endl;
			return (4);
		}
		if (isInviteOnly() &&
			std::find(invited.begin(), invited.end(), &usr) == invited.end())
			return (1);
		if (!key.empty() && key != _key)
			return (2);
		if (members.size() == limit)
			return (3);
		else
		{
			usr.nchannels++;
			members.push_back(ChannelMember(usr));
			std::vector<User *>::iterator kicked =
				find(invited.begin(), invited.end(), &usr);
			if (kicked != invited.end())
				invited.erase(kicked);
			return (0);
		}
	}

	void broadcast(const std::string &res, const User &skip) const
	{
		std::vector<ChannelMember>::const_iterator it;
		for (it = members.begin(); it != members.end(); it++)
			if (skip.nickname != (*it).usr.nickname)
				if (send(it->usr.fd, res.data(), res.size(), 0) == -1)
					BlockingError("send");
	}

	std::string get_memebers(void)
	{
		std::string membersList;
		std::vector<ChannelMember>::iterator it;
		for (it = members.begin(); it != members.end(); it++)
		{
			if (it->is_oper)
				membersList += '@';
			membersList += it->usr.nickname;
			membersList += ' ';
		}
		return (membersList);
	}

	User *lookUpUser(const std::string &nick)
	{
		for (std::vector<ChannelMember>::iterator it = members.begin();
			 it != members.end();
			 it++)
			if (nick == (*it).usr.nickname)
				return &it->usr;
		return (nullptr);
	}

	bool isMember(const User &user) const
	{
		for (std::vector<ChannelMember>::const_iterator it = members.begin();
			 it != members.end();
			 it++)
			if (user.nickname == it->usr.nickname)
				return true;
		return false;
	}
	void remove(User *user)
	{
		members.erase(std::find(members.begin(), members.end(), *user));
		user->nchannels--;
	}
	std::string getChannelModes() const
	{
		std::string modes;
		return modes;
	}

	bool isOperator(const User &usr) const
	{
		for (std::vector<ChannelMember>::const_iterator it = members.begin();
			 it != members.end();
			 it++)
			if (usr.nickname == it->usr.nickname)
				return it->is_oper;
		return false;
	}

	bool isSpeaker(const User &user) const
	{
		return speakers.end() != find(speakers.begin(), speakers.end(), &user);
	}

	const Message addOperator(const std::string &target, const bool add)
	{
		std::vector<ChannelMember>::iterator mem =
			find(members.begin(), members.end(), target);
		if (mem == members.end())
			return Message(441).addParam(target).addParam(name).addParam(
				":They aren't on that channel");
		std::cout << "param == " << add << std::endl;
		mem->is_oper = add;
		return add
			? Message().setCommand("REPLY").addParam(":Operator has been added")
			: Message().setCommand("REPLY").addParam(":Operator has been removed");
	}

	const Message setChannelLimit(const std::string &limit, const bool add)
	{
		ssize_t val = atoi(limit.data());
		if (!add)
		{
			this->limit = members.size();
			return Message(696).addParam(name).addParam("l").addParam(
				":limit has been removed");
		}
		if (val >= (ssize_t)members.size())
			this->limit = val;
		else
			return Message(696).addParam(name).addParam("l").addParam(limit).addParam(
				":limit is below current members count");
		return Message()
			.setCommand("REPLY")
			.addParam(":Limit has been set to")
			.addParam(limit);
	}

	const Message setBanMask(const std::string &mask, const bool add)
	{
		return add ? banMasks.push_back(mask)
				   : (void)banMasks.erase(
						 find(banMasks.begin(), banMasks.end(), mask)),
			   Message().setCommand("REPLY").addParam(":Ban mask has been added");
	}

	const Message setSpeaker(const std::string &user, const bool add)
	{
		if (add)
		{
			if (User *mem = lookUpUser(user))
				speakers.insert(mem);
			return Message(401).addParam(name).addParam(":No such nick/channel");
		}
		else
		{
			if (User *mem = lookUpUser(user))
				speakers.erase(std::find(speakers.begin(), speakers.end(), mem));
			return Message(401).addParam(name).addParam(":No such nick/channel");
		}
		return Message().setCommand("REPLY").addParam(":Speaker has been added");
	}

	const Message setSecret(const std::string &key, const bool add)
	{
		if (add)
		{
			if (key.empty())
				return Message(535).addParam(name).addParam(
					":Key is not well-formed");
			this->key = key;
		}
		else
			this->key.clear();
		return Message().setCommand("REPLY").addParam(":Secret has been set");
	}
	static bool isValidName(const std::string &name)
	{
		return !name.empty();
	}
};

class Server
{
	const int tcpsock;
	const std::string password;
	sockaddr_in serv;
	std::vector<Channel> channels;
	std::map<const int, User> users;
	std::vector<pollfd> cons;

	~Server();
	bool nickIsUsed(const std::string &nick) const;
	Message pass(User &usr, const Message &req);
	Message user(User &usr, const Message &req);
	Message nick(User &usr, const Message &req);
	Message privmsg(User &usr, const Message &req);
	Message notice(User &usr, const Message &req);
	Message quit(User &usr, const Message &req);

	Message join(User &usr, const Message &req);
	Message list(User &usr, const Message &req);
	Message kick(User &usr, const Message &req);
	Message invite(User &usr, const Message &req);

	Message mode(User &usr, const Message &req);
	Message names(User &usr, const Message &req);
	Message part(User &usr, const Message &req);
	Message topic(User &usr, const Message &req);

	Channel *lookUpChannel(const std::string &chn, const User &seeker);
	Server(const int port, const std::string &name);
	User *lookUpUser(const std::string &nick);
	void process(User &usr, const Message &req);
	void receive(std::vector<pollfd>::const_iterator &con);
	void Send(const Message &res, const User &usr)
	{
		if (!res.command.empty())
		{
			const std::string _tmp = res.totxt();
			if (std::cout << "Sending: " << '"' << _tmp << '"' << std::endl,
				send(usr.fd, _tmp.data(), _tmp.size(), 0) == -1)
				BlockingError("send");
		}
	}
	void sendChannelMemberList(const Channel *chn, User &usr)
	{
		if ((!chn->isPrivate() && !chn->isSecret()) ||
			(chn->isPrivate() && chn->isMember(usr)))
		{
			Message res = Message(353)
							  .addParam(chn->isPrivate()      ? "*"
											: chn->isSecret() ? "@"
															  : "=")
							  .addParam(chn->name);
			for (std::vector<Channel::ChannelMember>::const_iterator it =
					 chn->members.begin();
				 it != chn->members.end();
				 it++)
				res.params.size() == 2 ? res.addParam(':' + it->usr.nickname)
									   : res.addParam(it->usr.nickname);
			Send(res, usr);
		}
	}

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