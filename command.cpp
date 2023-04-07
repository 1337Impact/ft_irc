#include "ircserv.hpp"
#include <algorithm>
#include <sstream>
#include <string>
#include <unistd.h>
#include <utility>

Message Server::topic(User &usr, const Message &req)
{
	if (!usr.hasSecret)
		return Message(usr, 464).addParam(":Password incorrect");
	if (!usr.isRegistered())
		return Message(usr, 451).addParam(":You have not registered");
	if (req.params.size() < 1)
		return Message(usr, 461).addParam("TOPIC").addParam(":Not enough parameters");
	Channel *chn = lookUpChannel(req.params[0]);
	if (chn && !chn->topic.empty())
		return Message(usr, 332).addParam(req.params[0]).addParam(chn->topic);
	return Message();
}

Message Server::kick(User &usr, const Message &req)
{
	if (!usr.hasSecret)
		return Message(usr, 464).addParam(":Password incorrect");
	if (!usr.isRegistered())
		return Message(usr, 451).addParam(":You have not registered");
	Channel *chn = lookUpChannel(req.params[0]);
	if (!chn)
		return Message(usr, 403).addParam(req.params[0]).addParam(":No such channel");
	if (!chn->isMember(&usr))
		return Message(usr, 442)
			.addParam(req.params[0])
			.addParam(":You're not on that channel");
	if (!chn->isOperator(usr))
		return Message(usr, 482)
			.addParam(req.params[0])
			.addParam(":You're not channel operator");
	User *mem = lookUpUser(req.params[0]);
	if (!chn->isMember(mem))
		return Message(usr, 441)
			.addParam(req.params[1])
			.addParam(req.params[0])
			.addParam(":They aren't on that channel");
	return Message();
}

Message Server::names(User &usr, const Message &req)
{
	if (!usr.hasSecret)
		return Message(usr, 464).addParam(":Password incorrect");
	if (!usr.isRegistered())
		return Message(usr, 451).addParam(":You have not registered");
	std::istringstream chnls(req.params[0]);
	std::string name;
	while (getline(chnls, name, ','))
	{
		Channel *chn = lookUpChannel(name);
		if (!chn)
			return Message(usr, 403).addParam(name).addParam(":No such channel");
		if (chn->isSecret && !chn->isMember(&usr))
			return Message();
		Message res = Message(usr, 353).addParam(chn->isPrivate      ? "*"
												: chn->isSecret ? "@"
																: "=");
		for (std::vector<Channel::ChannelMember>::const_iterator it =
				 chn->members.begin();
			 it != chn->members.end();
			 it++)
			res.addParam(it->usr.nickname);
		return res;
	}
	return Message(usr, 366).addParam(req.params[0]).addParam(":End of /NAMES list");
}

Message Server::list(User &usr, const Message &req)
{
	if (!usr.hasSecret)
		return Message(usr, 464).addParam(":Password incorrect");
	if (!usr.isRegistered())
		return Message(usr, 451).addParam(":You have not registered");
	std::string rpl =
		Message(usr, 321).addParam("Channel").addParam(":Users  Name").totxt();
	Send(usr.fd, rpl.data(), rpl.size());
	// if (req.params.size())
	{
		// std::istringstream chnls(req.params[0]);
		std::string name;
		// while (getline(chnls, name, ','))
		for (std::vector<Channel>::iterator chn = channels.begin(); chn != channels.end(); chn++)
		{
			// Channel *chn = lookUpChannel(name);
			// if (chn && chn->isMember(&usr))
			{
				rpl = Message(usr, 322)
						.addParam(chn->name)
						.addParam(std::to_string(chn->members.size()))
						.addParam(chn->topic)
						.totxt();
				Send(usr.fd, rpl.data(), rpl.size());
			}
		}
	}
	(void)req;
	return Message(usr, 323).addParam(":End of /LIST");
}

Message Server::invite(User &usr, const Message &req)
{
	if (!usr.hasSecret)
		return Message(usr, 464).addParam(":Password incorrect");
	if (!usr.isRegistered())
		return Message(usr, 451).addParam(":You have not registered");
	if (req.params.size() < 2)
		return Message(usr, 461).addParam("INVITE").addParam(":Not enough parameters");
	Channel *chn = nullptr;
	if (!chn)
		return Message(usr, 403).addParam(req.params[0]).addParam(":No such channel");
	if (!chn->isMember(&usr))
		return Message(usr, 442)
			.addParam(req.params[0])
			.addParam(":You're not on that channel");
	if (!chn->isOperator(usr))
		return Message(usr, 482)
			.addParam(req.params[0])
			.addParam(":You're not channel operator");
	User *mem = lookUpUser(req.params[0]);
	if (!mem)
		return Message(usr, 401).addParam(":No such nick/channel");
	if (chn->isMember(mem))
		return Message(usr, 443).addParam(req.params[0]).addParam(":is already on channel");
	chn->join(*mem, "");
	return Message(usr, 341).addParam(req.params[0]).addParam(req.params[1]);
}

