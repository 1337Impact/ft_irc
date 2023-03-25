#ifndef IRCSERV_HPP
#define IRCSERV_HPP

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

class User
{
	std::string name;
};

class Message
{
	friend class Server;
	std::string prefix;
	std::string command;
	std::vector<std::string> params;
	Message(const std::string &msg)
	{
		std::vector<std::string> splited_msg;
		// split mgs
		char *tmp =  strtok(msg, " ");
		while(tmp != NULL)
		{
			tmp = strtok(NULL, "");
			splited_msg.push_back(tmp);
		}
		if (splited_msg[0][0] == '!'){
			prefix = splited_msg[0];
			command = splited_msg[1];
			params.assign(splited_msg.begin() + 2, splited_msg.end());
		}
		else
		{
			command = splited_msg[0];
			params.assign(splited_msg.begin() + 2, splited_msg.end());
		}
	}
};

class Server
{
	typedef struct pollfd pollfd;
	typedef struct sockaddr sockaddr;
	typedef struct sockaddr_in sockaddr_in;
	typedef struct sockaddr_storage sockaddr_storage;

	int sockfd;
	sockaddr_in addr;
	sockaddr_storage oth_addr;
	std::map<int, std::string> bufs;
	std::set<User> users;
	std::string pass;
	std::vector<pollfd> cons;

	void execute_message(const Message &msg){std::cout << msg.command << std::endl;}
	void read_sock(std::vector<pollfd>::const_iterator &con);
	void register_sock();

  public:
	~Server();
	Server(const int port, const std::string &name);
	void event_loop();
};

#endif