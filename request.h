#ifndef REQUEST_H
#define REQUEST_H

#define PORT 8080

/*  turn argument into string literal, 
 *  we need STRSTR so macro fed to STR 
 *  is expanded rather than taking literal value  
 *  i.e. STR(PORT) gives the value port represents as a String 
 *  rather than literally "PORT"*/
#define STRSTR(x) #x
#define STR(x) STRSTR(x)



/* Given a message string ending in '\r\n\r\n' 
 * Responds to the given socket with the appropriate
 * response*/
status respond_to(char *msg, int confd);

status obtain_request(char **buf, int confd);

status send_error(status s, int confd);

#endif /*REQUEST_H*/
