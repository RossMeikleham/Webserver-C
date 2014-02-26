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

#define LISTEN_MAX 20
#define MAX_THREADS 10 /*Max number of clients which can be
                         concurrently served */




void* client_thread(void * client_queue) {

    int confd;
    status res = OK;
    char* message = NULL;

    queue *cq = (queue *)client_queue;

    for(;;) {
        confd = pop_from_queue(cq);

        while(res != CONNECTION_ERROR) {

            printf("confd recieved %d\n", confd);  

            if((res = obtain_request(&message, confd)) != OK) {
                if(res != CONNECTION_ERROR) {
                   res = send_error(res, confd);
                }
            } else { 
                 printf("message: %s\n",message);
                 if((res = respond_to(message, confd)) != OK) {
                 if(res != CONNECTION_ERROR) {
                   res = send_error(res, confd);
                 }
              }
            }
        }

        printf("Either unable to read from client socket, or client closed socket\n");
        close(confd);
        return NULL;

    }
        
}



int main()
{
    int fd, t_count, confd;
    struct sockaddr_in6 addr, client_addr;
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
	    return 1;
    }

    client_queue = initialize_queue();
    for(t_count = 0; t_count < MAX_THREADS; t_count++) {
       pthread_create (threads+t_count, NULL, client_thread , client_queue); 

    }
   
    /* Prepare to listen for incoming connections */  
    if (listen(fd, LISTEN_MAX) == -1) {
        printf("error when listening for connection\n");

    }
   

    socklen_t client_addrlen = sizeof(client_addr);
    memset(&addr, 0, sizeof client_addr);

   
    /* Loop round accepting connections from clients and placing them
     * on the work queue, so each consumer thread can take one when they
     * are ready  */
    for(;;) {

        if ((confd = accept(fd, (struct sockaddr *) &client_addr, &client_addrlen)) == -1) {
            printf("unable to accept incomming addr\n");
            return 1;
        }
        push_to_queue(client_queue, confd);
   
    }
    

    close(fd);
    return 0;
}






