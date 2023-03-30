#include "ircserv.hpp"
#include "message.hpp"

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

const Message Server::notice(User &usr, const Message &req)
{
	return privmsg(usr, req), Message();
}