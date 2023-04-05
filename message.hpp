#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include <cstring>
#include <iostream>
#include <map>
#include <netinet/in.h>
#include <set>
#include <string>
#include <sys/fcntl.h>
#include <sys/poll.h>
#include <unistd.h>
#include <vector>

#define ERR_UNKNOWNCOMMAND(cmd) \
	Message(421).addParam(cmd).addParam(":Unknown command")

#define QUIT(usr)                                         \
	Message().setPrefix(usr).setCommand("QUIT").addParam( \
		":User quit unexpectedly")

class User;
struct Message
{
	std::string command;
	std::string prefix;
	std::string totxt() const;
	std::vector<std::string> params;

	friend class Server;
	Message &addParam(const std::string &prm);
	Message &setCommand(const std::string &cmd);
	Message();
	Message(const int fd);
	Message(std::string &msg);
	Message &setPrefix(const User &usr);
};

#endif