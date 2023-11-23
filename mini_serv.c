#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

typedef struct clients{
    int id;
    char msg[200100];
}t_clients;

t_clients clients[1024];
char buff[200100];
int max, ids = 0, sockfd = 0;
fd_set set, rd, wr;

void err(char *str)
{
    write(2, str, strlen(str));
    exit(1);
}

void sendit(int s)      
{
    for (int fd = 0; fd <= max; fd++){     
        if (fd != s && FD_ISSET(fd, &wr))
            send(fd, buff, strlen(buff), 0);
    }
}

int main(int ac, char **av)
{
    if (ac != 2)
        err("Wrong number of arguments\n");

    bzero(&clients, sizeof(clients));
    sockfd = socket(AF_INET, SOCK_STREAM, 0);   
    if (sockfd < 0)
        err("Fatal error\n");

    struct sockaddr_in addr, cli;
    bzero(&addr, sizeof(addr));     
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(2130706433);
    addr.sin_port = htons(atoi(av[1]));

    if (bind(sockfd, (const struct sockaddr *) &addr, sizeof(addr)) < 0)
        err("Fatal error\n");
    if (listen(sockfd, 128) < 0)
        err("Fatal error\n");

    int connfd;
    socklen_t len = sizeof(cli);
    max = sockfd;
    FD_ZERO(&set);
    FD_SET(sockfd, &set);       

    while(1)
    {
        rd = wr = set; 
        if (select(max + 1, &rd, &wr, 0, 0) < 0)        
            continue;
        for (int fd = 0; fd <= max; fd++){
            if (FD_ISSET(sockfd, &rd) && fd == sockfd)
            {
                connfd = accept(sockfd, (struct sockaddr *) &cli, &len);        
                if (connfd < 0)
                    continue;
                FD_SET(connfd, &set);       
                sprintf(buff, "server: client %d just arrived\n", ids);
                sendit(connfd);
                clients[connfd].id = ids++;
                if (connfd > max)
                    max = connfd;
                break;
            }
            if (FD_ISSET(fd, &rd) && fd != sockfd)
            {      
                int r = 1;
                bzero(&clients[fd].msg, sizeof(clients[fd].msg));
                while (r == 1 && clients[fd].msg[strlen(clients[fd].msg) - 1] != '\n')      
                    r = recv(fd, clients[fd].msg + strlen(clients[fd].msg), 1, 0);      
                if (r < 1){        
                    sprintf(buff, "server: client %d just left\n", clients[fd].id);
                    FD_CLR(fd, &set);       
                    close(fd);
                }
                else
                    sprintf(buff, "client %d: %s", clients[fd].id, clients[fd].msg);
                sendit(fd);
                break;
            }
        }
    }
}