/*
 * =====================================================================================
 *
 *       Filename:  http_server.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ross Meikleham 1107023m 
 *   Organization:  
 *
 * =====================================================================================
 */

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
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <strings.h>

#include "request.h"
#include "circular_int_queue.h"

#define BUFLEN 1500
#define MAX_THREADS 10 /*Max number of clients which can be
                         concurrently served */
#define REQUEST_MAX 8096
#define URL_MAX_LENGTH 2048
#define LINE_MAX 2048
#define PORT 8080




void* client_thread(void * client_queue) {

    int confd;
    int rcount;
    char type[2049]; 

    queue *cq = (queue *)client_queue;

    for(;;) {
        printf("attempting to get item\n");
        confd = pop_from_queue(cq);
        printf("thread doing work\n");

        for(;;) {

            printf("confd recieved %d\n", confd);  

            rcount = read(confd, type, 2048);
            printf("done recieving %d bytes from client\n",rcount);
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

                type[rcount] = '\0';
                //printf("Client message %s\n",type);
                
	
                rcount = request(type, confd);
                if(rcount <= 0)  {
                    close(confd); 
                    break;
                }
              }

        }
        printf("thread finished work\n");
     
     return NULL;
}


int main()
{
    int fd;
    struct sockaddr_in6 addr;
    int t_count;
    queue *client_queue;

    pthread_t threads[MAX_THREADS];
    

    fd = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
    if (fd == -1) {
        printf("unable to create socket\n");
        return 1;
    }

    memset(&addr,0,sizeof addr);  

    addr.sin6_addr =  in6addr_any;
    addr.sin6_family= AF_INET6;
    addr.sin6_port= htons(PORT);

    if (bind(fd, (struct sockaddr *) &addr,sizeof(addr)) == -1) {
        printf("unable to bind socket\n");
	exit(1);
    }

    client_queue = initialize_queue();
    for(t_count = 0; t_count < MAX_THREADS; t_count++) {
       pthread_create (threads+t_count, NULL, client_thread , client_queue); 

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
        push_to_queue(client_queue, confd);
   
    }

    printf("wtf\n");
    close(fd);
    return 0;
}