Message Server::mode(User &usr, const Message &req)
{
	if (!usr.hasSecret)
		return Message(usr, 464).addParam(":Password incorrect");
	if (!usr.isRegistered())
		return Message(usr, 451).addParam(":You have not registered");
	Channel *chn = lookUpChannel(req.params[0]);
	if (!chn)
		return Message(usr, 403).addParam(req.params[0]).addParam(":No such channel");
	if (!chn->isOperator(usr))
		return Message(usr, 482).addParam(chn->name).addParam(
			":You're not channel operator");
	if (req.params.empty())
		return Message(usr, 324).addParam(chn->name).addParam(chn->getChannelModes());
	if (req.params[0].size() <= 2 &&
		(req.params[0][0] == '+' || req.params[0][0] == '-'))
	{
		switch (req.params[0][1])
		{
		case 'p':
			if (!chn->isSecret)
				chn->isPrivate = req.params[0][0] == '+';
						return 		   Message();
		case 's':
			if (!chn->isPrivate)
				chn->isSecret = req.params[0][0] == '+';
						return 		   Message();
		case 'i':
			return chn->isInviteOnly = req.params[0][0] == '+', Message();
		case 't':
			return chn->hasProtectedTopic = req.params[0][0] == '+', Message();
		case 'n':
			return chn->hasExternalMessages = req.params[0][0] == '+', Message();
		case 'm':
			return chn->isModerated = req.params[0][0] == '+', Message();
		case 'o':
			return req.params.size() == 1
				? Message(usr, 461).addParam("MODE").addParam(":Not enough parameters")
				: chn->addOperator(usr, req.params[1], req.params[0][0] == '+');
		case 'l':
			return req.params.size() == 1
				? Message(usr, 461).addParam("MODE").addParam(":Not enough parameters")
				: chn->setClientLimit(req.params[1]);
		case 'b':
			return req.params.size() == 1
				? Message(usr, 461).addParam("MODE").addParam(":Not enough parameters")
				: chn->setBanMask(req.params[1], req.params[0][0] == '+');
		case 'v':
			return req.params.size() == 1
				? Message(usr, 461).addParam("MODE").addParam(":Not enough parameters")
				: chn->setSpeaker(usr, req.params[1], req.params[0][0] == '+');
		case 'k':
			return req.params.size() == 1
				? Message(usr, 461).addParam("MODE").addParam(":Not enough parameters")
				: chn->setSecret(req.params[1], req.params[0][0] == '+');
		}
	}
	return Message(usr, 501).addParam(":Unknown MODE flag");
}

Message Server::part(User &usr, const Message &req)
{
	if (!usr.hasSecret)
		return Message(usr, 464).addParam(":Password incorrect");
	if (!usr.isRegistered())
		return Message(usr, 451).addParam(":You have not registered");
	std::istringstream chnls(req.params[0]);
	std::string name;
	while (getline(chnls, name, ','))
	{
		if (Channel *channel = lookUpChannel(name))
		{
			if (channel->isMember(&usr))
				channel->kick(&usr);
			else
			{
				const std::string txt =
					Message(usr, 442)
						.addParam(name)
						.addParam(":You're not on that channel")
						.totxt();
				Send(usr.fd, txt.data(), txt.size());
			}
		}
		else
		{
			const std::string txt =
				Message(usr, 403).addParam(name).addParam(":No such channel").totxt();
			Send(usr.fd, txt.data(), txt.size());
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
		if (chn->isMember(&usr))
			chn->broadcast(txt, usr);
	users.erase(usr.fd);
	return Message();
}

Message Server::pass(User &usr, const Message &req)
{
	if (req.params.empty())
		return Message(usr, 461).addParam("PASS").addParam(":Not enough parameters");
	if (usr.isRegistered())
		return Message(usr, 462).addParam(":You may not reregister");
	if (password != req.params[0])
		return Message(usr, 464).addParam(":Password incorrect");
	usr.hasSecret = true;
	return Message();
}

Message Server::user(User &usr, const Message &req)
{
	if (!usr.hasSecret)
		return Message(usr, 464).addParam(":Password incorrect");
	if (usr.isRegistered())
		return Message(usr, 462).addParam(":You may not reregister");
	if (req.params.size() < 4)
		return Message(usr, 461).addParam("USER").addParam(":Not enough parameters");
	bool registered = usr.isRegistered();
	usr.username = req.params[0];
	usr.hostname = req.params[1];
	usr.servername = req.params[2];
	usr.realname = req.params[3];
	return !registered && usr.isRegistered()
		? Message(usr, 1).setPrefix(usr).addParam(":Welcome to the ft_irc Network")
		: Message();
}

Message Server::nick(User &usr, const Message &req)
{
	if (!usr.hasSecret)
		return Message(usr, 464).addParam(":Password incorrect");
	if (req.params.size() < 1)
		return Message(usr, 431).addParam(":No nickname given");
	if (!User::validNick(req.params[0]))
		return Message(usr, 432).addParam(req.params[0]).addParam(":Erroneus nickname");
	if (nickIsUsed(req.params[0]))
		return Message(usr, 433)
			.addParam(req.params[0])
			.addParam(":Nickname is already in use");
	bool registered = usr.isRegistered();
	usr.nickname = req.params[0];
	return !registered && usr.isRegistered()
		? Message(usr, 1).setPrefix(usr).addParam(":Welcome to the ft_irc Network")
		: Message().setPrefix(usr).addParam(":Your nickname has been changed");
}

Message Server::privmsg(User &usr, const Message &req)
{
	if (!usr.hasSecret)
		return Message(usr, 464).addParam(":Password incorrect");
	if (!usr.isRegistered())
		return Message(usr, 451).addParam(":You have not registered");
	if (req.params.size() < 1)
		return Message(usr, 411).addParam(":No recipient given").addParam("(PRIVMSG)");
	if (req.params.size() < 2)
		return Message(usr, 412).addParam(":No text to send");
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
					return Message(usr, 407).addParam(name).addParam(
						":Duplicate recipients. No message delivered");
			users.push_back(user);
			const std::string txt = req.totxt();
			for (unsigned rec = 0; rec < users.size(); rec++)
				Send(users[rec]->fd, txt.data(), txt.size());
		}
		else if (Channel *chn = lookUpChannel(name))
		{
			for (std::vector<Channel *>::const_iterator rec = channels.begin();
				 rec != channels.end();
				 rec++)
				if ((*rec)->name == chn->name)
					return Message(usr, 407).addParam(name).addParam(
						":Duplicate recipients. No message delivered");
			channels.push_back(chn);
			chn->broadcast(req.totxt(), usr);
		}
		else
			return Message(usr, 401).addParam(name).addParam(":No such nick/channel");
	}
	return Message();
}

