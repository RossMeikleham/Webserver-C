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

#define BUFLEN 1500
#define MAX_THREADS 10 /*Max number of clients which can be
                         concurrently served */
#define REQUEST_MAX 8096
#define URL_MAX_LENGTH 2048
#define LINE_MAX 2048


/*  Create an empty queue */
client_queue cq = {.head = 0, .tail = MAX_QUEUE_SIZE-1, .size = 0};

char RESPONSE_200[] = "200 OK";


typedef enum {NOT_FOUND = 404, BAD_REQUEST = 400, OK = 200, SERVER_ERROR = 500} status;

typedef struct {
    char *extention; 
    char *content_type;
} content_map;


content_map contents[] = { {"htm","text/html"}, {"html","text/html"},
    {"txt","text/plain"}, {"jpg","image/jpeg"}, {"jpeg","image/jpeg"},
    {"gif","image/gif"}, {NULL, "application/octet-stream"}};


const char *get_content_type(const char *resource)
{  
    char* extention; 
    content_map *type;

    if ((extention = strrchr(resource, '.')) == NULL)
        return contents[0].content_type; /* No extention default it text/html */

    extention++; /* move so start is past the . */
    type = contents;

    for(type = contents ;type->extention != NULL; type++) {
        if (strcmp(extention, type->extention) == 0)
            return type->content_type;
    }

    /*  is unknown we return application/octet stream */
    return type->content_type;   
}

int send_not_found_response(int confd)
{
    int res;
    char buf[] ="HTTP/1.1 404 Not Found\r\n" 
          "Content-Type: text/html \r\n" 
          "Connection: close \r\n\r\n"
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
    
    printf("bufsize %d\n", strlen(buf));      
    res = write(confd, buf, strlen(buf));
    close(confd);
    return res; 

}

int send_bad_request_response(int confd)
{
    int res;
    char buf[] = "HTTP/1.1 400 Bad Request\r\n"
        "Content-Type: text/html \r\n"
        "Connection: close \r\n\r\n"
        "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\"\r\n"
        "\t\"http://www.w3.org/TR/html4/strict.dtd\">"
        "<html>\r\n" 
        "<head>\r\n" 
        "<title> 400 Bad Request </title>\r\n" 
        "</head>\r\n" 
        "<body>\r\n" 
        "<p> Your request was bad. </p>\r\n" 
        "</body>\r\n" 
        "</html>";
     
    printf("bad rq len buf %d\n",strlen(buf)); 
    res = write(confd, buf, strlen(buf));
    close(confd);
    return res; 

}

int send_server_error_response(int confd)
{
     int res;
     char buf[] = "HTTP/1.1 500 Internal Server Error\r\n"
        "Content-Type: text/html \r\n"
        "Connection: close \r\n\r\n"
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
     
    res = write(confd, buf, strlen(buf)); 
    close(confd);
    return res;

}


int send_file(int confd, FILE *file)
{
    int buflen = 1024, res, rcount;
    char buf[buflen];

    res = write(confd, "", 0);
    while((rcount =fread(buf ,sizeof(char), buflen ,file)) > 0) {
       if( (res = write(confd, buf, rcount)) < 1) {
           printf("fail\n");
           return res;
       }
        printf("buf size %ld\n",sizeof buf);
       printf("sent 1024 bytes\n");
   }

    return res;

}

int get_response(char *resource, int confd) {
    char *file_name;
    char http_200_ok[] = "HTTP/1.1 200 OK \r\n";
    char *test;
    struct stat fs;
    int fd;
    int result;
    FILE *file;
    char *response;
    const char *content_type_header;
    char  length_buffer[11];
    long req_size;

    test = malloc(sizeof(char) * 20);
    file_name = resource;
    
    fd = open(file_name, O_RDONLY);
    if( fstat(fd, &fs) == -1) {
        printf("NOT FOUND f %s\n",file_name);
        return send_not_found_response(confd);
    }

    fd = fs.st_size;
    printf("FILESIZE %d\n",fd);
    
    if(!(file = fopen(file_name, "rb"))) {
        fclose(file);
        printf("NOT FOUND %s\n",file_name);
        return send_not_found_response(confd); 
    }

    content_type_header = get_content_type(resource);
    printf("Content Header %s\n",content_type_header);

    /*  Convert content-length to string */
    snprintf(length_buffer, sizeof length_buffer, "%d", fd);
    
    /*  Send headers */
    result = write(confd, http_200_ok, strlen(http_200_ok));
    printf("resultt %d\n",result);
    write(confd, "Content-Length: ", 16);
    write(confd, length_buffer, sizeof(length_buffer));
    printf("len buf %s\n",length_buffer);
    write(confd, "\r\n\r\n", 4);
    
    /*  Send the file over the socket */
    result = send_file(confd, file);
    fclose(file);
    return result;
    
}

   
   
    


/*  Parses a given header and stores details in given request
 *  returns 1 if successful, 0 if malformed */
