#ifndef SPACES_H
#define SPACES_H

#include <string.h>
#include <stdio.h>
#include <stdlib.h>


typedef struct Words {
    long size;
    word *head;
}


typedef struct Word{
    word *next;
    word *previous;
    char *element;
}

typedef struct Itterator {
    word *head;
    word *current;
}


char * getNext(Itterator it) {
    if(it.current != NULL && it.current.next != NULL)
        return it.current.next.element;
}

//Returns number of words in a given
//string i.e. count of sets of characters separated
//by a space
static int getWordCount(char *str) 
{
    int count = 0;
    while(*str != '\0') {
        if(*str != ' ') {
            count++;
            while(*++str != ' ' && *str!= '\0');
        }
        else *str++;
    }
    return count;
}

/* Given a String and address of a long variable
 * Returns a pointer to a char * each containing
 * the string delimited by spaces, number of words
 * is placed in the size variable 
 * returns null if unsuccessful*/
char ** spaces(char *str, long *size) 
{
    int j, i = -1;
    int wordLength;
    char **words;     
    char *startWord;

    *size = getWordCount(str);
    words = (char **)malloc(sizeof (char *) * *size);

    while(*str != '\0') {
        if(*(startWord = str++) != ' ') {
            i++;
            for(wordLength = 1; (!(*str == ' ' || *str == '\0')); wordLength++, *str++) 
                ;;
            /* Allocate memory for String */
            if( (words[i] = (char *)malloc((wordLength+1) * sizeof(char))) == NULL) {
                /* Free all previously allocated Strings */
                    deleteWords(words, i);
                    return NULL; 
                }
                else { 
                    /* Copy string and add end of string indicator at end */
                    strncpy(words[i], startWord, wordLength);
                    words[i][wordLength] = '\0';
                }
            }
   }
   return words;
  
}

char ** split(char *

void deleteWords(char **words, long size) 
{
   while(size--) {
    free(words[size]);
   }

   free(words);
}


#endif
