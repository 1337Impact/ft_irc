#include "ircserv.hpp"
#include <algorithm>
#include <sstream>
#include <string>
#include <unistd.h>
#include <utility>

Message Server::topic(User &usr, const Message &req)
{
	if (!usr.hasSecret)
		return Message(464).addParam(":Password incorrect");
	if (!usr.isRegistered())
		return Message(451).addParam(":You have not registered");
	if (req.params.size() < 1)
		return Message(461).addParam("TOPIC").addParam(":Not enough parameters");
	Channel *chn = lookUpChannel(req.params[0]);
	if (!chn)
		return Message(403).addParam(req.params[0]).addParam(":No such channel");
	if (req.params.size() == 2)
		chn->topic = req.params[1];
	if (!chn->topic.empty())
		return Message(332).addParam(req.params[0]).addParam(chn->topic); // RPL_TOPIC
	return Message(331).addParam(req.params[0]).addParam(":No topic is set"); // RPL_NOTOPIC
}

Message Server::kick(User &usr, const Message &req)
{
	if (!usr.hasSecret)
		return Message(464).addParam(":Password incorrect");
	if (!usr.isRegistered())
		return Message(451).addParam(":You have not registered");
	Channel *chn = lookUpChannel(req.params[0]);
	if (!chn)
		return Message(403).addParam(req.params[0]).addParam(":No such channel");
	if (!chn->isMember(usr))
		return Message(442)
			.addParam(req.params[0])
			.addParam(":You're not on that channel");
	if (!chn->isOperator(usr))
		return Message(482)
			.addParam(req.params[0])
			.addParam(":You're not channel operator");
	User *mem = lookUpUser(req.params[0]);
	if (!chn->isMember(*mem))
		return Message(441)
			.addParam(req.params[1])
			.addParam(req.params[0])
			.addParam(":They aren't on that channel");
	return Message();
}

Message Server::names(User &usr, const Message &req)
{
	if (!usr.hasSecret)
		return Message(464).addParam(":Password incorrect");
	if (!usr.isRegistered())
		return Message(451).addParam(":You have not registered");
	if (req.params.empty())
		return Message(461).setCommand("NAMES").addParam(":Not enough parameters");
	std::istringstream chnls(req.params[0]);
	std::string name;
	while (getline(chnls, name, ','))
	{
		Channel *chn = lookUpChannel(name);
		if (!chn)
			return Message(403).addParam(name).addParam(":No such channel");
		if (chn->isSecret && !chn->isMember(usr))
			continue;
		Message res = Message(353).addParam(chn->isPrivate      ? "*"
												: chn->isSecret ? "@"
																: "=");
		for (std::vector<Channel::ChannelMember>::const_iterator it =
				 chn->members.begin();
			 it != chn->members.end();
			 it++)
			res.addParam(it->usr.nickname);
		const std::string str = res.totxt();
		Send(res, usr);
	}
	return Message(366).addParam(req.params[0]).addParam(":End of /NAMES list");
}

Message Server::list(User &usr, const Message &req)
{
	if (!usr.hasSecret)
		return Message(464).addParam(":Password incorrect");
	if (!usr.isRegistered())
		return Message(451).addParam(":You have not registered");
	Send(Message(321).addParam("Channel").addParam(":Users  Name"), usr);
	// if (req.params.size())
	{
		// std::istringstream chnls(req.params[0]);
		std::string name;
		// while (getline(chnls, name, ','))
		for (std::vector<Channel>::iterator chn = channels.begin();
			 chn != channels.end();
			 chn++)
		{
			// Channel *chn = lookUpChannel(name);
			// if (chn && chn->isMember(&usr))
			{
				Send(Message(322)
						 .addParam(chn->name)
						 .addParam(std::to_string(chn->members.size()))
						 .addParam(chn->topic),
					 usr);
			}
		}
	}
	(void)req;
	return Message(323).addParam(":End of /LIST");
}

Message Server::invite(User &usr, const Message &req)
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
	if (!chn->isMember(usr))
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
	if (chn->isMember(*mem))
		return Message(443).addParam(req.params[0]).addParam(":is already on channel");
	chn->join(*mem, "");
	return Message(341).addParam(req.params[0]).addParam(req.params[1]);
}

