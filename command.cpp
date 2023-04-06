#include "ircserv.hpp"
#include "message.hpp"
#include <algorithm>
#include <string>
#include <utility>

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
		? Message(1).addParam(":Welcome to the ft_irc Network, " + usr.nickname +
							  "[!" + usr.username + "@" + usr.hostname + ']')
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
		? Message(1).addParam(":Welcome to the ft_irc Network, " + usr.nickname +
							  "[!" + usr.username + "@" + usr.hostname + ']')
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
	std::vector<User *> recipients;
	for (unsigned target = 0; target < req.params.size() - 1; target++)
	{
		User *rec = lookUpUser(req.params[target]);
		if (!rec)
			return Message(401)
				.addParam(req.params[target])
				.addParam(":No such nick/channel");
		for (unsigned recipient = 0; recipient < recipients.size(); recipient++)
			if (recipients[recipient]->nickname == rec->nickname)
				return Message(407)
					.addParam(req.params[target])
					.addParam(":Duplicate recipients. No message delivered");
		recipients.push_back(rec);
		std::string txt = req.totxt();
		for (unsigned recipient = 0; recipient < recipients.size(); recipient++)
			if (send(recipients[recipient]->fd, txt.data(), txt.size(), 0) == -1)
				BlockingError("send");
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
	std::map<std::string, std::string> params;
	std::istringstream _ss(req.params[0]);
	std::istringstream _ss2(req.params[1]);
	std::string _ch;
	std::string _key;

	while (std::getline(_ss, _ch, ','))
	{
		int _isKey = std::getline(_ss2, _key, ',') ? 1 : 0;

		std::list<Channel>::iterator channelIterator =
			std::find(channels.begin(), channels.end(), _ch);
		if (channelIterator == channels.end())
		{
			if (usr.is_oper) // check if user has access to create channel
			{
				Channel newChannel(_ch, usr, (_isKey ? _key : "")); // create new channel
				this->channels.push_back(newChannel);
			}
			else
				return Message(403).addParam(_ch).addParam(
					":No such channel"); // ERR_NOSUCHCHANNEL
		}
		else
		{
			int status = channelIterator->join(usr, (_isKey ? _key : ""));
			switch (status)
			{
				case 0: // join successful
				{
					// 1 broadcast this message to all users
					channelIterator->broadcast(
						Message(332).addPrefix(usr).addParam("JOIN").addParam(_ch).totxt());

					// 2 reply to usr with channel topic
					if (!channelIterator->chTopic.empty())
					{
						std::string res(Message(332)
											.addParam(usr.nickname)
											.addParam(_ch)
											.addParam(channelIterator->chTopic)
											.totxt());
						send(usr.fd, res.data(), res.size(), 0);
					}

					// 3 reply to usr with channel members
					std::string res2(Message(353)
										.addParam(usr.nickname)
										.addParam(_ch)
										.addParam(channelIterator->get_memebers())
										.totxt());
					send(usr.fd, res2.data(), res2.size(), 0);
					// 4 reply with END OF NAMES list
					return Message(366)
						.addParam(usr.nickname)
						.addParam(_ch)
						.addParam(":End of NAMES list");
				}
				case 1: // invite only error
				{
					return Message(473) // ERR_INVITEONLYCHAN
						.addParam(_ch)
						.addParam(":Cannot join channel (+i)");
				}
				case 2: // wrong key
				{
					return Message(475) // ERR_BADCHANNELKEY
						.addParam(_ch)
						.addParam(":Cannot join channel (+k)");
				}
				case 3: // channel is full
				{
					return Message(471) // ERR_CHANNELISFULL
						.addParam(_ch)
						.addParam(":Cannot join channel (+l)");
				}
				case 4: // banned from channel
				{
					return Message(); // I'm here now
				}
			}
		}
	}
	return Message();
}
// const Message Server::notice(User & usr, const Message &req)
// {
// 	return privmsg(usr, req), Message();
// }