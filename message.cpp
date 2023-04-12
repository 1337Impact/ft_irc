#include "ircserv.hpp"
#include <algorithm>
#include <sstream>

Message &Message::setCommand(const std::string &cmd)
{
	command = cmd;
	return *this;
}

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
			std::string trailing(msg.begin() + i + 1, msg.end());
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
	std::stringstream txt;
	if (!prefix.empty())
		txt << prefix << ' ';
	txt << command;
	for (unsigned i = 0; i < params.size() - 1; i++)
		txt << ' ' << params[i];
	if (!params.empty())
	{
		txt << ' ';
		if (find(params.back().begin(), params.back().end(), ' ') !=
			params.back().end())
			txt << ':';
		txt << params.back();
	}
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
