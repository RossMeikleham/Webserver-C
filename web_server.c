/*  HTTP Web Server implementation in C
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
#include <errno.h>


#define URL_MAX_LENGTH 2048
#define BUFLEN 1024    
#define LISTEN_MAX 20 /*  Max number of listeners */
#define MAX_THREADS 8 /*Max number of clients which can be concurrently served */
#define MAX_QUEUE_SIZE  2 * MAX_THREADS



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
    pthread_mutex_t lock;  
    pthread_cond_t condc, condp; /* Consumer/Producer condition variables*/

} queue;




/*  Obtain a client from the waiting client queue*/
int dequeue(queue *q) {
    int confd;
    pthread_mutex_lock(&(q->lock));

    while (q->size <= 0) {
        pthread_cond_wait(&(q->condc), &(q->lock));
    }
    /* Move queue head forward, 
     * wrap round to the start if at end */
    confd = q->items[q->head];
    q->head += 1;
    q->head %= MAX_QUEUE_SIZE;
    q->size--;

    pthread_cond_signal(&(q->condp));
    pthread_mutex_unlock(&(q->lock));

    return confd;
}



/*  Add a waiting client to the queue */
void enqueue(queue *q, int item) 
{
    
    pthread_mutex_lock(&(q->lock));

    while (q->size >= MAX_QUEUE_SIZE) {
        pthread_cond_wait(&(q->condp), &(q->lock));
    }
    /* Move queue tail forward,
     * wrap round to the start if at end */
    q->tail += 1;
    q->tail %= MAX_QUEUE_SIZE;
    q->size ++;

    q->items[q->tail] = item;

    pthread_cond_signal(&(q->condc));
    pthread_mutex_unlock(&(q->lock));
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



/* Attempts to write all bytes to the socket,
 * if for some reaon not all written at once attempts 
 * repeat writes to achieve this, returns size
 * if successful, otherwise 0 or less returned */
int write_all(int confd, const char *buf, size_t size) {
    size_t total;
    int wcount;

    for (total = wcount = 0; total < size; total += wcount) {
        if ((wcount = write(confd, buf+total, size-total)) <= 0) {
            return wcount; /*  Error */
        }
    }

    return total;
}

/* Attempts to send a file over the specified socket,
 * if all of the file is written successfully, no of
 * bytes sent returned */
int send_file_socket(int confd, int fd, size_t size) {
    char buf[BUFLEN];
    size_t total;
    int rcount;
    int wcount;

    /*  Attempt to read in up to BUFLEN amount of bytes
     *  and write them all to socket until entire file
     *  written */
    for(total = 0; total < size; total+=wcount) {
        if((rcount = read(fd, buf, BUFLEN)) <= 0) {
            return rcount;
        }
        if((wcount = write_all(confd, buf, rcount)) <= 0) {
            return wcount;
        }
    }
    return total; 
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
      
    write_all(confd, headers, strlen(headers));
    return write_all(confd, content, strlen(content)) > 0 ? OK : CONNECTION_ERROR;

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

     
    write_all(confd, headers, strlen(headers));
    return write_all(confd, content, strlen(content)) > 0 ? OK : CONNECTION_ERROR; 

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

     
    write_all(confd, headers, strlen(headers));
    return write_all(confd, content, strlen(content)) > 0 ? OK : CONNECTION_ERROR;
     
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
status send_response(const char *file_name, int confd) {

    char  http_200_ok[] = "HTTP/1.1 200 OK \r\n",
          content_length[] = "Content-Length: ",
          content_type[] = "Content-Type: ",
          length_buffer[11];

    struct stat fs;
    int fd, file_size, result;
    const char *content_type_header;
  
    /*  Simple check to avoid a directory traversal attack */
    if (strstr(file_name, "..")) {
        printf("Unable to find/open file %s requested by client %d\n", file_name, confd);
        return send_not_found_response(confd);
    }
    /*  Obtain file size to determine content length */
    if ((fd = open(file_name, O_RDONLY)) == -1) {
        printf("Unable to find/open file %s requested by client %d\n", file_name, confd);
        return send_not_found_response(confd);
    }

    if (fstat(fd, &fs) == -1) {
        printf("Error obtaining file size of file:%sfor client %d\n. %s\n",
            file_name, confd, strerror(errno));
        close(fd);
        return send_not_found_response(confd);
    }
    
    file_size = fs.st_size;
   
    content_type_header = get_content_type(file_name);

    /*  Convert content-length to string */
    snprintf(length_buffer, sizeof length_buffer, "%d", file_size);
    
    /*  Send headers information */
    write_all(confd, http_200_ok, strlen(http_200_ok));
    write_all(confd, content_length, strlen(content_length));
    write_all(confd, length_buffer, strlen(length_buffer));
    write_all(confd, "\r\n", 2);
    write_all(confd, content_type, strlen(content_type));
    write_all(confd, content_type_header, strlen(content_type_header));
    write_all(confd, "\r\n\r\n", 4);
    
    /* Attempt to send the entire file over socket*/
    result = send_file_socket(confd, fd, file_size);
    close(fd);
    if (result <= 0) {
        printf("Unable to write file:%s to client %d\n",file_name, confd);
        return CONNECTION_ERROR;
    }
    else if (result != file_size) {
        printf("Incorrect number of bytes written expected %d but was %d for"
        "file:%s requested by client %d\n",file_size, result, file_name, confd);
        return OK;
    } else {
        printf("File:%s successfully sent to client %d\n",file_name, confd);
        return OK;
    }
    
    return result;
}



/* Same functionality as strsep except takes an entire string
 * delimiter instead of multiple char delimiters
 * as far as I'm aware there are no functions like
 * this in the standard c libraries */   
char *strsep_str(char **buf, const char *delimiter) {
    unsigned long count;
    char *start_token, *current, *end_token;

    if(!buf) {
        return NULL;
    }
    
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
    while (line) {
        
        if(!strchr(line,':')) { /*  Badly formatted header line */
            return BAD_REQUEST;
        }
                    
        temp = strsep(&line, ":"); 

        if (!(strcasecmp(temp,"host"))) { /*  host header found */
            temp = line + strspn(line, " \t"); /*skip whitespace*/
        
            if (max_header_size < strlen(temp)+1) {/* too large */
                return BAD_REQUEST;
            }else {
                strncpy(header_store, temp, strlen(temp) + 1);
                return OK;
            }
        }
        
        printf("buf:%s\n",buf);
        line = strsep_str(&buf, "\r\n");

    }

    return BAD_REQUEST; /*  no host header defined */
}


/* Given a HTTP message and the size of the message 
 * Responds to the given socket with the approrpriate
 * HTTP response. The space allocated for message should
 * be at least 1 more than req_size */
status respond_to(char* buf, int confd)
{
    char resource[BUFLEN+1],
         *resource_line, *headers, *current;
    int result;
    
    current = buf;
    /*  Cut off the Resource line */
    resource_line = strsep_str(&current, "\r\n"); 

    /*  Get the resource, if error with parsing request line, send 400 response */
    if ((result = parse_request_line(resource_line, resource)) != OK) {
        printf("Recieved badly formatted request line from connection %d\n",confd);
        return send_error(result, confd);
    }

    printf("request good\n");

    headers = strsep_str(&current, "\r\n\r\n");
    printf("headers:%s\n",headers);
     
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
    char read_buf[BUFLEN+1],
         *end, *temp;
    
    /*  If buf hasn't been allocated any mem it needs
     *  to be initialized with size of 1 and EOS placed in it
     *  so it can be reallocated and concatonated to*/
    if (!*request_buf && !(*request_buf = (char *)malloc(sizeof (char)))) {
        return SERVER_ERROR;
    }

    *(request_buf[0]) = '\0'; /* 'overwrite' previous string */
    
    for(;;) {
        if ((count = read(confd, read_buf, BUFLEN)) <= 0) {
            return CONNECTION_ERROR;
        }

        read_buf[count] = '\0';

        /*  If end of request is before end of buffer
         *  then we can just ignore the rest of the read buffer*/
        if ((end = strstr(read_buf, "\r\n\r\n"))) {
            count = end - read_buf + 4;
        }

        size += count;
          
        /*Reallocate Size + 1 (space for EOS char ) to buf and  
         * concatonate new read into it*/
        if (!(temp = (char *)realloc(*request_buf, sizeof(char) * (size + 1)))) {
             return SERVER_ERROR;
        }

        *request_buf = temp;

        strncat(*request_buf, read_buf, count);
        strncat(*request_buf, "\0", 1);
        printf("buf:%s",*request_buf);
        if (strstr(*request_buf, "\r\n\r\n")) { /*  We have all we need */
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
        printf("Connection %d\n acquired by thread %lu\n", confd, pthread_self());
        /*  Serve client until they close connection, or
         *  there is an error when attempting to read/write to/from them */
        do {
            /*  Obtain the request message as a string ending in '\r\n\r\n */
            if ((res = obtain_request(&message, confd)) != OK) {
                printf("Failed to obtain HTTP request from connection %d\n",confd);
                if (res != CONNECTION_ERROR) {
                   res = send_error(res, confd);
                }
            } else { /* If request message recieved successfully */
                res = respond_to(message, confd); 
            }
            
           
        } while (res != CONNECTION_ERROR);
        printf("Closing connection %d for thread %lu\n",confd, pthread_self());
        close(confd);       
    }
    /*  Should never get here, but if it does
     *  might as well attempt to avoid any memory leaks */
    free(message);
    return NULL;
        
}

/*  Create and initialize bounded buffer queue for client
 *  connections */
queue init_client_queue() {
    queue client_queue = {.head = 0,
    .tail = MAX_QUEUE_SIZE-1, .size = 0};

    pthread_cond_init(&(client_queue.condp), NULL);
    pthread_cond_init(&(client_queue.condc), NULL);
    pthread_mutex_init(&(client_queue.lock), NULL);

    return client_queue;
}

int main(int argc, char *argv[])
{
    int fd, t_count, confd;
    struct sockaddr_in6 addr, client_addr;
    queue client_queue;   
    pthread_t threads[MAX_THREADS];
    
    int port;
    char *endPtr;

    if (argc >= 2) {
         port = strtol(argv[1], &endPtr, 10);
         if (argv == NULL || *endPtr != '\0') {
            fprintf(stderr, "Port must be a number\n");
            return 1;
         }
    } else {
        fprintf(stderr, "Usage: %s [Port]\n",argv[0]);
        return 1;
    }
    
    fd = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
    if (fd == -1) {
        perror("unable to create socket. ");
        return 1;
    }

    memset(&addr,0,sizeof addr);  

    addr.sin6_addr =  in6addr_any;
    addr.sin6_family= AF_INET6;
    addr.sin6_port= htons(port);

    if (bind(fd, (struct sockaddr *) &addr,sizeof(addr)) == -1) {
        perror("unable to bind socket. ");
	    return 1;
    }

    client_queue = init_client_queue();

    /*  Initialize the threads for the threadpool */
    for(t_count = 0; t_count < MAX_THREADS; t_count++) {
        if (pthread_create(threads+t_count, NULL, client_thread, &client_queue) != 0) {
            fprintf(stderr, "failed to create thread %d\n",t_count+1);
            return 1;
        }
    }  

    /* Prepare to listen for incoming connections */  
    if (listen(fd, LISTEN_MAX) == -1) {
        perror("error when listening for connection. ");
        return 1;
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
