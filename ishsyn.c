/*--------------------------------------------------------------------*/
/* ishsyn.c                                                           */
/* Author: Nate Wilson                                                */
/*--------------------------------------------------------------------*/

#include "token.h"
#include "ish.h"
#include "command.h"
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
/* implements the syntactic analyzer, returns 0 on success
   argv array is a string array of command line args
   argc is the count of arguments in argv array */
int main(int argc, char *argv[])
{
   char *pcLine;
   DynArray_T oTokens;
   int iRet;
   Command_T oCommand;

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
         oCommand = Command_createCommand(oTokens);
         if (oCommand != NULL)
         {
            Command_writeCommand(oCommand);
            /* free commad struct and internals */
            Command_freeCommand(oCommand);
         }
      /* free each token in dynarray tokens */
      lex_freeTokens(oTokens);
      /* free dynarray */
      DynArray_free(oTokens);

      }
      
      free(pcLine);
      printf("%% ");
   }
   printf("\n");
   return 0;
}