int parse_request_line(const char *header_start, char *resource)
{    
    char req_type[5], http_version[8],
         line[REQUEST_MAX+1], dummy[1];

    char *header_end;
    int  argc;
    long line_length;

    resource[0] = '.';
    
    /*  Get header end */
    if( (header_end = strstr(header_start, "\r\n")) == NULL) {
        printf("no request end\n");
        return 0;
    }
    
    /* Check if request line is not too long */
    if( (line_length = header_end - header_start + 1) > REQUEST_MAX) {
        printf("wtf too big yo (that's what she said)\n");
        return 0;
    }

    strncpy(line, header_start, line_length);
    printf("line: %s\n",line);

    /*  Parse for header values, ensure exactly 3*/
    argc = sscanf(line, "%5s %2048s %8s %1s", 
        req_type, resource+1, http_version, dummy);
    
    printf("req type %s\n",req_type);
    printf("resource %s\n",resource);
    printf("http %s\n",http_version);

    /* Validate values */
    if (argc != 3 || strcmp(req_type, "GET") != 0 
      || strcmp(http_version, "HTTP/1.1") != 0) {
        printf("something wrong with req params\n");
        return 0;
    }


    return 1;    
          
}


/*  Takes a header where the header ends with \r\n\r\n */
int get_header_value(char *start_field, char *header_value, long max_size){

    char *current, *eol;
    int count = 0, header_size = 0, add;
  
    if(max_size <= 0) 
        return SERVER_ERROR;

    header_value[0] = '\0';
    current = start_field;
   
    for(;;) {

        sscanf(current," %n%*s", &count); /* Skip all whitespace */
        current = current + count;
        if(count > 1 && current[-1] == '\n' && current[-2] == '\r') {
            return OK;
        }
        
        
        if ((eol = strstr(current, "\r\n")) == NULL)
            return BAD_REQUEST;
        

        add = eol - current + 1;
        if(header_size + add > max_size)
            return BAD_REQUEST;

        strncat(header_value, current, add);
        if(eol[1] != ' ' && eol[1] != '\t')
            return OK;

    }


}



/*  Reurn hostname or NULL if none or
 *  failure with fields */
int check_headers_get_host(char *buf, char *host_name, long max_host_size) {

    char *start_name, *end_name,
         *start_field, *end_field, 
         *start_line, *end_line,
        *end;

    long length, skip = 0;
    int isHost = 0, host_length = 0, value;

    length = strlen(buf);

    start_name = start_line = buf;

    if ((end = strstr(buf,"\r\n\r\n")) == NULL)
        return BAD_REQUEST;

    for(;;) {

    if ((end_name = strchr(start_line, ':')) == NULL) {
        return BAD_REQUEST;
    }

    /*  check if we have a host header */
    printf("header name %.5s\n",start_line);
    if(end_name - start_line >= 3 && strncasecmp(start_line,"Host:",5) == 0){
        printf("scanning\n");
        if ((value = get_header_value(end_name+2, host_name, max_host_size)) != OK)
            return value;
    }
    else printf("nope\n");
    
    /*  Skip to the end of the field name */
    start_field = end_name+1;
    end_field = start_field;
  
    /*  Read through the until we get to the end of field value, i.e. a line not starting with a tab
     *  or a space */
    while ((end_field = strstr(end_field, "\r\n")) != NULL
        && (end_field[1] == ' ' || end_field[1] == '\t'))
        ;;

    if(end == end_field)
        return OK;
    
    start_line = end_field + 1; /* Go to next line */
        
   }
    
}





/* Performs HTTP request from given buffer */
int request(char* buf, int confd) 
{
    char host[1024], this_host[1024];
    int res;
    char resource[URL_MAX_LENGTH+1];

    /*Check request ends correctly */
    if(strstr(buf, "\r\n\r\n") == NULL) {
        send_bad_request_response(confd);
        return -1;
    }

    /*  Get the resource, if error with parsing request line, send 400 response */
    if(!(res = parse_request_line(buf, resource))) {
       send_bad_request_response(confd);
       return -1;
    }

      /* Check headers and hostname, if error send 400 response */
    if ((res = check_headers_get_host(strstr(buf,"\r\n")+2, host, 1024)) != OK) {
            if (res == 400) return send_bad_request_response(confd);
            else {send_server_error_response(confd);
                  return -1;
            }
    }

    gethostname(this_host, 1024);
    printf("my hostname %s\n",this_host);
    //if(strcmp(host, this_host) !=0)
    //    return send_bad_request_response(confd);


       printf("res host %d %s donee\n",res, host);
       
    
    printf("Getting a response\n");
    return get_response(resource, confd);

}






void* client_thread(void * stuff) {

    int confd;
    int connection_closed;
    int rcount;
    char type[2049]; 
    long size;

    for(;;) {
        confd = get_client();
    

        for(;;) {

            printf("confd recieved %d\n", confd);  
            rcount = read(confd, type, 2048);
            printf("done recieving");
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

                type[2048] = '\0';
                printf("Client message %s\n",type);
                
	
                rcount = request(type, confd);
                if(rcount <= 0) return;
              }

        }
     
     return NULL;
}


int main()
{
    int fd;
    struct sockaddr_in6 addr;
    int t_count;
    pthread_t threads[MAX_THREADS];
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

    for(t_count = 0; t_count < MAX_THREADS; t_count++) {
       pthread_create (threads+t_count, NULL, client_thread , NULL); 

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
        add_client(confd);
   
    }

    printf("wtf\n");
    close(fd);
    return 0;
}






