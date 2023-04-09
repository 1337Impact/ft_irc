#include "ircserv.hpp"
#include <sstream>

Message &Message::setCommand(const std::string &cmd)
{
	command = cmd;
	return *this;
}

// parse request
Message::Message(std::string &msg)
{
	size_t i = 0;
	for (; msg[i] == ' ' && i < msg.size(); i++)
		;
	if (msg[i] == ':')
		for (; msg[i] != ' ' && i < msg.size(); i++)
			;
	for (; msg[i] == ' ' && i < msg.size(); i++)
		;
	for (; msg[i] != ' ' && i < msg.size(); i++)
	{
		if (!isalpha(msg[i]))
			throw "command parsing error";
		command += msg[i];
	}
	while (i < msg.size())
	{
		for (; msg[i] == ' ' && i < msg.size(); i++)
			;
		if (msg[i] == ':')
		{
			std::string trailing(msg.begin() + i, msg.end());
			params.push_back(trailing);
			break;
		}
		else if (msg[i])
		{
			std::string param;
			for (; msg[i] != ' ' && i < msg.size(); i++)
				param += msg[i];
			params.push_back(param);
		}
	}
}

std::string Message::totxt() const
{
	if (command.empty())
		return "\r\n";
	std::stringstream txt;
	if (!prefix.empty())
		txt << prefix << ' ';
	txt << command;
	for (unsigned i = 0; i < params.size(); i++)
		txt << ' ' << params[i];
	txt << "\r\n";
	return txt.str();
}

Message &Message::setPrefix(const User &usr)
{
	prefix = ':' + usr.nickname + '!' + usr.username + '@' + usr.hostname;
	return *this;
}

Message::Message(const int ncmd)
{
	std::stringstream cmd;
	cmd.width(3);
	cmd.fill('0');
	cmd << ncmd;
	command = cmd.str();
	prefix = ":irc.42.com";
}

Message &Message::addParam(const std::string &prm)
{
	params.push_back(prm);
	return *this;
}

Message::Message()
{
	prefix = ":irc.42.com";
}
