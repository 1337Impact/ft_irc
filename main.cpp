#include <arpa/inet.h>
#include <cstdio>
#include <iostream>
#include <sys/fcntl.h>
#include <sys/poll.h>
#include <unistd.h>
#include <vector>
#include <map>
#include <string>

int main()
{
	int sockfd, port;
	struct sockaddr_in my_addr;
	struct sockaddr_storage their_addr;
	port = 9991;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	fcntl(sockfd, F_SETFL, O_NONBLOCK);

	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(port);

	bind(sockfd, (struct sockaddr *)&my_addr, sizeof(my_addr));
	listen(sockfd, 1);

	std::vector<struct pollfd> cons(
		1, (struct pollfd){.fd = sockfd, .events = POLLIN});
	std::map<int, std::string> reqs;

	for (int evts; (evts = poll(cons.data(), cons.size(), -1));)
	{
		if (evts == -1)
		{
			perror("poll");
			break;
		}
		if (cons.front().revents & POLLIN)
		{
			unsigned addr_size = sizeof their_addr;
			struct pollfd con = {.events = POLLIN};
			con.fd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size);
			if (con.fd == -1)
			{
				perror("accept");
				break;
			}
			cons.push_back(con);
			evts--;
		}
		for (std::vector<struct pollfd>::const_iterator con = cons.cbegin() + 1;
			 evts && con != cons.cend();
			 con++)
		{
			if (con->revents & POLLIN)
				continue;
			evts--;
			for (char buf[512];;)
			{
				bzero(buf, sizeof buf);
				int nbytes = recv(con->fd, buf, sizeof buf, 0);
				if (nbytes <= 0)
				{
					if (nbytes == -1)
						perror("recv");
					close(con->fd);
					cons.erase(con, con + 1);
					break;
				}
				if (buf[nbytes - 1] == '\n')
				{
					reqs[con->fd].append(buf);
					for (std::vector<struct pollfd>::const_iterator oth =
							 cons.cbegin() + 1;
						 con != cons.cend();
						 con++)
						if (send(con->fd,
								 reqs[con->fd].data(),
								 reqs[con->fd].size(),
								 0) == -1)
							perror("send");
                    reqs.erase(con->fd);
					break;
				}
				if (nbytes < 512)
				{
					reqs[con->fd].append(buf);
					break;
				}
			}
		}
	}
	for (std::vector<struct pollfd>::const_iterator con = cons.cbegin();
		 con != cons.cend();
		 con++)
		close(con->fd);
}