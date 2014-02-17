  #include <sys/types.h>
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <netdb.h>
  #include <arpa/inet.h>
  #include <stdio.h>
  #include <stdlib.h>
  #include <string.h>
  #include <unistd.h>
  #include <errno.h>
  #define BUFLEN 1500


/* Attempt to connect to server via DNS */
int connectDNS(char *dns, char *port) 
{
	struct addrinfo hints, *ai, *ai0;
	int i, fd;

	memset(&hints, 0, sizeof(hints));
    	hints.ai_family = PF_UNSPEC;
    	hints.ai_socktype = SOCK_STREAM;  

	if ((i = getaddrinfo(dns, port, &hints, &ai0)) != 0) {
    		printf("Unable to look up IP address: %s", gai_strerror(i));
		    return -1;
	}

	for (ai = ai0; ai != NULL; ai = ai->ai_next) {
		fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
	    if (fd == -1) {
		    perror("Unable to create socket");
		    continue;
	    }
	    if (connect(fd, ai->ai_addr, ai->ai_addrlen) == -1) {
		    perror("Unable to connect");
		    close(fd);
		    continue;
	    }
	    //...success, use the connection
	    return fd;
    }

	if (ai == NULL) {
   	// Connection failed
   	    printf("Connection failed\n");
	    return -1;
   	}	
    
    else {
       printf("this should never ever happen");	
       return -1;
    }

}



int main(int argc, char *argv[])
{
    int fd, i;
    char *dns, buffer[BUFLEN];
    char buffer2[1000];
	
   if(argc != 2) {
	printf("Error, usage: client [server]");
	return 1;
   }

   

    dns = argv[1];
    if ((fd = connectDNS(dns, "5000")) == -1)
	return 1;
    
    
    char sendMessage[] = "GET /index.html HTTP/1.1\r\n\r\n";
    printf("SENDING THIS PIECE OF SHIT\n");
    if ((write(fd, sendMessage, strlen(sendMessage)))==-1) {
        printf("Error writing to server\n");
        printf("%d\n", errno);
        close(fd);
        return 1;
    }

    printf("reading from server:\n");

    int rcount = read(fd, buffer2, 1000);
     if (rcount == -1) {
        printf("%d\n",errno);
        printf("unable to read from socket\n");
        close(fd);
        return 1;
    }
    
    printf("rcount %d\n",rcount);
    for (i = 0; i < rcount; i++) {
        printf("%c", buffer2[i]);
    }
    printf("\n");
    
    
    
    close(fd);
    return 0;
}