Message Server::join(User &usr, const Message &req)
{
	if (!usr.hasSecret)
		return Message(usr, 464).addParam(":Password incorrect");
	if (!usr.isRegistered())
		return Message(usr, 451).addParam(":You have not registered");
	if (req.params.size() < 1)
		return Message(usr, 461).addParam("JOIN").addParam(
			":Not enough parameters"); // ERR_NEEDMOREPARAMS
	std::map<std::string, std::string> params;
	std::istringstream _ss(req.params[0]);
	std::istringstream _ss2(req.params[1]);
	std::string _ch;
	std::string _key;

	while (std::getline(_ss, _ch, ','))
	{
		int _isKey = std::getline(_ss2, _key, ',') ? 1 : 0;

		std::vector<Channel>::iterator channelIterator =
			std::find(channels.begin(), channels.end(), _ch);
		int status = 0;
		if (channelIterator == channels.end())
		{
			Channel newChannel(
				_ch, usr, (_isKey ? _key : "")); // create new channel
			channels.push_back(newChannel);
			channelIterator = channels.end() - 1;
		}
		else
			status = channelIterator->join(usr, (_isKey ? _key : ""));
		switch (status)
		{
		case 0: // join successful
		{
			// 1 broadcast this message to all users
			channelIterator->broadcast(
				Message(usr, 332).setPrefix(usr).addParam("JOIN").addParam(_ch).totxt(),
				usr);
			std::string res(
				Message(usr, 332).setPrefix(usr).addParam("JOIN").addParam(_ch).totxt());
			send(usr.fd, res.data(), res.size(), 0);

			// 2 reply to usr with channel topic
			std::string _tmp =
				topic(usr, Message().setCommand("TOPIC").addParam(_ch)).totxt();
			Send(usr.fd, _tmp.data(), _tmp.size());

			// 3 reply to usr with channel member
			_tmp =
				names(usr, Message().setCommand("NAMES").addParam(_ch)).totxt();
			Send(usr.fd, _tmp.data(), _tmp.size());
			// 4 reply with END OF NAMES list
			return Message(usr, 366)
				.addParam(usr.nickname)
				.addParam(_ch)
				.addParam(":End of NAMES list");
		}
		case 1: // invite only error
		{
			return Message(usr, 473) // ERR_INVITEONLYCHAN
				.addParam(_ch)
				.addParam(":Cannot join channel (+i)");
		}
		case 2: // wrong key
		{
			return Message(usr, 475) // ERR_BADCHANNELKEY
				.addParam(_ch)
				.addParam(":Cannot join channel (+k)");
		}
		case 3: // channel is full
		{
			return Message(usr, 471) // ERR_CHANNELISFULL
				.addParam(_ch)
				.addParam(":Cannot join channel (+l)");
		}
		case 4: // banned from channel
		{
			return Message(); // I'm here now
		}
		}
	}
	return Message();
}

Message Server::notice(User &usr, const Message &req)
{
	//ERR_CANNOTSENDTOCHAN to check
	return privmsg(usr, req), Message();
}