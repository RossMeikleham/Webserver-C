/*
 * =====================================================================================
 *
 *       Filename:  testParseRequest.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  26/01/14 11:52:38
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ross Meikleham 
 *   Organization:  
 *
 * =====================================================================================
 */

#include "../request.h"
#include <stdio.h>


int main() {

    char* testStr = {"\r\n hi \r\n test1 \r\nthis \r\n is a test\r\n\r\n\r\n blarg"};

    ;
    getRequest(testStr);
    deleteTokens(p);
    return 0;
}










