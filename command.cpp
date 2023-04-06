#include "ircserv.hpp"
#include <sstream>
#include <string>
#include <unistd.h>

const Message Server::kick(User &usr, const Message &req)
{
	if (!usr.hasSecret)
		return Message(464).addParam(":Password incorrect");
	if (!usr.isRegistered())
		return Message(451).addParam(":You have not registered");
	Channel *chn = lookUpChannel(req.params[0]);
	if (!chn)
		return Message(403).addParam(req.params[0]).addParam(":No such channel");
	if (!chn->isMember(&usr))
		return Message(442)
			.addParam(req.params[0])
			.addParam(":You're not on that channel");
	if (!chn->isOperator(usr))
		return Message(482)
			.addParam(req.params[0])
			.addParam(":You're not channel operator");
	User *mem = lookUpUser(req.params[0]);
	if (!chn->isMember(mem))
		return Message(441)
			.addParam(req.params[1])
			.addParam(req.params[0])
			.addParam(":They aren't on that channel");
	return Message();
}

const Message Server::names(User &usr, const Message &req)
{
	if (!usr.hasSecret)
		return Message(464).addParam(":Password incorrect");
	if (!usr.isRegistered())
		return Message(451).addParam(":You have not registered");
	std::istringstream chnls(req.params[0]);
	std::string name;
	while (getline(chnls, name, ','))
	{
		Channel *chn = lookUpChannel(name);
		if (!chn)
			return Message(403).addParam(name).addParam(":No such channel");
		if (chn->secret && !chn->isMember(&usr))
			return Message();
		return Message(353)
			.addParam(chn->priv         ? "*"
						  : chn->secret ? "@"
										: "=")
			.addParam(":Me");
	}
	return Message(366).addParam(req.params[0]).addParam(":End of /NAMES list");
}

const Message Server::list(User &usr, const Message &req)
{
	if (!usr.hasSecret)
		return Message(464).addParam(":Password incorrect");
	if (!usr.isRegistered())
		return Message(451).addParam(":You have not registered");
	std::string rpl =
		Message(321).addParam("Channel").addParam(":Users  Name").totxt();
	Send(usr.fd, rpl.data(), rpl.size());
	std::istringstream chnls(req.params[0]);
	std::string name;
	while (getline(chnls, name, ','))
	{
		Channel *chn = lookUpChannel(name);
		if ((chn->priv || chn->secret) && !chn->isMember(&usr))
		{
			rpl = Message(322)
					  .addParam(chn->name)
					  .addParam(std::to_string(chn->members.size()))
					  .addParam(chn->topic)
					  .totxt();
			Send(usr.fd, rpl.data(), rpl.size());
		}
	}
	return Message(323).addParam(":End of /LIST");
}

const Message Server::invite(User &usr, const Message &req)
{
	if (!usr.hasSecret)
		return Message(464).addParam(":Password incorrect");
	if (!usr.isRegistered())
		return Message(451).addParam(":You have not registered");
	if (req.params.size() < 2)
		return Message(461).addParam("INVITE").addParam(":Not enough parameters");
	Channel *chn = nullptr;
	if (!chn)
		return Message(403).addParam(req.params[0]).addParam(":No such channel");
	if (!chn->isMember(&usr))
		return Message(442)
			.addParam(req.params[0])
			.addParam(":You're not on that channel");
	if (!chn->isOperator(usr))
		return Message(482)
			.addParam(req.params[0])
			.addParam(":You're not channel operator");
	User *mem = lookUpUser(req.params[0]);
	if (!mem)
		return Message(401).addParam(":No such nick/channel");
	if (chn->isMember(mem))
		return Message(443).addParam(req.params[0]).addParam(":is already on channel");
	chn->join(mem);
	return Message(341).addParam(req.params[0]).addParam(req.params[1]);
}

