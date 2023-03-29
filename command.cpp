#include "ircserv.hpp"
#include "message.hpp"

const Message Server::pass(User &usr, const std::vector<std::string> &prms)
{
	if (usr.hasSecret)
		return Message();
	if (prms.empty())
		return Message(461).addParam("PASS").addParam(":Not enough parameters");
	if (usr.isRegistered())
		return Message(462).addParam(":You may not reregister");
	if (password != prms[0])
		return Message(464).addParam(":Password incorrect");
	usr.hasSecret = true;
	return Message();
}

const Message Server::user(User &usr, const std::vector<std::string> &prms)
{
	if (prms.size() < 4)
		return Message(461).addParam("USER").addParam(":Not enough parameters");
	if (usr.isRegistered())
		return Message(462).addParam(":You may not reregister");
	if (!usr.hasSecret)
		return Message(464).addParam(":Password incorrect");
	bool registered = usr.isRegistered();
	usr.username = prms[0];
	usr.hostname = prms[1];
	usr.servername = prms[2];
	usr.realname = prms[3];
	return !registered && usr.isRegistered()
		? Message(1).addParam(":Welcome to the ft_irc Network, " + usr.nickname +
							  "[!" + usr.username + "@" + usr.hostname + ']')
		: Message();
}

const Message Server::nick(User &usr, const std::vector<std::string> &prms)
{
	if (!usr.hasSecret)
		return Message(464).addParam(":Password incorrect");
	if (prms.size() < 1)
		return Message(431).addParam(":No nickname given");
	if (!User::validNick(prms[0]))
		return Message(432).addParam(prms[0]).addParam(":Erroneus nickname");
	if (nickIsUsed(prms[0]))
		return Message(433).addParam(prms[0]).addParam(
			":Nickname is already in use");
	bool registered = usr.isRegistered();
	usr.nickname = prms[0];
	return !registered && usr.isRegistered()
		? Message(1).addParam(":Welcome to the ft_irc Network, " + usr.nickname +
							  "[!" + usr.username + "@" + usr.hostname + ']')
		: Message();
}