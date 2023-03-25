#ifndef MAIN_HPP
#define MAIN_HPP

#include <map>
#include <netinet/in.h>
#include <set>
#include <string>
#include <vector>

class User
{
	std::string name;
};

class Message
{
	friend class Server;
	std::string command;
	std::string params;
	std::string prefix;
	Message(const std::string &msg);
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

	void execute_message(const Message &msg);
	void read_sock(std::vector<pollfd>::const_iterator &con);
	void register_sock();

  public:
	~Server();
	Server(const int port, const std::string &name);
	void event_loop();
};

#endif