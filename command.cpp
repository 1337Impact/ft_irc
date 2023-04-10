#include "ircserv.hpp"
#include <sstream>
#include <unistd.h>

Message Server::dcc(User &usr, const Message &req)
{
	if (req.params.size() < 3)
		return Message(461).addParam("DCC").addParam("Not enough parameters");
	User *recipient = lookUpUser(req.params[1]);
	if (!recipient)
		return Message(401).addParam(req.params[1]).addParam("No such nick/channel");
	Send(req, *recipient);
	return (void)usr, Message();
}

Message Server::topic(User &usr, const Message &req)
{
	if (!usr.hasSecret)
		return Message(464).addParam("Password incorrect");
	if (!usr.isRegistered())
		return Message(451).addParam("You have not registered");
	if (req.params.size() < 1)
		return Message(461).addParam("TOPIC").addParam("Not enough parameters");
	Channel *chn = lookUpChannel(req.params[0]);
	if (!chn)
		return Message(403).addParam(req.params[0]).addParam("No such channel");
	if (req.params.size() == 2)
	{
		if (chn->hasProtectedTopic() && !chn->isOperator(usr))
			return Message(482)
				.addParam(req.params[0])
				.addParam("You're not channel operator");
		chn->topic = req.params[1];
	}
	if (!chn->topic.empty())
		return Message(332).addParam(req.params[0]).addParam(chn->topic);
	return Message(331).addParam(req.params[0]).addParam("No topic is set");
}

Message Server::kick(User &usr, const Message &req)
{
	if (!usr.hasSecret)
		return Message(464).addParam("Password incorrect");
	if (!usr.isRegistered())
		return Message(451).addParam("You have not registered");
	if (req.params.size() < 2)
		return Message(461).addParam("KICK").addParam("Not enough parameters");
	Channel *chn = lookUpChannel(req.params[0]);
	if (!chn)
		return Message(403).addParam(req.params[0]).addParam("No such channel");
	if (!chn->isMember(usr))
		return Message(442)
			.addParam(req.params[0])
			.addParam("You're not on that channel");
	if (!chn->isOperator(usr))
		return Message(482)
			.addParam(req.params[0])
			.addParam("You're not channel operator");
	User *mem = lookUpUser(req.params[1]);
	if (!mem)
		return Message(401).addParam(req.params[0]).addParam("No such nick/channel");
	if (!chn->isMember(*mem))
		return Message(441)
			.addParam(req.params[1])
			.addParam(req.params[0])
			.addParam("They aren't on that channel");
	chn->broadcast(
		Message().setPrefix(usr).setCommand("KICK").addParam(chn->name).totxt(),
		usr);
	return chn->remove(mem), Message();
}

Message Server::names(User &usr, const Message &req)
{
	if (!usr.hasSecret)
		return Message(464).addParam("Password incorrect");
	if (!usr.isRegistered())
		return Message(451).addParam("You have not registered");
	if (req.params.empty())
		for (std::vector<Channel>::iterator chn = channels.begin();
			 chn != channels.end();
			 chn++)
			sendChannelMemberList(&*chn, usr);
	else
	{
		std::istringstream chnls(req.params[0]);
		std::string name;
		while (getline(chnls, name, ','))
			if (Channel *chn = lookUpChannel(name))
				sendChannelMemberList(chn, usr);
			else
				Send(Message(403).addParam(name).addParam("No such channel"),
					 usr);
	}
	if (req.params.empty())
	{
		Message res(353);
		res.addParam("=").addParam("*");
		for (std::map<const int, User>::iterator usr = users.begin();
			 usr != users.end();
			 usr++)
			if (usr->second.isRegistered() && !usr->second.nchannels)
				res.params.size() == 2 ? res.addParam(':' + usr->second.nickname)
									   : res.addParam(usr->second.nickname);
		if (res.params.size() > 1)
			Send(res, usr);
	}
	return req.params.empty()
		? Message(366).addParam("End of /NAMES list")
		: Message(366).addParam(req.params[0]).addParam("End of /NAMES list");
}

