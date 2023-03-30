#include "message.hpp"
#include <sstream>
#include <string>
#include "ircserv.hpp"

Message::Message(std::string &msg)
{
	std::vector<std::string> splited_msg;
	// split mgs
	std::stringstream ss(msg);
	std::string word;
	while (ss >> word)
	{
		splited_msg.push_back(word);
		// std::cout << word << std::endl;
	}
	if (splited_msg[0][0] == '!')
	{
		prefix = splited_msg[0];
		command = splited_msg[1];
		params.assign(splited_msg.begin() + 2, splited_msg.end());
	}
	else
	{
		command = splited_msg[0];
		params.assign(splited_msg.begin() + 1, splited_msg.end());
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

Message &Message::addPrefix(const User& usr)
{
	prefix = usr.nickname;
	return *this;
}

Message::Message(const int ncmd)
{
	std::stringstream cmd;
	cmd << ncmd;
	command = cmd.str();
}

Message &Message::addParam(const std::string &prm)
{
	params.push_back(prm);
	return *this;
}

Message::Message()
{
}

// int main()
// {
//     std::string par("cmd param1 param2");
//     Message msg(par);

//     std::cout << msg.command << std::endl;
//     std::cout << msg.params[0] << std::endl;
//     std::cout << msg.params[1] << std::endl;
// }