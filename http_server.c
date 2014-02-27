/*  HTTP Server Implementation in C
 *  Ross Meikleham 1107023m */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <strings.h>
#include <pthread.h>


#define URL_MAX_LENGTH 2048
#define LINE_MAX 1024    
#define LISTEN_MAX 20 /*  Max number of listeners */
#define MAX_THREADS 8 /*Max number of clients which can be concurrently served */
#define MAX_QUEUE_SIZE  2 * MAX_THREADS
#define PORT 8080 /* Port to connect to */



typedef enum {NOT_FOUND, BAD_REQUEST, OK, SERVER_ERROR, CONNECTION_ERROR} status;


/*  Map of file extensions to their respective content type */
typedef struct {
    char *extention; 
    char *content_type;
} content_map;


content_map contents[] = { {".htm","text/html"}, {".html","text/html"},
    {".txt","text/plain"}, {".jpg","image/jpeg"}, {".jpeg","image/jpeg"},
    {".gif","image/gif"}, {NULL, "application/octet-stream"}};


/*  Circular int bounded buffer queue */
typedef struct int_queue {
    
    int items[MAX_QUEUE_SIZE];
    unsigned long head;
    unsigned long tail;
    unsigned long size;

} queue;


pthread_mutex_t pool_mutex = PTHREAD_MUTEX_INITIALIZER; /* thread pool mutex */
pthread_cond_t condc, condp; /*  Consumer/Producer condition variables */



/*  Obtain a client from the waiting client queue*/
int dequeue(queue *q) {
    int confd;
    pthread_mutex_lock(&pool_mutex);

    while (q->size <= 0) {
        pthread_cond_wait(&condc, &pool_mutex);
    }
    /* Move queue head forward, 
     * wrap round to the start if at end */
    confd = q->items[q->head];
    q->head += 1;
    q->head %= MAX_QUEUE_SIZE;
    q->size--;

    pthread_cond_signal(&condp);
    pthread_mutex_unlock(&pool_mutex);

    return confd;
}



/*  Add a waiting client to the queue */
void enqueue(queue *q, int item) 
{
    
    pthread_mutex_lock(&pool_mutex);

    while (q->size >= MAX_QUEUE_SIZE) {
        pthread_cond_wait(&condp, &pool_mutex);
    }
    /* Move queue tail forward,
     * wrap round to the start if at end */
    q->tail += 1;
    q->tail %= MAX_QUEUE_SIZE;
    q->size ++;

    q->items[q->tail] = item;

    pthread_cond_signal(&condc);
    pthread_mutex_unlock(&pool_mutex);
}




/* Returns a pointer to the content type of the 
 * specified resource, using its extension.
 * Returns txt/html if no extension or
 * octet-stream type if unrecognized extension */
const char *get_content_type(const char *resource)
{  
    char* extention; 
    content_map *type;

    if ((extention = strrchr(resource, '.')) == NULL) {
        return contents[0].content_type; /* No extention default is text/html */
    }
    type = contents;

    for(type = contents ;type->extention != NULL; type++) {
        if (strcmp(extention, type->extention) == 0) {
            return type->content_type;
        }
    }
    /*  is unknown we return application/octet stream */
    return type->content_type;   
}




/*  Send a "404 Not Found" response
 *  back to the client, returns the result
 *  of the last write */
status send_not_found_response(int confd)
{ 
      char content[] =  
          "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\"\r\n"
          "\t\"http://www.w3.org/TR/html4/strict.dtd\">"
          "<html>\r\n" 
          "<head>\r\n" 
          "<title> 404 Not Found </title>\r\n" 
          "</head>\r\n" 
          "<body>\r\n" 
          "<p> The requested file cannot be found. </p>\r\n" 
          "</body>\r\n" 
          "</html>";

      char headers[] ="HTTP/1.1 404 Not Found\r\n" 
          "Content-Type: text/html \r\n" 
          "Content-Length: 219\r\n\r\n";
      
    write(confd, headers, strlen(headers));
    return write(confd, content, strlen(content)) > 0 ? OK : CONNECTION_ERROR;

}


/*  Send a "400 Bad Request" response
 *  back to the client, returns the result
 *  of the last write */