const Message Server::mode(User &usr, const Message &req)
{
	if (!usr.hasSecret)
		return Message(464).addParam(":Password incorrect");
	if (!usr.isRegistered())
		return Message(451).addParam(":You have not registered");
	Channel *chn = lookUpChannel(req.params[0]);
	if (!chn)
		return Message(403).addParam(req.params[0]).addParam(":No such channel");
	if (!chn->isOperator(usr))
		return Message(482).addParam(chn->name).addParam(
			":You're not channel operator");
	if (req.params.empty())
		return Message(324).addParam(chn->name).addParam(chn->getChannelModes());
	if (req.params[0].size() <= 2 &&
		(req.params[0][0] == '+' || req.params[0][0] == '-'))
	{
		switch (req.params[0][1])
		{
		case 'p':
			return chn->secret ?: chn->priv = req.params[0][0] == '+', Message();
		case 's':
			return chn->priv ?: chn->secret = req.params[0][0] == '+', Message();
		case 'i':
			return chn->inviteOnly = req.params[0][0] == '+', Message();
		case 't':
			return chn->protectedTopic = req.params[0][0] == '+', Message();
		case 'n':
			return chn->externalMessages = req.params[0][0] == '+', Message();
		case 'm':
			return chn->moderated = req.params[0][0] == '+', Message();
		case 'o':
			return req.params.size() == 1
				? Message(461).addParam("MODE").addParam(":Not enough parameters")
				: chn->addOperator(req.params[1], req.params[0][0] == '+');
		case 'l':
			return req.params.size() == 1
				? Message(461).addParam("MODE").addParam(":Not enough parameters")
				: chn->setClientLimit(req.params[1]);
		case 'b':
			return req.params.size() == 1
				? Message(461).addParam("MODE").addParam(":Not enough parameters")
				: chn->setBanMask(req.params[1], req.params[0][0] == '+');
		case 'v':
			return req.params.size() == 1
				? Message(461).addParam("MODE").addParam(":Not enough parameters")
				: chn->setSpeaker(req.params[1], req.params[0][0] == '+');
		case 'k':
			return req.params.size() == 1
				? Message(461).addParam("MODE").addParam(":Not enough parameters")
				: chn->setSecret(req.params[1], req.params[0][0] == '+');
		}
	}
	return Message(501).addParam(":Unknown MODE flag");
}

const Message Server::part(User &usr, const Message &req)
{
	if (!usr.hasSecret)
		return Message(464).addParam(":Password incorrect");
	if (!usr.isRegistered())
		return Message(451).addParam(":You have not registered");
	std::istringstream chnls(req.params[0]);
	std::string name;
	while (getline(chnls, name, ','))
	{
		if (Channel *channel = lookUpChannel(name))
		{
			if (channel->isMember(&usr))
				channel->disjoin(&usr);
			else
			{
				const std::string txt =
					Message(442)
						.addParam(name)
						.addParam(":You're not on that channel")
						.totxt();
				Send(usr.fd, txt.data(), txt.size());
			}
		}
		else
		{
			const std::string txt =
				Message(403).addParam(name).addParam(":No such channel").totxt();
			Send(usr.fd, txt.data(), txt.size());
		}
	}
	return Message();
}

const Message Server::quit(User &usr, const Message &req)
{
	const std::string txt = req.totxt();
	Close(usr.fd);
	for (std::list<Channel>::const_iterator chn = channels.cbegin();
		 chn != channels.cend();
		 chn++)
		if (chn->isMember(&usr))
			chn->broadcast(txt, usr);
	users.erase(usr.fd);
	return Message();
}

const Message Server::pass(User &usr, const Message &req)
{
	if (usr.hasSecret)
		return Message();
	if (req.params.empty())
		return Message(461).addParam("PASS").addParam(":Not enough parameters");
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
	std::istringstream targets(req.params[0]);
	std::string name;
	while (getline(targets, name, ','))
	{
		if (User *user = lookUpUser(name))
		{
			for (unsigned rec = 0; rec < users.size(); rec++)
				if (users[rec]->nickname == user->nickname)
					return Message(407).addParam(name).addParam(
						":Duplicate recipients. No message delivered");
			users.push_back(user);
			const std::string txt = req.totxt();
			for (unsigned rec = 0; rec < users.size(); rec++)
				Send(users[rec]->fd, txt.data(), txt.size());
		}
		else if (Channel *chn = lookUpChannel(name))
		{
			for (std::list<Channel *>::const_iterator rec = channels.cbegin();
				 rec != channels.end();
				 rec++)
				if ((*rec)->name == chn->name)
					return Message(407).addParam(name).addParam(
						":Duplicate recipients. No message delivered");
			channels.push_back(chn);
			chn->broadcast(req.totxt(), usr);
		}
		else
			return Message(401).addParam(name).addParam(":No such nick/channel");
	}
	return Message();
}

const Message Server::join(User &usr, const Message &req)
{
	if (!usr.hasSecret)
		return Message(464).addParam(":Password incorrect");
	if (!usr.isRegistered())
		return Message(451).addParam(":You have not registered");
	if (req.params.size() < 1)
		return Message(461).addParam("JOIN").addParam(
			":Not enough parameters"); // ERR_NEEDMOREPARAMS
	std::list<Channel>::iterator channelIterator = channels.end();
	if (channelIterator == channels.end())
	{
		// check if user has access to create channel
		if (usr.is_oper)
		{
			Channel newChannel(req.params[0], &usr);

			this->channels.push_back(newChannel);
		}
		else
			return Message(403)
				.addParam(req.params[0])
				.addParam(":No such channel"); // ERR_NOSUCHCHANNEL
	}
	else
	{
		int status = channelIterator->join(&usr);
		if (!status)
			return Message(); // create reply
	}
	return Message();
}

const Message Server::notice(User &usr, const Message &req)
{
	return privmsg(usr, req), Message();
}