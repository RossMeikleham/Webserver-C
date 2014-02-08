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
#include <time.h>
#include <string.h>
#define BUFLEN 2048

void get_current_date(char *str) {
    time_t current_time;
    struct tm* tm_info;

    time(&current_time);
    tm_info = localtime(&current_time);
    strftime(str, 25, "%Y:%m:%d", tm_info);
    str[10] = '\r';
    str[11] = '\n';
    str[12] = '\0';

}

void get_current_time(char *str) {
    time_t current_time;
    struct tm*  tm_info;

    time(&current_time);
    tm_info = localtime(&current_time);
    strftime(str, 25, "%H:%M:%S", tm_info);
    str[8] = '\r';
    str[9] = '\n';
    str[10] = '\0';
}


int main()
{
    int fd;
    struct sockaddr_in6 addr;
    ssize_t i, rcount;
    char buf[BUFLEN+1];

    

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

        for(;;) {
        
        char client_ip[INET6_ADDRSTRLEN];
        inet_ntop(AF_INET6, &(client_addr.sin6_addr), client_ip, INET6_ADDRSTRLEN);
        printf("accepted connection from %s\n", client_ip);


        rcount = read(confd, buf, BUFLEN);
           if (rcount == -1) {
            printf("%d\n",errno);
            printf("unable to read from client socket\n");
            close(confd);
            break;
        }
         if(rcount == 0) {
            printf("client closed socket\n");
            close(confd);
            break;
        }

        buf[BUFLEN] = '\0'; //Add terminating char
        printf("rcount %d\n", rcount);
	
	 printf( "Read message from client:\n");
         for (i = 0; i < 6; i++) {
            	type[i] = buf[i];
         }
         type[6] = '\0';
	 
     Request request;
     char** words;
     long size;
     if((words = spaces(buf, &size)) != NULL) {
         
     }

     if(strcmp(type, "GET\r\n") == 0) {
         printf("return time\n");
         get_current_time(returnStr);
     }

     else if(strcmp(type, "DATE\r\n") == 0) {
        printf("return date\n");
        get_current_date(returnStr);
     }

	
        
        printf("Sending message back to client\n");
        rcount = write(confd, returnStr, strlen(returnStr));
        if(rcount == -1) {
            printf("error writing to socket\n");
            close(confd);
            break;
        }
        if(rcount == 0) {
            printf("client closed socket\n");
            close(confd);
            break;
        }
        

        printf("Wrote to socket\n");
    }
    
    }

    close(fd);
    return 0;

}

