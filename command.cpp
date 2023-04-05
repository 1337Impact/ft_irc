#include "ircserv.hpp"
#include "message.hpp"
#include <unistd.h>

const Message Server::part(User &usr, const Message &req)
{
	if (!usr.hasSecret)
		return Message(464).addParam(":Password incorrect");
	if (!usr.isRegistered())
		return Message(451).addParam(":You have not registered");
	for (unsigned chn = 0; chn < req.params.size(); chn++)
	{
		if (Channel *channel = lookUpChannel(req.params[chn]))
		{
			if (channel->isMember(&usr))
				channel->disjoint(&usr);
			else
			{
				const std::string txt =
					Message(442)
						.addParam(req.params[chn])
						.addParam(":You're not on that channel")
						.totxt();
				Send(usr.fd, txt.data(), txt.size());
			}
		}
		else
		{
			const std::string txt = Message(403)
										.addParam(req.params[chn])
										.addParam(":No such channel")
										.totxt();
			Send(usr.fd, txt.data(), txt.size());
		}
	}
	return Message();
}

const Message Server::quit(User &usr, const Message &req)
{
	const std::string txt = req.totxt();
	Close(usr.fd);
	for (std::map<const int, User>::const_iterator oth = users.cbegin();
		 oth != users.cend();
		 oth++)
		if (usr.fd != oth->first)
			Send(oth->second.fd, txt.data(), txt.size());
	users.erase(usr.fd);
	return Message();
}

const Message Server::pass(User &usr, const Message &req)
{
	if (usr.hasSecret)
		return Message();
	if (req.params.empty())
	{
		return Message(461).addParam("PASS").addParam(":Not enough parameters");
	}
	if (usr.isRegistered())
		return Message(462).addParam(":You may not reregister");
	if (password != req.params[0])
		return Message(464).addParam(":Password incorrect");
	usr.hasSecret = true;
	return Message();
}

const Message Server::user(User &usr, const Message &req)
{
	if (req.params.size() < 4)
		return Message(461).addParam("USER").addParam(":Not enough parameters");
	if (usr.isRegistered())
		return Message(462).addParam(":You may not reregister");
	if (!usr.hasSecret)
		return Message(464).addParam(":Password incorrect");
	bool registered = usr.isRegistered();
	usr.username = req.params[0];
	usr.hostname = req.params[1];
	usr.servername = req.params[2];
	usr.realname = req.params[3];
	return !registered && usr.isRegistered()
		? Message(1).setPrefix(usr).addParam(":Welcome to the ft_irc Network")
		: Message();
}

const Message Server::nick(User &usr, const Message &req)
{
	if (!usr.hasSecret)
		return Message(464).addParam(":Password incorrect");
	if (req.params.size() < 1)
		return Message(431).addParam(":No nickname given");
	if (!User::validNick(req.params[0]))
		return Message(432).addParam(req.params[0]).addParam(":Erroneus nickname");
	if (nickIsUsed(req.params[0]))
		return Message(433)
			.addParam(req.params[0])
			.addParam(":Nickname is already in use");
	bool registered = usr.isRegistered();
	usr.nickname = req.params[0];
	return !registered && usr.isRegistered()
		? Message(1).setPrefix(usr).addParam(":Welcome to the ft_irc Network")
		: Message();
}

const Message Server::privmsg(User &usr, const Message &req)
{
	if (!usr.hasSecret)
		return Message(464).addParam(":Password incorrect");
	if (!usr.isRegistered())
		return Message(451).addParam(":You have not registered");
	if (req.params.size() < 1)
		return Message(411).addParam(":No recipient given").addParam("(PRIVMSG)");
	if (req.params.size() < 2)
		return Message(412).addParam(":No text to send");
	std::list<Channel *> channels;
	std::vector<User *> users;
	for (unsigned target = 0; target < req.params.size() - 1; target++)
	{
		if (User *user = lookUpUser(req.params[target]))
		{
			for (unsigned rec = 0; rec < users.size(); rec++)
				if (users[rec]->nickname == user->nickname)
					return Message(407)
						.addParam(req.params[target])
						.addParam(":Duplicate recipients. No message delivered");
			users.push_back(user);
			const std::string txt = req.totxt();
			for (unsigned rec = 0; rec < users.size(); rec++)
				Send(users[rec]->fd, txt.data(), txt.size());
		}
		else if (Channel *chn = lookUpChannel(req.params[target]))
		{
			for (std::list<Channel *>::const_iterator rec = channels.cbegin();
				 rec != channels.end();
				 rec++)
				if ((*rec)->name == chn->name)
					return Message(407)
						.addParam(req.params[target])
						.addParam(":Duplicate recipients. No message delivered");
			channels.push_back(chn);
			chn->broadcast(req.totxt());
		}
		else
			return Message(401)
				.addParam(req.params[target])
				.addParam(":No such nick/channel");
	}
	return Message();
}

const Message Server::notice(User &usr, const Message &req)
{
	return privmsg(usr, req), Message();
}