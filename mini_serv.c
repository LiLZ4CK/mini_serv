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
    // Send a message to all clients except the 's'
void sendit(int s)      
{
    for (int fd = 0; fd <= max; fd++){     // Iterate through all file descriptors and send the message to those ready for writing
        if (fd != s && FD_ISSET(fd, &wr))
            send(fd, buff, strlen(buff), 0);
    }
}

int main(int ac, char **av)
{
    if (ac != 2)
        err("Wrong number of arguments\n");
        // Initialize clients array to zero
    bzero(&clients, sizeof(clients));
        // Create a socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);   
    if (sockfd < 0)
        err("Fatal error\n");
        // Initialize the server address structure
    struct sockaddr_in addr, cli;
    bzero(&addr, sizeof(addr));     
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(2130706433);
    addr.sin_port = htons(atoi(av[1]));
        // Bind the socket to the specified address
    if (bind(sockfd, (const struct sockaddr *) &addr, sizeof(addr)) < 0)
        err("Fatal error\n");
        // Listen for incoming connections
    if (listen(sockfd, 128) < 0)
        err("Fatal error\n");
    max = sockfd;
    int connfd;
    socklen_t len = sizeof(cli);
        // Initialize the set of file descriptors
    FD_ZERO(&set);
        // Add the server socket to the set
    FD_SET(sockfd, &set);       

    while(1)
    {
            // Copy the set of file descriptors to read and write sets
        rd = wr = set; 
            // Use select to monitor file descriptors for read and write event
        if (select(max + 1, &rd, &wr, 0, 0) < 0)        
            continue;
            // Iterate through all file descriptors
        for (int fd = 0; fd <= max; fd++){
                // Check if the server socket is ready for reading and it is the current file descriptor
            if (FD_ISSET(sockfd, &rd) && fd == sockfd){
                    // Accept a new connection
                connfd = accept(sockfd, (struct sockaddr *) &cli, &len);        
                if (connfd < 0)
                    continue;
                    // Add the new connection to the set
                FD_SET(connfd, &set);       
                sprintf(buff, "server: client %d just arrived\n", ids);
                    // Send the message to all other clients
                sendit(connfd);
                    // Assign the next ID to the new client
                clients[connfd].id = ids++;
                    // Update the maximum file descriptor
                if (connfd > max)
                    max = connfd;
                break;
            }
                // Check if a client socket is ready for reading and it is not the server socket
            if (FD_ISSET(fd, &rd) && fd != sockfd){      
                int r = 1;
                    // Clear the client's message buffer
                bzero(&clients[fd].msg, sizeof(clients[fd].msg));
                    // Receive data from the client until a newline character is encountered
                while (r == 1 && clients[fd].msg[strlen(clients[fd].msg) - 1] != '\n')      
                    r = recv(fd, clients[fd].msg + strlen(clients[fd].msg), 1, 0);      
                    // Check for errors or client disconnection
                if (r < 1){        
                    sprintf(buff, "server: client %d just left\n", clients[fd].id);
                    // Remove the client from the set and close the connection
                    FD_CLR(fd, &set);       
                    close(fd);
                }else
                    sprintf(buff, "client %d: %s", clients[fd].id, clients[fd].msg);
                sendit(fd);
                break;
            }
        }
    }
}