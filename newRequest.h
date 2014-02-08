#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

char RESPONSE_200[] = "200 OK";
char RESPONSE_404[] = "404 Not Found";
char RESPONSE_400[] = "400 input Error";
char RESPONSE_500[] = "500 Server Error";


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


char CONTENT_TEXT_HTML[] = "Content-Type: text/html";
char CONNECTION_CLOSE[] = "Connection: close";

char END_OF_LINE[] = "\r\n";
char END_OF_INPUT[] = "\r\n\r\n";

/*  Stores details for request */
typedef struct {

    char *type;
    char *resource;
    char *http_version;

} Request;

/*  Returns a 404 not found response */
char *not_found() {
    return ENTIRE_404_RESPONSE;
}
 

char *getResponse(Request *request) {
    char *file_name;
    struct stat fs;
    int fd;
    FILE *file;
    char *response;
    char contentHeader[] = "Content-Type: ";
    char connectionHeader[] = "Connection: close";
    char *head;

    file_name = request->resource;
    
    fd = open(file_name, O_RDONLY);
    if( fstat(fd, &fs) == -1) {
        printf("NOT FOUND %s\n",file_name);
        printf("test\n%s\n",not_found());
        return not_found();
    }
    fd = fs.st_size;
    
    if(!(file = fopen(file_name, "r"))) {
        fclose(file);
        printf("NOT FOUND %s\n",file_name);
        return not_found(); 
    }

   
    head = malloc(strlen(request->http_version) + 1 +
    strlen(RESPONSE_200) + 2 + strlen(CONTENT_TEXT_HTML) +
    2 + strlen(CONNECTION_CLOSE) + 4 + fd +1);


    strcat(head, request->http_version);
    strcat(head," ");
    strcat(head, RESPONSE_200);
    strcat(head, END_OF_LINE);
    strcat(head, CONTENT_TEXT_HTML);
    strcat(head, END_OF_LINE);
    strcat(head, CONNECTION_CLOSE);
    strcat(head, END_OF_INPUT);
    fread(head+strlen(head),sizeof(char),fd,file);
    
    printf("result:\n%s",head); 
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

/*  Parses a given header and stores details in given request
 *  returns 1 if successful, 0 if malformed, -1 if error*/
int parse_header(Request *request, const char *header_start, unsigned long size)
{
    char *buf, *req_type, *resource , *http_version;

    /*  Read entire header into buffer */
    if((buf = (char *)malloc(sizeof(char) * size + 1)) == NULL)
        return -1;
    
    strncpy(buf, header_start, size);
    buf[size] = '\0';
    printf("buffer for header %s\n",buf);

    /*  Parse buffer for header values, ensure there are no more than 3 */
    if((req_type = strtok(buf, " \t")) == NULL
      ||(resource = strtok(NULL, " \t")) == NULL
      ||(http_version = strtok(NULL, " \t")) == NULL
      ||strtok(NULL, "\t") != NULL) {

        free(buf);
        return 0;
    }
    printf("resource: %s\n",resource);
    
    /* Validate values  */
    if(strcmp(req_type, "GET") != 0 || strcmp(http_version, "HTTP/1.1") != 0) {
        free(buf);
        return 0;
    }
    
    /*  Allocate request memory and copy values into them */
    request->type = (char *)malloc(sizeof(char) * strlen(req_type) + 1);
    request->http_version = (char *)malloc(sizeof(char) *strlen(http_version) + 1);
    request->resource = (char *)malloc(sizeof(char) *strlen(resource) + 2);  
    
    if(request->type == NULL
        || request->http_version == NULL 
        || request->resource == NULL) {
        free(buf);
        return -1;
    }
    request->resource[0] = '\0';

    strcpy(request->type, req_type);
    strcat(request->resource, ".");
    printf("start res: %s\n",request->resource);
    strcat(request->resource, resource);
    strcpy(request->http_version, http_version);

    free(buf);
    return 1;
        
}


int parse_fields() {

}


/* Performs HTTP request from given buffer */
char * request(char* buf) 
{
    char* header_end;
    int res;
    Request *request;
    request = create_request(); 

    header_end = strstr(buf, "\r\n");
    if(header_end == NULL)
        ;//return not_found();

    res = parse_header(request, buf, header_end - buf);
    if (res == 1 ) {printf("type %s res %s http %s\n",
                    request->type, request->resource, request->http_version);
                    return getResponse(request);
                     }
    else printf("failed\n");
    delete_request(request);
    return NULL;
    
    
}

