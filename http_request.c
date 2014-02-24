/*
 * =====================================================================================
 *
 *       Filename:  http_request.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  24/02/14 12:47:55
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */

#include "request.h"

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


#include "circular_int_queue.h"

#define BUFLEN 1500
#define MAX_THREADS 10 /*Max number of clients which can be
                         concurrently served */
#define REQUEST_MAX 8096
#define URL_MAX_LENGTH 2048
#define LINE_MAX 2048


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
        return contents[0].content_type; /* No extention default is text/html */

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

      printf("strlen:%d\n",strlen(content));
      char headers[] ="HTTP/1.1 404 Not Found\r\n" 
          "Content-Type: text/html \r\n" 
          "Content-Length: 219\r\n\r\n";
      
    printf("not found stuff:%s\n", headers);
    write(confd, headers, strlen(headers));
    res = write(confd, content, strlen(content));
    return res; 

}


int send_bad_request_response(int confd)
{
    int res;
           
    char content[] =  
          "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\"\r\n"
          "\t\"http://www.w3.org/TR/html4/strict.dtd\">"
          "<html>\r\n" 
          "<head>\r\n" 
          "<title> 404 Not Found </title>\r\n" 
          "</head>\r\n" 
          "<body>\r\n" 
          "<p> Bad Request. </p>\r\n" 
          "</body>\r\n" 
          "</html>";

      printf("strlen:%d\n",strlen(content));
      char headers[] ="HTTP/1.1 400 Bad Request\r\n" 
          "Content-Type: text/html \r\n" 
          "Content-Length: 196\r\n\r\n";

     
    printf("not found stuff:%s\n", headers);
    write(confd, headers, strlen(headers));
    res = write(confd, content, strlen(content));
 
    return res; 

}

int send_server_error_response(int confd)
{
     int res;
      printf("wtf\n");
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

      printf("strlen:%d\n",strlen(content));
      char headers[] ="HTTP/1.1 500 Internal Server Error\r\n" 
          "Content-Type: text/html \r\n" 
          "Content-Length: 231\r\n\r\n";

     
    printf("not found stuff:%s\n", headers);
    write(confd, headers, strlen(headers));
    res = write(confd, content, strlen(content));
     
     return res;

}


/*  Attempt to send an open file through the given connection 
 *  returns  > 0 if successful, 0 or less if unsuccessful*/
int send_file(int confd, FILE *file)
{
    int res, wcount;
    char buf[LINE_MAX];

    res = write(confd, "", 0); /*  test connection */
    while((wcount = fread(buf ,sizeof(char), LINE_MAX ,file)) > 0) {
       if((res = write(confd, buf, wcount)) < 1) {
           break;
       }
   }

   return res;

}

int send_error(status s, int confd) 
{
    switch (s) {
        case BAD_REQUEST: return send_server_error_response(confd);
        case SERVER_ERROR: return send_server_error_response(confd);
        case NOT_FOUND: return send_not_found_response(confd);
        default: return send_server_error_response(confd);
    }
}

int get_response(char *resource, int confd) {

    char *file_name,
          http_200_ok[] = "HTTP/1.1 200 OK \r\n",
          content_length[] = "Content-Length: ",
          content_type[] = "Content-Type: ",
          length_buffer[11];

    struct stat fs;
    int fd, result, file_size;
    FILE *file;
    const char *content_type_header;
 
    file_name = resource;
    
    fd = open(file_name, O_RDONLY);
    if( fstat(fd, &fs) == -1) {
        close(fd);
        return send_not_found_response(confd);
    }
    
    file_size = fs.st_size;
    close(fd);
    
    if(!(file = fopen(file_name, "rb"))) {
        fclose(file);
        return send_not_found_response(confd); 
    }

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
    result = send_file(confd, file);
    fclose(file);
    return result;
    
}

/*  Modify string by an entire string delimiter instead of multiple
 *  character delimiters */   
char *get_token_str(char *buf, const char *delimiter, char **next) {
    char *end_token, *end_delimiters, *p;
    int len;

    len = strlen(delimiter);

    if(!(end_token = strstr(buf, delimiter))) {
        *next = NULL;
        return buf;
    }

    end_delimiters = end_token;
    while((!strncmp(end_delimiters, delimiter, len))) {
        end_delimiters += len;
    }
    
    while(end_token < end_delimiters) {
        *end_token++ = '\0';
    }

    *next = end_delimiters;
    return buf;
}

   
    


/*  Parses a given header and stores details in given request
 *  returns status */
status parse_request_line(char *header_start, char *resource)
{    
    char *next, *temp_resource;
    
    resource[0] = '.';
    next = NULL;
    
    /* Parse for header values, (Method, Resource, HTTP version) */
    if(strcmp(strtok_r(header_start," ", &next), "GET") != 0)
        return BAD_REQUEST;

    /*  Parse resource */
    temp_resource = strtok_r(NULL, " ", &next); 
    strncpy(resource+1, temp_resource, strlen(temp_resource) +1);
   
    if(strcmp(next, "HTTP/1.1") != 0)
        return BAD_REQUEST;
    
    return OK;    
}





/*  Reurn hostname or NULL if none or
 *  failure with fields */
static int get_host(char *buf, char *header_store, unsigned long max_header_size) {

    char *next, *line;

    next = buf;
    printf("entire header:%s\n",buf);
    while(next){

     line = get_token_str(next, "\r\n", &next);
     next = strtok_r(line, ": ", &next); 
     if(!(strcasecmp(next,"host"))) {
        next = strtok_r(NULL, "\t ", &next); 
        if(max_header_size < strlen(next)+1)
            return 400;
        else {
            strncpy(header_store, next, strlen(next) + 1);
            return OK;
       }
     }

    }

    return OK;
    
}





/* Performs HTTP request from given buffer */
int request(char* buf, int confd) 
{
    char host[1024], this_host[1024];
    status res;
    char *next = NULL;
    char resource[URL_MAX_LENGTH+1];
    char *resource_line;
    char *headers;

    /*Check request ends correctly */
    if(strstr(buf, "\r\n\r\n") == NULL) {
        return send_bad_request_response(confd);
    }

    resource_line = get_token_str(buf, "\r\n", &next); /*Cut the part of the request that's necessary*/

    printf("requests fine\n");
    printf("next:%s\n",next);
    /*  Get the resource, if error with parsing request line, send 400 response */
    if((res = parse_request_line(resource_line, resource)) != OK) {
       return send_error(res, confd);
    }

    printf("before split:%s\n",next);
    headers = get_token_str(next, "\r\n\r\n", &next);
    printf("attempting to retrive host-name:%s\n",headers);

     /* Check headers and hostname, if error send 400 response */
     if ((res = get_host(headers, host, 1024)) != OK) {
            return send_error(res, confd);
     }
    
    
    gethostname(this_host, 1024);
    printf("my hostname %s\n",this_host);
    //if(strcmp(host, this_host) !=0) {
    //    return send_bad_request_response(confd);}


       printf("res host %d %s donee\n",res, host);
       
    
    printf("Getting a response\n");
    return get_response(resource, confd);

}



