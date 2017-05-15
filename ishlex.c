/*--------------------------------------------------------------------*/
/* ishlex.c                                                           */
/* Author: Nate Wilson                                                */
/*--------------------------------------------------------------------*/

#include "ish.h"
#include "lex.h"
#include "dynarray.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* program name, filled in by main */
static const char *pcPgmName;

const char *getPgmName(void)
{
   return pcPgmName;
}


/* this function implements a client of the lexical analyzer
   return 0 on success, 1 otherwise
   argv array is an array with command line arguments
   argc is the count of arguments in argv array */
int main(int argc, char *argv[])
{
   char *pcLine;
   DynArray_T oTokens;
   int iRet;

   pcPgmName = argv[0];
   printf("%% ");
   while ((pcLine = lex_readLine(stdin)) != NULL)
   {
      printf("%s\n", pcLine);
      iRet = fflush(stdout);
      if (iRet == EOF)
      {perror(pcPgmName); exit(EXIT_FAILURE);}
      oTokens = lex_lexLine(pcLine);
      if (oTokens != NULL)
      {
         lex_writeTokens(oTokens);
         lex_freeTokens(oTokens);
         DynArray_free(oTokens);
      }
      free(pcLine);
      printf("%% ");
   }
   printf("\n");
   return 0;
}