status send_bad_request_response(int confd)
{
    char content[] =  
          "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\"\r\n"
          "\t\"http://www.w3.org/TR/html4/strict.dtd\">"
          "<html>\r\n" 
          "<head>\r\n" 
          "<title> 400 Bad Reqest</title>\r\n" 
          "</head>\r\n" 
          "<body>\r\n" 
          "<p> Bad Request. </p>\r\n" 
          "</body>\r\n" 
          "</html>";

      char headers[] ="HTTP/1.1 400 Bad Request\r\n" 
          "Content-Type: text/html \r\n" 
          "Content-Length: 196\r\n\r\n";

     
    write(confd, headers, strlen(headers));
    return write(confd, content, strlen(content)) > 0 ? OK : CONNECTION_ERROR; 

}



/*  Send a "500 Server Error" response
 *  back to the client, returns the result
 *  of the last write */
status send_server_error_response(int confd)
{    
      char content[] =  
          "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\"\r\n"
          "\t\"http://www.w3.org/TR/html4/strict.dtd\">"
          "<html>\r\n" 
          "<head>\r\n" 
          "<title> 500 Internal Server Error </title>\r\n" 
          "</head>\r\n" 
          "<body>\r\n" 
          "<p> There was an error with the server. </p>\r\n" 
          "</body>\r\n" 
          "</html>";

      printf("strlen:%lu\n",strlen(content));
      char headers[] ="HTTP/1.1 500 Internal Server Error\r\n" 
          "Content-Type: text/html \r\n" 
          "Content-Length: 231\r\n\r\n";

     
    write(confd, headers, strlen(headers));
    return write(confd, content, strlen(content)) > 0 ? OK : CONNECTION_ERROR;
     
}




/*  Attempt to send an open file through the given connection 
 *  returns  OK if successful, CONNECTION_ERROR if unsuccessful*/
status send_file(int confd, int fd, int size)
{
    int res = 1, wcount;
    char buf[LINE_MAX];

    /*  We paritally read from the file then send to save
     *  having to read the entire file into memory at once */    
    while ((wcount = read(fd, buf, LINE_MAX)) > 0 &&
        (res = write(confd, buf, wcount)) > 0) 
        ;;
 
   return res > 0 ? OK : CONNECTION_ERROR;

}



/* Send an Error Response back to the 
 * specified client */
status send_error(status s, int confd) 
{
    switch (s) {
        case BAD_REQUEST: return send_bad_request_response(confd);
        case SERVER_ERROR: return send_server_error_response(confd);
        case NOT_FOUND: return send_not_found_response(confd);
        /* Error-Ception, if it reaches here server has made
         * an error, so we send a server error response */
        default: return send_server_error_response(confd);
    }
}



/* Given the resource and socket, attempts
 * to locate the resource file and send it
 * down the connection, sends appropriate error
 * response if this fails. Returns OK if a response
 * is sent, CONNECTION_ERROR if unable to send
 * some sort of response back to the client */
status send_response(char *resource, int confd) {

    char *file_name,
          http_200_ok[] = "HTTP/1.1 200 OK \r\n",
          content_length[] = "Content-Length: ",
          content_type[] = "Content-Type: ",
          length_buffer[11];

    struct stat fs;
    int fd, file_size;
    status result;
    const char *content_type_header;
 
    file_name = resource;
    
    /*  Obtain file size to determine content length */
    if ((fd = open(file_name, O_RDONLY)) == -1) {
        return send_not_found_response(confd);
    }
    if ( fstat(fd, &fs) == -1) {
        close(fd);
        return send_not_found_response(confd);
    }
    
    file_size = fs.st_size;
   
    content_type_header = get_content_type(resource);

    /*  Convert content-length to string */
    snprintf(length_buffer, sizeof length_buffer, "%d", file_size);
    
    /*  Send headers information */
    write(confd, http_200_ok, strlen(http_200_ok));
    write(confd, content_length, strlen(content_length));
    write(confd, length_buffer, strlen(length_buffer));
    write(confd, "\r\n", 2);
    write(confd, content_type, strlen(content_type));
    write(confd, content_type_header, strlen(content_type_header));
    write(confd, "\r\n\r\n", 4);
    
    /*  Send the file over the socket */
    result = send_file(confd, fd, file_size);
    close(fd);
    return result;
    
}



