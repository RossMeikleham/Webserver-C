/*
 * =====================================================================================
 *
 *       Filename:  testSpace.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  25/01/14 18:53:38
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */

#include "spaces.h"

int main() 
{
    printf("%d\n",getWordCount("tes tes tes")); //3
    printf("%d\n",getWordCount(" t t t t ")); //4
    printf("%d\n",getWordCount("t   t  t    t ")); //4
    printf("%d\n",getWordCount(" t   t    t t t")); //5

    char **words;
    long size,i;
    words = spaces(" tes  fuydify fuydasifuydsaiu dffd", &size);
    printf("%d\n", size);
    for (i = 0; i < size; i++) {
        printf("words %s\n",words[i]);
    }

    deleteWords(words, size);

    return 0;

}




