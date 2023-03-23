#ifndef MAIN_HPP
#define MAIN_HPP

#include <map>
#include <netinet/in.h>
#include <set>
#include <string>
#include <vector>

class Channel
{

};

class Server
{
	typedef struct pollfd pollfd;
	typedef struct sockaddr sockaddr;
	typedef struct sockaddr_in sockaddr_in;
	typedef struct sockaddr_storage sockaddr_storage;

	int sockfd;
	sockaddr_in addr;
	std::map<int, std::string> bufs;
	std::set<Channel> channels;
	std::string pass;
	std::vector<pollfd> cons;
	sockaddr_storage oth_addr;

	void register_sock();
	void read_sock(std::vector<pollfd>::const_iterator &con);

  public:
	~Server();
	Server(const int port, const std::string &name);
	void event_loop();
};

#endif