Message Server::list(User &usr, const Message &req)
{
	if (!usr.hasSecret)
		return Message(464).addParam("Password incorrect");
	if (!usr.isRegistered())
		return Message(451).addParam("You have not registered");
	Send(Message(321).addParam("Channel").addParam("Users Name"), usr);
	if (req.params.empty())
		for (std::vector<Channel>::iterator chn = channels.begin();
			 chn != channels.end();
			 chn++)
		{
			if ((!chn->isPrivate() && !chn->isSecret()) ||
				(chn->isPrivate() && chn->isMember(usr)))
				Send(Message(322)
						 .addParam(chn->name)
						 .addParam(std::to_string(chn->members.size()))
						 .addParam(chn->topic),
					 usr);
		}
	else
	{
		std::istringstream chnls(req.params[0]);
		std::string name;
		while (getline(chnls, name, ','))
		{
			Channel *chn = lookUpChannel(name);
			if (!chn)
				Send(Message(403).addParam(name).addParam("No such channel"),
					 usr);
			else if ((!chn->isPrivate() && !chn->isSecret()) ||
					 (chn->isPrivate() && chn->isMember(usr)))
				Send(Message(322)
						 .addParam(chn->name)
						 .addParam(std::to_string(chn->members.size()))
						 .addParam(chn->topic),
					 usr);
		}
	}
	return Message(323).addParam("End of /LIST");
}

Message Server::invite(User &usr, const Message &req)
{
	if (!usr.hasSecret)
		return Message(464).addParam("Password incorrect");
	if (!usr.isRegistered())
		return Message(451).addParam("You have not registered");
	if (req.params.size() < 2)
		return Message(461).addParam("INVITE").addParam("Not enough parameters");
	Channel *chn = lookUpChannel(req.params[1]);
	if (!chn)
		return Message(403).addParam(req.params[0]).addParam("No such channel");
	if (!chn->isMember(usr))
		return Message(442)
			.addParam(req.params[0])
			.addParam("You're not on that channel");
	if (!chn->isOperator(usr))
		return Message(482)
			.addParam(req.params[0])
			.addParam("You're not channel operator");
	User *mem = lookUpUser(req.params[0]);
	if (!mem)
		return Message(401).addParam("No such nick/channel");
	if (chn->isMember(*mem))
		return Message(443).addParam(req.params[0]).addParam("is already on channel");
	chn->invited.push_back(mem);
	return Message(341).addParam(req.params[0]).addParam(req.params[1]);
}

Message Server::mode(User &usr, const Message &req)
{
	if (!usr.hasSecret)
		return Message(464).addParam("Password incorrect");
	if (!usr.isRegistered())
		return Message(451).addParam("You have not registered");
	if (req.params.size() < 2)
		return Message(461).addParam("MODE").addParam("Not enough parameters");
	Channel *chn = lookUpChannel(req.params[0]);
	if (!chn)
		return Message(403).addParam(req.params[0]).addParam("No such channel");
	if (!chn->isOperator(usr))
		return Message(482).addParam(chn->name).addParam(
			":You're not channel operator");
	if (req.params.empty())
		return Message(324).addParam(chn->name).addParam(chn->getChannelModes());
	if (!(req.params[1].size() <= 2) ||
		(req.params[1][0] != '+' && req.params[1][0] != '-'))
		return Message(501).addParam("Unknown MODE flag");
	const int flag = Channel::FlagToMask[(int)req.params[1][1]];
	const bool set = req.params[1][0] == '+';
	if ((flag & PrivateMask && chn->isSecret()) ||
		(flag & SecretMask && chn->isPrivate()))
		return Message();
	if (flag >= SecretKeyMask && flag <= OperatorMask && req.params.size() != 3)
		return Message(461).addParam("MODE").addParam("Not enough parameters");
	Message res = flag & SecretKeyMask ? chn->setSecret(req.params[2], set)
		: flag & SpeakerMask           ? chn->setSpeaker(req.params[2], set)
		: flag & BanMask               ? chn->setBanMask(req.params[2], set)
		: flag & ChannelLimitMask ? chn->setChannelLimit(req.params[2], set)
		: flag & OperatorMask     ? chn->setSecret(req.params[2], set)
								  : Message();
	return set ? chn->modes |= flag : chn->modes &= ~flag, res;
}

