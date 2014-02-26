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
 *         Author:  Ross Meikleham 1107023m
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
#include <sys/select.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <strings.h>


#define URL_MAX_LENGTH 2048
#define LINE_MAX 2048


char RESPONSE_200[] = "200 OK";


typedef struct {
    char *extention; 
    char *content_type;
} content_map;


content_map contents[] = { {"htm","text/html"}, {"html","text/html"},
    {"txt","text/plain"}, {"jpg","image/jpeg"}, {"jpeg","image/jpeg"},
    {"gif","image/gif"}, {NULL, "application/octet-stream"}};




/* Returns a pointer to the content type of the 
 * specified resource, using its extension.
 * Returns txt/html if no extension or
 * octet-stream type if unrecognized extension */
static const char *get_content_type(const char *resource)
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



/*  Send a "404 Not Found" response
 *  back to the client, returns the result
 *  if the last write */
static int send_not_found_response(int confd)
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
    return write(confd, content, strlen(content));

}


/*  Send a "400 Bad Request" response
 *  back to the client, returns the result
 *  of the last write */
static int send_bad_request_response(int confd)
{
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

      char headers[] ="HTTP/1.1 400 Bad Request\r\n" 
          "Content-Type: text/html \r\n" 
          "Content-Length: 196\r\n\r\n";

     
    write(confd, headers, strlen(headers));
    return write(confd, content, strlen(content)); 

}



/*  Send a "500 Server Error" response
 *  back to the client, returns the result
 *  of the last write */
static int send_server_error_response(int confd)
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

      printf("strlen:%d\n",strlen(content));
      char headers[] ="HTTP/1.1 500 Internal Server Error\r\n" 
          "Content-Type: text/html \r\n" 
          "Content-Length: 231\r\n\r\n";

     
    write(confd, headers, strlen(headers));
    return write(confd, content, strlen(content));
     
}




/*  Attempt to send an open file through the given connection 
 *  returns  > 0 if successful, 0 or less if unsuccessful*/
static int send_file(int confd, FILE *file)
{
    int res, wcount;
    char buf[LINE_MAX];

    res = write(confd, "", 0); /*  test connection */
    while((wcount = fread(buf ,sizeof(char), LINE_MAX ,file)) > 0) {
       if((res = write(confd, buf, wcount)) < 1) {
           break;
       }
   }

   return res > 0 ? OK : CONNECTION_ERROR;

}



/* Send an Error Response back to the 
 * specified client */
status send_error(status s, int confd) 
{
    switch (s) {
        case BAD_REQUEST: return send_bad_request_response(confd) > 0 ? OK : CONNECTION_ERROR;
        case SERVER_ERROR: return send_server_error_response(confd) > 0 ? OK : CONNECTION_ERROR;
        case NOT_FOUND: return send_not_found_response(confd) > 0 ? OK : CONNECTION_ERROR;
        /* Error-Ception, if it reaches here server had made
         * an error, so we send a server error response */
        default: return send_server_error_response(confd) > 0 ? OK : CONNECTION_ERROR;
    }
}


/* Given the resource and socket, attempts
 * to locate the resource file and send it
 * down the connection, sends appropriate error
 * response if this fails. Returns the result of
 * the last message sent to the client */
