#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>

int main()
{
    

    int sockfd, port;
    struct sockaddr_in my_addr;
    struct sockaddr_storage their_addr;
    port = 9991;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(port);


    bind(sockfd, (struct sockaddr *)&my_addr, sizeof(my_addr));
    listen(sockfd, 1);


    while (1){

        unsigned addr_size = sizeof their_addr;
        int new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size);
        for (int i = 0; i < len; i++)
        {
            char buff[100];
            read(new_fd, buff, 100);
            printf("%s\n", buff);

        }
    }
}