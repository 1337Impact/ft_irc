#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include <map>
#include <netinet/in.h>
#include <set>
#include <string>
#include <vector>
#include <iostream>
#include <cstring>
#include <sys/fcntl.h>
#include <sys/poll.h>
#include <unistd.h>

struct Message
{
	// friend class Server;
	std::string prefix;
	std::string command;
	std::vector<std::string> params;
	Message(std::string &msg);
};

#endif