static status send_response(char *resource, int confd) {

    char *file_name,
          http_200_ok[] = "HTTP/1.1 200 OK \r\n",
          content_length[] = "Content-Length: ",
          content_type[] = "Content-Type: ",
          length_buffer[11];

    struct stat fs;
    int fd, file_size;
    status result;
    FILE *file;
    const char *content_type_header;
 
    file_name = resource;
    
    /*  Obtain file size to determine content length */
    fd = open(file_name, O_RDONLY);
    if( fstat(fd, &fs) == -1) {
        close(fd);
        return NOT_FOUND;
    }
    
    file_size = fs.st_size;
    close(fd);

    /* Should not be negative, else something has gone wrong */
    if(file_size < 0) 
        return SERVER_ERROR;

    /* We just want a pure byte stream so open file in binary write form*/
    if(!(file = fopen(file_name, "rb"))) {
        fclose(file);
        return NOT_FOUND; 
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
static char *get_token_str(char *buf, const char *delimiter, char **next) {
    char *end_token, *end_delimiters;
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

   
    


/*  Parses a request line and stores details in given request
 *  returns status of parsing the line, should be OK
 *  if request line format is correct */
static status parse_request_line(char *buf, char *resource)
{    
    char *next, *temp_resource;
    
    resource[0] = '.'; /*  Need to prepend '.'  */
    next = NULL;
    
    /* Parse for header values, (Method, Resource, HTTP version) */
    if(strcmp(strtok_r(buf, " ", &next), "GET") != 0)
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
static status get_host(char *buf, char *header_store, unsigned long max_header_size) {

    char *next, *line;

    next = buf;
    printf("entire header:%s\n",buf);
    while(next){

     line = get_token_str(next, "\r\n", &next);
     if(!strchr(line,':'))
         return BAD_REQUEST;
         
     next = strtok_r(line, ": ", &next); 
     if(!(strcasecmp(next,"host"))) {
        next = strtok_r(NULL, "\t ", &next); 
        if(max_header_size < strlen(next)+1)
            return BAD_REQUEST;
        else {
            strncpy(header_store, next, strlen(next) + 1);
            return OK;
       }
     }

    }

    return OK;
    
}



static status check_recieved_host(char *recieved_host) 
{
    char this_host[LINE_MAX];

    gethostname(this_host, LINE_MAX);
    if(gethostname(this_host, LINE_MAX) == -1 
      )//|| this_host[LINE_MAX-1] != '\0') /*  Check gethostname successful*/
        return SERVER_ERROR;

    /*  Compare direct hostname with recieved host */
    if(!strcmp(this_host, recieved_host))
        return OK;

    /*  If fails check our hostname + :PORT with recieved host */
    /*  Check enough space to add port */
    if(strlen(this_host) + strlen(STR(PORT)) + 1 > LINE_MAX)
        return SERVER_ERROR;
    

    strncat(this_host, ":", 1);
    strncat(this_host, STR(PORT), strlen(STR(PORT)));

    if(!strcmp(this_host, recieved_host))
        return OK;

    /*  If both fail check our  */

    printf("my hostname %s\n",this_host);
    //if(strcmp(host, this_host) !=0) {
    //    return send_bad_request_response(confd);}


       //printf("res host %d %s donee\n",res, host);
       return OK;

}



/*  result is OK if successfully obtained request, and
 *  request is stored in buf
 *  result is SERVER_ERROR if failure to allocate memory for request
 *  result is CONNECTION_ERROR if failure to read */
status obtain_request(char **buf, int confd) 
{
    unsigned long size = 0;
    int count;
    char read_buf[31];
    char *end, *temp;

    if(!(temp = (char *)realloc(*buf, sizeof(char)))) {
        return SERVER_ERROR;
    }
    *buf = temp;

    (*buf)[0] = '\0';
    
    for(;;) {
        if((count = read(confd, read_buf, 30)) <= 0)
            return CONNECTION_ERROR;

        read_buf[count] = '\0';
        printf("values read %s\n", read_buf);

        /*  If end of request is before end of buffer
         *  then we can just ignore the rest */
        if((end = strstr(read_buf, "\r\n\r\n"))) {
            count = end - read_buf + 4;
        }

        size += count;

        /*Reallocate Size + 1 (space for EOS char ) to buf and  
         * concatonate new read into it*/
        if(!(temp = (char *)realloc(*buf, sizeof(char) * (size + 1)))) {
             return SERVER_ERROR;
        }

        else { 
            *buf = temp;
            strncat(*buf, read_buf, count+1);
            printf("buffer contains %s\n", *buf);
            //printf("%s\n",strstr(buf, "\r\n\r\n"));
            if(strstr(*buf, "\r\n\r\n")) {
                printf("request ok\n");
                printf("buf:%s\n",*buf);
                return OK;
            }
        }
    }      


}

/* Given a HTTP message and the size of the message 
 * Responds to the given socket with the approrpriate
 * HTTP response. The space allocated for message should
 * be at least 1 more than req_size */
status respond_to(char* buf, int confd)
{
    char host[1024];
    status res;
    char *next = NULL;
    char resource[LINE_MAX+1];
    char *resource_line;
    char *headers;
    

    /*  Cut off the Resource line */
    printf("splitting lines\n");
    resource_line = get_token_str(buf, "\r\n", &next); 

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
     if ((res = get_host(headers, host, LINE_MAX)) != OK) {
            return send_error(res, confd);
     }
    
     if ((res = check_recieved_host(host)) != OK) {
         return send_error(res, confd);
     }
           
    
        printf("Getting a response\n");
    return send_response(resource, confd);

}



