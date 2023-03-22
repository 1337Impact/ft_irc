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
    unsigned addr_size = sizeof their_addr;
    int new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size);
    char buff[100];
    while (1){
        bzero(buff, 100);
        if (read(new_fd, buff, 100) == 0)
            exit(0);
        printf("%s", buff);
        bzero(buff, 100);
        read(0, buff, 100);
        write(new_fd, buff, 100);
    }

}