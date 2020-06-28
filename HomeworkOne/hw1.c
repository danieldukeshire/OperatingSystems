/*********************************************************************************
 hw1.c
 Written by Daniel Dukeshire, 1.28.2020 - 2.1.2020

 This program uses pointer arithmetic to implement a rudimentary cache of words
 found in a given text file. Here, the cache will be represented using a hash
 table. The hash table handles collisions by replacing the exisiting word with
 the queried word.

 The user is required to input the wanted size of the cache as the first command-
 line argument, with a file to open and read as the second argument.
 *********************************************************************************/

#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>

/****************************************************************************
printCache
Takes a 2D char array representing the cache, and prints the contents within
****************************************************************************/
void printCache(char** cache, int size_of_cache)
{
  for(int i=0; i<size_of_cache; i+=1)
  {
    if ( *(cache+i) != NULL )
    {
      printf("Cache index %d ==> \"%s\"\n", i, *(cache+i));
    }
  }
}

/****************************************************************************
print_cache
Takes a 2D char array representing the cache, and prints the contents within
****************************************************************************/
int main(int argc, char** argv)
{
  setvbuf( stdout, NULL, _IONBF, 0);
    // The first argument in argv specifies the size of the cache (hash table)
    // The second argument in argv is the file to be opened and parsed.
    if(argc != 3)                       // Some error checking
    {
      fprintf(stderr, "ERROR: Invalid arguments\n");
      fprintf(stderr, "USAGE: a.out cache-size input-file\n");
      exit(EXIT_FAILURE);
    }

    int size_of_cache = atoi(*(argv+1));
    if(size_of_cache == 0)
    {
      fprintf(stderr, "ERROR: Invalid arguments\n");
      fprintf(stderr, "USAGE: a.out cache-size input-file\n");
      exit(EXIT_FAILURE);
    }

    char** cache = calloc(size_of_cache, sizeof(char*));
    FILE* input_file = fopen(*(argv+2), "r");

    if(input_file == NULL)
    {
      fprintf(stderr, "ERROR: Invalid arguments\n");
      fprintf(stderr, "USAGE: a.out cache-size input-file\n");
      exit(EXIT_FAILURE);
    }

    char* input_line = malloc(128);

    while(fgets(input_line, 128, input_file))
    {
        char * i;                                 // Iterator for the string (1)
        char * j;                                 // Iterator for the string (2)
        i = input_line;                           // Both start at the input line
        j = input_line;

        while(*i)                                 // Looping across the line
        {
          if(!isalnum(*i)) { i+=1; continue; }   // Loops until an alphanumeric
          if(isalnum(*i))
          {
            j = i;                                // j is the start of the word
            while(*j && isalnum(*j)) j+=1;
            *j = '\0';
            if(j-i>2)                             // if the word is 3 or more characters
            {
              // perform some computation, store in the cache
              int total, index;                   // The total ascii for hash function
              total = 0;
              char *k = i;
              while(*k){                          // Loops over the word ... adds the ascii
                total += *k++;
              }
              index = total % size_of_cache;      // Hash function for the

              printf( "Word \"%s\" ==> %d (", i, index);

              if(*(cache+index) != NULL)          // If there is a value already in the cache
              {
                printf("realloc)\n");
                *(cache+index) = realloc( *(cache+index), 128*sizeof(char));
              }
              else                                // If there isnt a value in the cache yet
              {
                printf("calloc)\n");
                *(cache+index) = calloc(128, sizeof(char));         // allocating the proper space
              }
              strcpy(*(cache+index), i);          // Puts the word in the cache
            }
            i = j+1;
            continue;
          }
          i+=1;
        }
        memset(input_line, 0, 128);     // clears the memory in the line
    }

    printCache(cache, size_of_cache);
    free(input_line);                   // Freeing memory
    fclose(input_file);

    for(int i=0; i<size_of_cache; i+=1) // Freeing everything in the cache
    {
      if(*(cache+i) != NULL) free( *(cache+i));
    }
    free(cache);                        // Freeing the cache

  return EXIT_SUCCESS;
}