/* Same as strsep except takes an entire string
 * delimiter instead of multiple char delimiters
 * as far as I'm aware there are no functions like
 * this in the standard c libraries */   
char *strsep_str(char **buf, const char *delimiter) {
    unsigned long count;
    char *start_token, *current, *end_token;
    
    while ((current = strstr(*buf, delimiter)) == *buf) {
        *buf = current;
    }
    start_token = *buf; 
    
    if (!(end_token = strstr(*buf, delimiter))) {
        *buf = NULL;
        return start_token;
    }
    
    for (count = 0; count < strlen(delimiter); count++) {
        *end_token++ ='\0'; 
    }

    *buf = end_token;
    return start_token;
}

   
    


/*  Parses a request line and stores details in given request
 *  returns status of parsing the line, should be OK
 *  if request line format is correct */
status parse_request_line(char *buf, char *resource)
{    
    char *temp_resource;
    
    resource[0] = '.'; /*  Need to prepend '.'  */
    
    /* Parse for header values, (Method, Resource, HTTP version) */
    if (strcmp(strsep(&buf, " "), "GET") != 0)
        return BAD_REQUEST;

    /*  Parse resource */
    temp_resource = strsep(&buf, " "); 
    strncpy(resource+1, temp_resource, strlen(temp_resource) +1);
   
    if (strcmp(buf, "HTTP/1.1") != 0) {
        return BAD_REQUEST;
    }
    
    return OK;    
}





/*  Reurn hostname or NULL if none if
 *  failure with fields */
status get_host(char *buf, char *header_store, unsigned long max_header_size) {

    char *line, *temp = NULL;
    
    /*  Split the first line */
    line = strsep_str(&buf, "\r\n");
    while(line){

     if(!strchr(line,':')) { /*  Badly formatted header line */
         return BAD_REQUEST;
     }    
     temp = strsep(&line, ":"); 

     if(!(strcasecmp(temp,"host"))) { 
        temp = line + strspn(line, " \t"); /*skip whitespace*/
        
        if(max_header_size < strlen(temp)+1) {/* too large */
            return BAD_REQUEST;
        }else {
            strncpy(header_store, temp, strlen(temp) + 1);
            return OK;
        }
     }
     
     line = strsep(&buf, "\r\n");

    }

    return BAD_REQUEST; /*  no host header defined */
    
}



status check_recieved_host(char *recieved_host) 
{
    char this_host[LINE_MAX],
         dcs_host[] = ".dcs.gla.ac.uk";

    /*  Split off port number if recieved host contains it*/
    recieved_host = strsep(&recieved_host,":");

    gethostname(this_host, LINE_MAX);
    if(gethostname(this_host, LINE_MAX) < 0) { /* Check gethostname successful */
        return SERVER_ERROR;
    }
    /*  Compare direct hostname with recieved host */
    if(!strcmp(this_host, recieved_host)) {
        return OK;
    }
    /*  Check enough space for appending the dcs url to host */
    if(strlen(dcs_host) + strlen(this_host) >= LINE_MAX) {
        return SERVER_ERROR;
    }
    strncat(this_host, dcs_host, strlen(dcs_host) + 1);
    if(!strcmp(this_host, recieved_host)) {
        return OK;
    }
    /*  HostName doesn't match at all */   
    return BAD_REQUEST;

}




/* Given a HTTP message and the size of the message 
 * Responds to the given socket with the approrpriate
 * HTTP response. The space allocated for message should
 * be at least 1 more than req_size */
status respond_to(char* buf, int confd)
{
    char host[LINE_MAX+1], resource[LINE_MAX+1],
         *resource_line, *headers, *current;
    int result;
    
    current = buf;
    /*  Cut off the Resource line */
    resource_line = strsep_str(&current, "\r\n"); 

    /*  Get the resource, if error with parsing request line, send 400 response */
    if((result = parse_request_line(resource_line, resource)) != OK) {
       return send_error(result, confd);
    }

    headers = strsep_str(&current, "\r\n\r\n");

     /* attempt to obtain hostname */
     if ((result = get_host(headers, host, LINE_MAX)) != OK) {
        return send_error(result, confd);
     }
    
     if ((result = check_recieved_host(host)) != OK) {
         return send_error(result, confd);
     }
           
    return send_response(resource, confd);

}