Message Server::part(User &usr, const Message &req)
{
	if (!usr.hasSecret)
		return Message(464).addParam("Password incorrect");
	if (!usr.isRegistered())
		return Message(451).addParam("You have not registered");
	std::istringstream chnls(req.params[0]);
	std::string name;
	while (getline(chnls, name, ','))
	{
		if (Channel *channel = lookUpChannel(name))
			if (channel->isMember(usr))
			{
				channel->broadcast(
					Message().setPrefix(usr).setCommand("PART").addParam(name).totxt(),
					usr);
				channel->remove(&usr);
			}
			else
				Send(Message(442).addParam(name).addParam(
						 ":You're not on that channel"),
					 usr);
		else
			Send(Message(403).addParam(name).addParam("No such channel"), usr);
	}
	return Message();
}

Message Server::quit(User &usr, const Message &req)
{
	const std::string txt = req.totxt();
	Close(usr.fd);
	for (std::vector<Channel>::iterator chn = channels.begin();
		 chn != channels.end();
		 chn++)
		if (chn->isMember(usr))
		{
			chn->broadcast(txt, usr);
			chn->members.erase(
				find(chn->members.begin(), chn->members.end(), usr));
		}
	users.erase(usr.fd);
	std::cout << "A connection has been closed" << std::endl;
	return Message();
}

Message Server::pass(User &usr, const Message &req)
{
	if (req.params.empty())
		return Message(461).addParam("PASS").addParam("Not enough parameters");
	if (usr.isRegistered())
		return Message(462).addParam("You may not reregister");
	if (password != req.params[0])
		return Message(464).addParam("Password incorrect");
	usr.hasSecret = true;
	return Message();
}

Message Server::user(User &usr, const Message &req)
{
	if (!usr.hasSecret)
		return Message(464).addParam("Password incorrect");
	if (usr.isRegistered())
		return Message(462).addParam("You may not reregister");
	if (req.params.size() < 4)
		return Message(461).addParam("USER").addParam("Not enough parameters");
	usr.username = req.params[0];
	usr.hostname = req.params[1];
	usr.servername = req.params[2];
	usr.realname = req.params[3];
	if (!usr.isRegistered())
		return Message().setCommand("REPLY").addParam(
			":Your user info are set up");
	Send(Message(1).addParam(usr.nickname).addParam("Welcome to the ft_irc Network"),
		 usr);
	Send(Message(2).addParam("Your host is " + usr.servername +
							 ", running version 1.0"),
		 usr);
	Send(Message(3).addParam("This server was created <datetime>"), usr);
	Send(Message(4).addParam(
			 usr.nickname +
			 " localhost 1.0 [] {[+|-]|o|p|s|i|t|n|b|m|l|b|v|k} [<limit>] [<user>] [<ban mask>]"),
		 usr);
	Send(Message(372).addParam(usr.nickname).addParam("<line of the motd>"),
		 usr);
	return Message(376).addParam(usr.nickname).addParam("End of /MOTD command.");
}

Message Server::nick(User &usr, const Message &req)
{
	if (!usr.hasSecret)
		return Message(464).addParam("Password incorrect");
	if (req.params.size() < 1)
		return Message(431).addParam("No nickname given");
	if (!User::validNick(req.params[0]))
		return Message(432).addParam(req.params[0]).addParam("Erroneus nickname");
	if (nickIsUsed(req.params[0]))
		return Message(433)
			.addParam(req.params[0])
			.addParam("Nickname is already in use");
	bool isRegistered = usr.isRegistered();
	usr.nickname = req.params[0];
	if (isRegistered || !usr.isRegistered())
		return Message().setCommand("REPLY").addParam(
			"Your user info are set up");
	Send(Message(1).addParam(usr.nickname).addParam("Welcome to the ft_irc Network"),
		 usr);
	Send(Message(2).addParam("Your host is " + usr.servername +
							 ", running version 1.0"),
		 usr);
	Send(Message(3).addParam("This server was created <datetime>"), usr);
	Send(Message(4).addParam(
			 usr.nickname +
			 " localhost 1.0 [] {[+|-]|o|p|s|i|t|n|b|m|l|b|v|k} [<limit>] [<user>] [<ban mask>]"),
		 usr);
	Send(Message(372).addParam(usr.nickname).addParam("<line of the motd>"),
		 usr);
	return Message(376).addParam(usr.nickname).addParam("End of /MOTD command.");
}

