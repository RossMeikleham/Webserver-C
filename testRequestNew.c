/*
 * =====================================================================================
 *
 *       Filename:  testRequestNew.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  05/02/14 11:45:04
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */

#include "newRequest.h"

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
#include <pthread.h>
#define BUFLEN 1500

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

void* client_thread(void *connection) {
    int confd = *(int*)(connection);
    int rcount;
    char type[2048]; 
    char *ret;
    long size;

    for(;;) {

        printf("confd recieved %d\n", confd);  
        rcount = read(confd, type, 2047);
           if (rcount == -1) {
            printf("%d\n",errno);
            printf("unable to read from client socket\n");
            close(confd);
            return NULL;
        }
         if(rcount == 0) {
            printf("client closed socket\n");
            close(confd);
            return NULL;
        }

        type[2048] = '\0';
        printf("Client message %s\n",type);
        
	
        ret = request(type, &size);
         
    printf("Sending message back to client\n");
        printf("ret %s\n",ret);
        rcount = write(confd, ret, size);
        printf("sent \n");
        if(rcount == -1) {
            printf("error writing to socket\n");
            close(confd);
            return NULL;
        }
        if(rcount == 0) {
            printf("client closed socket\n");
            close(confd);
            return NULL;
        }

    }
   
    return NULL;
}

int main()
{
    int fd;
    struct sockaddr_in6 addr;
    pthread_t client_t;
    

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
    printf("confd found %d\n", confd);
    pthread_create (&client_t, NULL, client_thread , (void *)(&confd)); 

    }
    close(fd);
    return 0;
}






