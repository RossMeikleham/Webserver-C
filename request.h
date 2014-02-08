#ifndef REQUEST_H
#define REQUEST_H

#include <string.h>
#include <stdlib.h>
#include <stdio.h>


typedef enum {GET, POST, PUT, DELETE, INVALID} RequestType;
typedef enum {NOT_FOUND = 404, FORBIDDEN = 403, SERVER_ERROR = 500, OK =200} ResponseCode;
typedef enum {HEADER} HeaderType;


char notFound[]= " <html>\r\n
                    <head>\r\n
                    <title> 404 Not Found </title>\r\n
                    </head>\r\n
                    <body>\r\n
                    <p> The requested file cannot be found. </p>\r\n
                    </body>\r\n
                    </html>";

char serverError[] =  " <html>\r\n
                    <head>\r\n
                    <title> 500 Server error </title>\r\n
                    </head>\r\n
                    <body>\r\n
                    <p> The Server experienced an error. </p>\r\n
                    </body>\r\n
                    </html>";

char requestError[] = " <html>\r\n
                    <head>\r\n
                    <title> 400 HTML error </title>\r\n
                    </head>\r\n
                    <body>\r\n
                    <p> The Server couldn't process your request. </p>\r\n
                    </body>\r\n
                    </html>";


char *response() {
    return sprintf("HTTP/1.1 404 Not Found\r\n Content-Type: text/html\r\nConnection: close\r\n\r\n
                    <!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN"\r\n
                    \t\t\"http://www.w3.org/TR/html4/strict.dtd\">\r\n
                    <html>\r\n
                    <head>\r\n
                    <title> 404 Not Found </title>\r\n
                    </head>\r\n
                    <body>\r\n
                    <p> The requested file cannot be found. </p>\r\n
                    </body>\r\n
                    </html>");

}

typedef struct header {
    struct header next;
    char* headerName;
    char *value;
}

typedef struct {
    RequestType requestType;
    char *resource;
    char *http;

} Request;


typedef struct response {
    ResponseCode responseCode;
    char *content;
} Response;

typedef struct Str_Node{
    struct Str_Node *next;
    char* element;
} str_node;


char *getResource(Request *request) {
    FILE *resource;
    fopen(request->resource);
}
 


unsigned long getLength(str_node *t)
{
    unsigned long count = 0;
    for(;t != NULL; count++ ; t=t->next);
    ;;
    return count;

}

RequestType getRequestType(const char *word)
{

    if (!strcmp(word, "GET"))
        return GET;
    else return INVALID;
}


int isHttpProtocol(const char *word)
{

}

char **getType(char** words, int size) 
{
    
}

int setResource(Request *request, char *word)
{
    request->resource = word;
}


int checkRequestLine(Request *request, char *line)
{
    str_node *token;
    token = splitWhiteSpace(line);

    if(getLength(token) < 3 ) {  
        deleteTokens(token);
        return 0;
    }
    request->requestType = getRequestType(token->element);
    line = line->next;
    request->resource = line->element;
    line = line->next;
    if(!strcmp(line, "/HTTP/1.1"))
        return 0;
        
            
    return 1; 

} 

int addHeader(Request *request, char *line)
{
    return 1;
}


char * generateResponse(Request *request, Response *response)
{
    switch (response->responseCode) {
        case NOT_FOUND : return notFound;
        case OK : break;
    }
    return NULL;
}

Request *getRequest(char *buf)
{
    Request request;
    Response response;
    p = splitRequestLines(buf);
    n = p;
    if(n != NULL) {
        /*  Get request line */
        if(!checkRequestLine(&request, n->element)
            response.responseCode = NOT_FOUND;
    }
    
    /*  Get Header fields */
    while(n != NULL) {
        addHeader(&request, n->element);
    }

    
}


int isEndRequest(char *str) {
    return strlen(str) > 1 && !strncmp(str, "\r\n\r\n", 4);
}

void deleteTokens(str_node *token) {
    while(token != NULL) {
        str_node *next = token->next;
        free(token->element);
        free(token);
        token = next;
    }
}

/*  Takes a char array and splits by whitespace */
str_node *splitWhiteSpace(char * str) {

    str_node *startToken = NULL;
    str_node *currentToken = NULL;
    str_node *previous = NULL;
    char *startWord = NULL;
    long wordLength;
    
    /* Loop until end of request \r\n\r\n found or no more lines found  */

    while(*str != '\0') {
        if(*(startWord = str++) != ' ' && *startWord != '\t') {
            currentToken = (str_node *)malloc(sizeof(str_node));
            for(wordLength = 1; (!(*str == ' ' || *str == '\t' || *str == '\0')); wordLength++, *str++) 
                ;;
            /* Allocate memory for String */
            if ((currentToken->element = (char *)malloc((wordLength+1) * sizeof(char))) == NULL) {
                /* Free all previously allocated Strings */
                    deleteTokens(startToken);
                    return NULL; 
             }

             if (previous != NULL) previous->next = currentToken; /*Link up list*/
                 
             /* Copy string and add end of string indicator at end */
             strncpy(currentToken->element, startWord, wordLength);
             currentToken->element[wordLength] = '\0';

             if(startToken == NULL) {
                 startToken = currentToken;
             }
                
             currentToken->next = NULL;
             previous = currentToken;
             currentToken = NULL;
         }
     }

   //previous->next = NULL;
   return startToken;
}


/*  Takes a request and splits it up into line tokens
 *  up to \r\n. If request doesn't contain or end with
 *  \r\n or malloc fails null is returned*/
str_node *splitRequestLines(char * buf) {
    
    str_node *startToken = NULL;
    str_node *currentToken = NULL;
    str_node *previous = NULL;
    char *start = NULL;
    char *cur = NULL;
    
    start = buf;
    cur = strstr(buf, "\r\n");
    /* Loop until end of request \r\n\r\n found or no more lines found  */
    for(; cur != NULL && !(isEndRequest(start)); start = cur+2, cur = strstr(cur+2, "\r\n")) {

            /*  Allocate a new token */
            if((currentToken = (str_node *)malloc(sizeof(str_node))) == NULL) {
                deleteTokens(startToken);
                return NULL;
            }
           
            /*  Allocate memory for line in token */
            if((currentToken->element = (char *)malloc(sizeof (char) * (cur - start + 1))) == NULL) {
                free(currentToken);
                deleteTokens(startToken);
                return NULL;
            }

            if(previous != NULL) previous->next = currentToken; /*  link up to list */

            /*  Copy current line into token */
            strncpy(currentToken->element, start, cur-start); /*  copy request line */
            currentToken->element[cur-start] = '\0'; /* Ensure String ends properly */

            if (startToken == NULL) {
                startToken = currentToken;
            }
            
            previous = currentToken;
            currentToken = NULL;
    }

    /*  Request didn't end with \r\n\r\n */
    if(cur == NULL) {
        deleteTokens(startToken);
        return NULL;
    } else {
        previous->next = NULL;
        return startToken;
        }
}



#endif /*REQUEST_H*/