/*  Attempts to read in http request up until
 *  \r\n\r\n, result is stored in request_buf   
 *  result is OK if successfully obtained request
 *  result is SERVER_ERROR if failure to allocate memory for request
 *  result is CONNECTION_ERROR if failure to read */
status obtain_request(char **request_buf, int confd) 
{
    unsigned long size = 0;
    int count;
    char read_buf[LINE_MAX+1],
        *end, *temp;
    
    /*  If buf hasn't been allocated any mem it needs
     *  to be initialized with size of 1 and EOS placed in it
     *  so it can be reallocated and concatonated to*/
    if(!*request_buf && !(*request_buf = (char *)malloc(sizeof (char)))) {
        return SERVER_ERROR;
    }
    *(request_buf[0]) = '\0'; /* 'overwrite' previous string */
    
    for(;;) {
        if((count = read(confd, read_buf, LINE_MAX)) <= 0) {
            return CONNECTION_ERROR;
        }

        read_buf[count] = '\0';

        /*  If end of request is before end of buffer
         *  then we can just ignore the rest of the read buffer*/
        if((end = strstr(read_buf, "\r\n\r\n"))) {
            count = end - read_buf + 4;
        }

        size += count;
       
        /*Reallocate Size + 1 (space for EOS char ) to buf and  
         * concatonate new read into it*/
        if(!(temp = (char *)realloc(*request_buf, sizeof(char) * (size + 1)))) {
             return SERVER_ERROR;
        }
        *request_buf = temp;

        strncat(*request_buf, read_buf, count+1);
        if(strstr(*request_buf, "\r\n\r\n")) { /*  We have all we need */
            return OK;
        }
    }      

    return SERVER_ERROR; 
}




void* client_thread(void * client_queue) {

    int confd;
    status res;
    char* message = NULL;

    queue *cq = (queue *)client_queue;

    for(;;) {

        confd = dequeue(cq);
        printf("fd:%d\n",confd);

        /*  Serve client until they close connection, or
         *  there is an error when attempting to read/write to/from them */
        do {
            /*  Obtain the request message as a string ending in '\r\n\r\n */
            if((res = obtain_request(&message, confd)) != OK) {
                if(res != CONNECTION_ERROR) {
                   res = send_error(res, confd);
                }
            } else { /* If request message recieved successfully */
                res = respond_to(message, confd); 
            }
           
        } while (res != CONNECTION_ERROR);
        
        close(confd);       
    }
    /*  Should never get here, but if it does
     *  might as well avoid any memory leaks */
    free(message);
    return NULL;
        
}



int main()
{
    int fd, t_count, confd;
    struct sockaddr_in6 addr, client_addr;
    queue client_queue;
    
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

    client_queue.head = 0;
    client_queue.tail = MAX_QUEUE_SIZE-1;
    client_queue.size = 0;

    /*  Initialize the threads for the threadpool */
    for(t_count = 0; t_count < MAX_THREADS; t_count++) {
       if (pthread_create(threads+t_count, NULL, client_thread, &client_queue) != 0) {
           printf("failed to create thread %d\n",t_count+1);
           return 1;
       }
    }

   
    /* Prepare to listen for incoming connections */  
    if (listen(fd, LISTEN_MAX) == -1) {
        printf("error when listening for connection\n");
        return 1;;

    }
   
    socklen_t client_addrlen = sizeof(client_addr);
    memset(&addr, 0, sizeof client_addr);

   
    /* Loop round accepting connections from clients and placing them
     * on the work queue, so each consumer thread can take one when they
     * are ready */
    for(;;) {

        if ((confd = accept(fd, (struct sockaddr *) &client_addr, &client_addrlen)) == -1) {
            close(fd);
            return 1;
        }
        enqueue(&client_queue, confd);
   
    }
    
    close(fd);
    return 0;
}



