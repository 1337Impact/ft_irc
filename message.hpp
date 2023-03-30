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

class User;
struct Message
{
	std::string command;
	std::string prefix;
	std::string totxt() const;
	std::vector<std::string> params;

	friend class Server;
	Message &addParam(const std::string &prm);
	Message();
	Message(const int fd);
	Message(std::string &msg);
	Message &addPrefix(const User& usr);
};

#endif