Message Server::mode(User &usr, const Message &req)
{
	if (!usr.hasSecret)
		return Message(464).addParam(":Password incorrect");
	if (!usr.isRegistered())
		return Message(451).addParam(":You have not registered");
	if (req.params.size() < 2)
		return Message(461).addParam("MODE").addParam(":Not enough parameters");
	Channel *chn = lookUpChannel(req.params[0]);
	if (!chn)
		return Message(403).addParam(req.params[0]).addParam(":No such channel");
	if (!chn->isOperator(usr))
		return Message(482).addParam(chn->name).addParam(
			":You're not channel operator");
	if (req.params.empty())
		return Message(324).addParam(chn->name).addParam(chn->getChannelModes());
	if (req.params[1].size() <= 2 &&
		(req.params[1][0] == '+' || req.params[1][0] == '-'))
		switch (req.params[1][1])
		{
		case 'p':
			if (!chn->isSecret)
				chn->isPrivate = req.params[1][0] == '+';
			return Message().setCommand("REPLY").addParam(
				":Private flag has been set");
		case 's':
			if (!chn->isPrivate)
				chn->isSecret = req.params[1][0] == '+';
			return Message().setCommand("REPLY").addParam(
				":Secret flag has been set");
		case 'i':
			return chn->isInviteOnly = req.params[1][0] == '+',
				   Message().setCommand("REPLY").addParam(
					   ":Invite flag has been set");
		case 't':
			return chn->hasProtectedTopic = req.params[1][0] == '+',
				   Message().setCommand("REPLY").addParam(
					   ":Protected topic has been set");
		case 'n':
			return chn->hasExternalMessages = req.params[1][0] == '+',
				   Message().setCommand("REPLY").addParam(
					   ":External messages has been set");
		case 'm':
			return chn->isModerated = req.params[1][0] == '+',
				   Message().setCommand("REPLY").addParam(
					   ":Moderated flag has been set");
		case 'o':
			return req.params.size() != 3
				? Message(461).addParam("MODE").addParam(":Not enough parameters")
				: chn->addOperator(req.params[2], req.params[1][0] == '+');
		case 'l':
			return req.params.size() != 3
				? Message(461).addParam("MODE").addParam(":Not enough parameters")
				: chn->setChannelLimit(req.params[2]);
		case 'b':
			return req.params.size() != 3
				? Message(461).addParam("MODE").addParam(":Not enough parameters")
				: chn->setBanMask(req.params[2], req.params[1][0] == '+');
		case 'v':
			return req.params.size() != 3
				? Message(461).addParam("MODE").addParam(":Not enough parameters")
				: chn->setSpeaker(req.params[2], req.params[1][0] == '+');
		case 'k':
			return req.params.size() != 3
				? Message(461).addParam("MODE").addParam(":Not enough parameters")
				: chn->setSecret(req.params[2], req.params[1][0] == '+');
		}
	return Message(501).addParam(":Unknown MODE flag");
}

Message Server::part(User &usr, const Message &req)
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
			if (channel->isMember(usr))
				channel->kick(&usr);
			else
			{
				Send(Message(442).addParam(name).addParam(
						 ":You're not on that channel"),
					 usr);
			}
		}
		else
		{
			Send(Message(403).addParam(name).addParam(":No such channel"), usr);
		}
	}
	return Message();
}

Message Server::quit(User &usr, const Message &req)
{
	const std::string txt = req.totxt();
	Close(usr.fd);
	for (std::vector<Channel>::const_iterator chn = channels.begin();
		 chn != channels.end();
		 chn++)
		if (chn->isMember(usr))
			chn->broadcast(txt, usr);
	users.erase(usr.fd);
	std::cout << "A connection has been closed" << std::endl;
	return Message();
}

Message Server::pass(User &usr, const Message &req)
{
	if (req.params.empty())
		return Message(461).addParam("PASS").addParam(":Not enough parameters");
	if (usr.isRegistered())
		return Message(462).addParam(":You may not reregister");
	if (password != req.params[0])
		return Message(464).addParam(":Password incorrect");
	usr.hasSecret = true;
	return Message();
}

