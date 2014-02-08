#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/select.h>
#define BUFLEN 1500

int main()
{
    int fd;
    struct sockaddr_in6 addr;
    ssize_t i, rcount;
    char buf[BUFLEN];

    

    fd = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
    if (fd == -1) {
        printf("unable to create socket\n");
        return 1;
    }

    memset(&addr,0,sizeof addr);  

    addr.sin6_addr =  in6addr_any;
    addr.sin6_family= AF_INET6;
    addr.sin6_port= htons(5000);

    if (bind(fd, (struct sockaddr *) &addr,sizeof(addr)) == -1) {
        printf("unable to bind socket\n");
	exit(1);
    }

    /* Prepare to listen for 1 incoming connection */
    if (listen(fd, 10) == -1) {
        printf("error when listening for connection\n");
    }
   

    struct sockaddr_in6 client_addr;
    socklen_t client_addrlen = sizeof(client_addr);
     memset(&addr, 0, sizeof client_addr);

    int confd;
    for(;;) {
        printf("waiting for connections\n");
        if ((confd = accept(fd, (struct sockaddr *) &client_addr, &client_addrlen)) == -1) {
            printf("unable to accept incomming addr\n");
            return 1;
        }
        
        char client_ip[INET6_ADDRSTRLEN];
        inet_ntop(AF_INET6, &(client_addr.sin6_addr), client_ip, INET6_ADDRSTRLEN);
        printf("accepted connection from %s\n", client_ip);

              
        rcount = read(confd, buf, BUFLEN);
           if (rcount == -1) {
            printf("%d\n",errno);
            printf("unable to read from client socket\n");
        }
	
	 printf( "Read message from client:\n");
         for (i = 0; i < rcount && buf[i]; i++) {
            	printf( "%c", buf[i]);
         }
	 printf( "\n\n");

	
        
        printf("Sending message back to client\n");
        char hello[] = "hello to you";
        rcount = write(confd, hello, strlen(hello));
        printf("\nclosing connection to client\n\n");
	close(confd);
    
    }

    close(fd);
    return 0;

}

