#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

char RESPONSE_200[] = "200 OK";

#define REQ_MAX 8096
#define URL_MAX_LENGTH 2048

char ENTIRE_404_RESPONSE[] = "HTTP/1.1 404 Not Found\r\n" 
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

char ENTIRE_400_RESPONSE[] = 
    "HTTP/1.1 400 Bad Request\r\n"
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


char ENTIRE_500_RESPONSE[] =
    "HTTP/1.1 500 Internal Server Error\r\n"
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




char CONTENT_TEXT_HTML[] = "Content-Type: text/html";
char CONTENT_TEXT_PLAIN[] = "Content-Type: text/plain";
char CONTENT_IMAGE_JPEG[] = "Content-Type: image/jpeg";
char CONTENT_IMAGE_GIF[] = "Content-Type: image/gif";
char CONTENT_UNKNOWN[] = "Content-Type: application/octet-stream";

char CONNECTION_CLOSE[] = "Connection: close";

char END_OF_LINE[] = "\r\n";
char END_OF_INPUT[] = "\r\n\r\n";

/*  Stores details for request */
typedef struct {

    char *type;
    char *resource;
    char *http_version;


} Request;


typedef struct Header {
    struct Header next;
    char *name;
    char *value;
} header;


/*  Return pointer to Content Type response */
const char *getContentType(const char *resource)
{
    char* extention;
    int result;
    extention = strrchr(resource, '.');
    printf("extention: %s\n\n", extention);

    if(extention == NULL)
        return CONTENT_UNKNOWN;
    
    extention++;
    if(strcmp(extention,"html") == 0 || strcmp(extention,"htm") == 0)
        return CONTENT_TEXT_HTML;
        
    if(strcmp(extention,"txt") == 0)
        return CONTENT_TEXT_PLAIN;
        
    if(strcmp(extention,"jpg") == 0 || strcmp(extention,"jpeg") == 0)
        return CONTENT_IMAGE_JPEG;
    
    if(strcmp(extention,"gif") == 0)
        return CONTENT_IMAGE_GIF;

    else
        return CONTENT_UNKNOWN;
}



char *getResponse(Request *request, long *size) {
    char *file_name;
    struct stat fs;
    int fd;
    int result;
    FILE *file;
    char *response;
    const char *content_type_header;
    char *content_length_header = "Content-Length: ";
    char *head;
    char *length;
    char  length_buffer[11];
    long req_size;

    file_name = request->resource;
    
    fd = open(file_name, O_RDONLY);
    if( fstat(fd, &fs) == -1) {
        printf("NOT FOUND %s\n",file_name);
        *size = strlen(ENTIRE_404_RESPONSE);
        return ENTIRE_404_RESPONSE;
    }
    fd = fs.st_size;
    printf("FILESIZE %d\n",fd);
    
    if(!(file = fopen(file_name, "rb"))) {
        fclose(file);
        printf("NOT FOUND %s\n",file_name);
        *size = strlen(ENTIRE_404_RESPONSE);
        return ENTIRE_404_RESPONSE; 
    }

    content_type_header = getContentType(request->resource);
    printf("Content Header %s\n",content_type_header);

    snprintf(length_buffer, sizeof length_buffer, "%d", fd);

    *size = strlen(request->http_version) + 1 +
    strlen(RESPONSE_200) + 2 + strlen(content_type_header) +2 + strlen(content_length_header) +
    strlen(length_buffer) + 4 + fd;

    head = malloc(sizeof(char) * (*size));

    if(head == NULL) {
        fclose(file);
        *size = strlen(ENTIRE_500_RESPONSE);
        return ENTIRE_500_RESPONSE;
    }

    head[0] = '\0';    
    strcat(head, request->http_version);
    strcat(head," ");
    strcat(head, RESPONSE_200);
    strcat(head, END_OF_LINE);
    strcat(head, content_type_header);
    strcat(head, END_OF_LINE);
    strcat(head, content_length_header);
    strcat(head, length_buffer);
    strcat(head, END_OF_INPUT);
    if((result = fread(head+strlen(head),sizeof(char),fd,file)) < 1) {
        printf("Error reading file\n");
        fclose(file);
        *size = strlen(ENTIRE_500_RESPONSE);
        return ENTIRE_500_RESPONSE;
    }
    printf("size read %ld\n",result);
    printf("result:\n%s",head); 
    printf("result sizeee %d  %ld\n ", strlen(head), *size);
    fclose(file);
    return head;


}

 
Request *create_request() {
    Request *request;
    if((request = malloc(sizeof(Request))) == 0)
        return NULL;
    request->type = NULL;
    request->resource = NULL;
    request->http_version = NULL;
}

void delete_request(Request *request)
{
    if(request != NULL) {
        if(request->type != NULL) free(request->type);
        if(request->resource != NULL) free(request->resource);
        if(request->http_version != NULL) free(request->http_version);
        free(request);
    }
}


void destroy_headers(header *h)
{
    header *prev;
    while (h != NULL) {

        prev = h;
        h = h->next;
        free(prev->name);
        free(prev->value);
        free(prev);
    }

}

int create_headers(char *headers_start, header *h) {
    
    char *header_name, *head_value, *start, *end;
    header *temp_h;

    start = headers_start;
    end = strchr(start,":");

    if(end == NULL) {
        return 1
    }

    header_name = malloc(sizeof(char) * (end-start-1));
    strncpy(header_name, end-start-1, start);

    
}

/*  Parses a given header and stores details in given request
 *  returns 1 if successful, 0 if malformed, -1 if error*/
int parse_request_line(const char *header_start, char *resource)
{    
    char req_type[5], http_version[8],
         line[MAX_REQUEST+1], dummy[1];

    char *header_end;
    int  argc;
    long line_length;
    
    /*  Get header end */
    if( header_end = strstr(header_start, "\r\n") == NULL)
        return 400;
    
    if( (line_length = header_end - header_start + 1) > MAX_REQUEST)
        return 400;

    strncpy(line, MAX_REQUEST, line_length);

    /*  Parse for header values, ensure exactly 3*/
    argc = sscanf(line, "%5s %URL_MAX_LENGTHs %8s %1s", 
        req_type, resource, http_version, dummy);
    
    /* Validate values  */
    if (argc != 3 ||
        || strcmp(req_type, "GET") != 0 
        || strcmp(http_version, "HTTP/1.1") != 0) 
         {

        return 400;
    }

    return 0;    
          
}


int parse_fields() {

}


/* Performs HTTP request from given buffer */
char * request(char* buf, long *size) 
{
    char* header_end;
    int res;
    Request *request;
    char resource[MAX_URL_LENGTH+1], host[2048];

    /*If server can't create a request structure, then there was an error */
    if(strstr(buf, "\r\n\r\n") == NULL)
        return 400; 

    if( (res = parse_request_line(buf, resource)) == 400)
        return 400;

    

    /* Check request ends correctly and has a response line */
    if ((header_end = strstr(buf, "\r\n")) == NULL 
        || strstr(buf,END_OF_INPUT) == NULL) {
            *size = strlen(ENTIRE_400

    res = parse_header(request, buf, header_end - buf);
    
    switch (res) {
        case -1 : *size = strlen(ENTIRE_500_RESPONSE); return ENTIRE_500_RESPONSE; /*Something we did wrong */
        case  0 : *size = strlen(ENTIRE_400_RESPONSE);return ENTIRE_400_RESPONSE; /*Something client did wrong */
        default :  return getResponse(request, size); /* Header seems ok */
    }   
    
}