Message Server::user(User &usr, const Message &req)
{
	if (!usr.hasSecret)
		return Message(464).addParam(":Password incorrect");
	if (usr.isRegistered())
		return Message(462).addParam(":You may not reregister");
	if (req.params.size() < 4)
		return Message(461).addParam("USER").addParam(":Not enough parameters");
	bool registered = usr.isRegistered();
	usr.username = req.params[0];
	usr.hostname = req.params[1];
	usr.servername = req.params[2];
	usr.realname = req.params[3];
	return !registered && usr.isRegistered()
		? Message(1).addParam(":Welcome to the ft_irc Network")
		: Message().setCommand("REPLY").addParam(":Your user info are set up");
}

Message Server::nick(User &usr, const Message &req)
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
		? Message(1).addParam(":Welcome to the ft_irc Network")
		: Message().setCommand("REPLY").addParam(
			  ":Your nickname has been changed");
}

Message Server::privmsg(User &usr, const Message &req)
{
	if (!usr.hasSecret)
		return Message(464).addParam(":Password incorrect");
	if (!usr.isRegistered())
		return Message(451).addParam(":You have not registered");
	if (req.params.size() < 1)
		return Message(411).addParam(":No recipient given").addParam("(PRIVMSG)");
	if (req.params.size() < 2)
		return Message(412).addParam(":No text to send");
	std::vector<Channel *> channels;
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
			for (unsigned rec = 0; rec < users.size(); rec++)
				Send(req, *users[rec]);
		}
		else if (Channel *chn = lookUpChannel(name))
		{
			for (std::vector<Channel *>::const_iterator rec = channels.begin();
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

Message Server::join(User &usr, const Message &req)
{
	if (!usr.hasSecret)
		return Message(464).addParam(":Password incorrect");
	if (!usr.isRegistered())
		return Message(451).addParam(":You have not registered");
	if (req.params.size() < 1)
		return Message(461).addParam("JOIN").addParam(
			":Not enough parameters"); // ERR_NEEDMOREPARAMS
	std::stringstream _ss(req.params[0]);
	std::stringstream _ss2;
	if (req.params.size() > 1)
		_ss2 << req.params[1];
	std::string _ch;
	std::string _key;

	while (std::getline(_ss, _ch, ','))
	{
		int _isKey = 0;
		if (req.params.size() > 1)
			_isKey = std::getline(_ss2, _key, ',') ? 1 : 0;
		if (!Channel::isValidName(_ch))
			Send(Message(476).addParam(_ch).addParam(":Bad Channel Mask"), usr);
		std::vector<Channel>::iterator channelIterator =
			std::find(channels.begin(), channels.end(), _ch);
		int status = 0;
		if (channelIterator == channels.end())
		{
			Channel newChannel(
				_ch, usr, (_isKey ? _key : "")); // create new channel
			channels.push_back(newChannel);
			channelIterator = channels.end() - 1;
			std::cout << "channel has been created\n";
		}
		else
		{
			status = channelIterator->join(usr, (_isKey ? _key : ""));
			std::cout << "user has joined channel\n";
		}
		switch (status)
		{
		case 0:
		{
			channelIterator->broadcast(
				Message(332).setPrefix(usr).addParam("JOIN").addParam(_ch).totxt(),
				usr);
			Send(Message(332).setPrefix(usr).addParam("JOIN").addParam(_ch), usr);
			Send(topic(usr, Message().setCommand("TOPIC").addParam(_ch)), usr);
			Send(names(usr, Message().setCommand("TOPIC").addParam(_ch)), usr);
			break;
		}
		case 1: // invite only error
		{
			Send(Message(473) // ERR_INVITEONLYCHAN
					 .addParam(_ch)
					 .addParam(":Cannot join channel (+i)"),
				 usr);
			break;
		}
		case 2: // wrong key
		{
			Send(Message(475).addParam(_ch).addParam(":Cannot join channel (+k)"),
				 usr);
			break;
		}
		case 3: // channel is full
		{
			Send(Message(471).addParam(_ch).addParam(":Cannot join channel (+l)"),
				 usr);
			break;
		}
		case 4: // already in channel
		{
			Send(Message(400).addParam("JOIN").addParam(
					 ":you're already on channel"),
				 usr);
			break;
		}
		}
	}
	return Message();
}

Message Server::notice(User &usr, const Message &req)
{
	// ERR_CANNOTSENDTOCHAN to check
	return privmsg(usr, req), Message();
}