Message Server::privmsg(User &usr, const Message &req)
{
	if (!usr.hasSecret)
		return Message(464).addParam("Password incorrect");
	if (!usr.isRegistered())
		return Message(451).addParam("You have not registered");
	if (req.params.size() < 1)
		return Message(411).addParam("No recipient given").addParam("(PRIVMSG)");
	if (req.params.size() < 2)
		return Message(412).addParam("No text to send");
	std::vector<Channel *> channels;
	std::vector<User *> users;
	std::istringstream targets(req.params[0]);
	std::string name;
	while (getline(targets, name, ','))
		if (User *user = lookUpUser(name))
		{
			if (find(users.begin(), users.end(), user) != users.end())
				Send(Message(407).addParam(name).addParam(
						 "Duplicate recipients. No message delivered"),
					 usr);
			else
			{
				users.push_back(user);
				if (name == "Emet")
					Send(bot.botTalk(usr, req.params[1]), usr);
				else
					Send(req, *user);
			}
		}
		else if (Channel *chn = lookUpChannel(name))
		{
			if (find(channels.begin(), channels.end(), chn) != channels.end())
				Send(Message(407).addParam(name).addParam(
						 ":Duplicate recipients. No message delivered"),
					 usr);
			else if (channels.push_back(chn),
					 (chn->hasNoExternalMessages() && !chn->isMember(usr)) ||
						 (chn->isModerated() && !chn->isSpeaker(usr) &&
						  !chn->isOperator(usr)))
				(chn->isSecret() || chn->isPrivate()) && !chn->isMember(usr) &&
						!chn->isSpeaker(usr)
					? Send(Message(401).addParam(name).addParam(
							   ":No such nick/channel"),
						   usr)
					: Send(Message(404).addParam(name).addParam(
							   ":Cannot send to channel"),
						   usr);
			else
				chn->broadcast(req.totxt(), usr);
		}
		else
			Send(Message(401).addParam(name).addParam("No such nick/channel"),
				 usr);
	return Message();
}

Message Server::join(User &usr, const Message &req)
{
	if (!usr.hasSecret)
		return Message(464).addParam("Password incorrect");
	if (!usr.isRegistered())
		return Message(451).addParam("You have not registered");
	if (req.params.size() < 1)
		return Message(461).addParam("JOIN").addParam("Not enough parameters");
	std::stringstream _ss(req.params[0]), _ss2;
	if (req.params.size() > 1)
		_ss2 << req.params[1];
	std::string _ch, _key;
	while (std::getline(_ss, _ch, ','))
	{
		int _isKey = req.params.size() > 1 ? !!std::getline(_ss2, _key, ',') : 0;
		if (!Channel::isValidName(_ch))
			Send(Message(476).addParam(_ch).addParam("Bad Channel Mask"), usr);
		std::vector<Channel>::iterator channelIterator =
			std::find(channels.begin(), channels.end(), _ch);
		int status = 0;
		if (channelIterator == channels.end())
		{
			Channel newChannel(_ch, usr, (_isKey ? _key : ""));
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
			Send(Message().setPrefix(usr).setCommand("JOIN").addParam(_ch), usr);
			channelIterator->broadcast(
				Message().setPrefix(usr).setCommand("JOIN").addParam(_ch).totxt(),
				usr);
			Send(topic(usr, Message().setCommand("TOPIC").addParam(_ch)), usr);
			Send(names(usr, Message().setCommand("NAMES").addParam(_ch)), usr);
			break;
		case 1:
			Send(Message(473).addParam(_ch).addParam("Cannot join channel (+i)"),
				 usr);
			break;
		case 2:
			Send(Message(475).addParam(_ch).addParam("Cannot join channel (+k)"),
				 usr);
			break;
		case 3:
			Send(Message(471).addParam(_ch).addParam("Cannot join channel (+l)"),
				 usr);
			break;
		case 4:
			Send(Message(400).addParam("JOIN").addParam(
					 ":you're already on channel"),
				 usr);
			break;
		case 5:
			Send(Message(474).addParam(_ch).addParam("Cannot join channel (+b)"),
				 usr);
			break;
		}
	}
	return Message();
}

Message Server::notice(User &usr, const Message &req)
{
	return privmsg(usr, req), Message();
}