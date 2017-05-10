/*--------------------------------------------------------------------*/
/* ishlex.c                                                           */
/* Author: Nate Wilson                                                */
/*--------------------------------------------------------------------*/

#include "lex.h"
#include "dynarray.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* program name, filled in by main */
static const char *pcPgmName;

const char *getPgmName()
{
   return pcPgmName;
}

static char *readLine(FILE *psFile)
{
   enum {INITIAL_LINE_LENGTH = 2};
   enum {GROWTH_FACTOR = 2};

   size_t uLineLength = 0;
   size_t uPhysLineLength = INITIAL_LINE_LENGTH;
   char *pcLine;
   int iChar;

   assert(psFile != NULL);

   /* If no lines remain, return NULL. */
   if (feof(psFile))
      return NULL;
   iChar = fgetc(psFile);
   if (iChar == EOF)
      return NULL;

   /* Allocate memory for the string. */
   pcLine = (char*)malloc(uPhysLineLength);
   if (pcLine == NULL)
   {perror(pcPgmName); exit(EXIT_FAILURE);}

   /* Read characters into the string. */
   while ((iChar != '\n') && (iChar != EOF))
   {
      if (uLineLength == uPhysLineLength)
      {
         uPhysLineLength *= GROWTH_FACTOR;
         pcLine = (char*)realloc(pcLine, uPhysLineLength);
         if (pcLine == NULL)
         {perror(pcPgmName); exit(EXIT_FAILURE);}
      }
      pcLine[uLineLength] = (char)iChar;
      uLineLength++;
      iChar = fgetc(psFile);
   }

   /* Append a null character to the string. */
   if (uLineLength == uPhysLineLength)
   {
      uPhysLineLength++;
      pcLine = (char*)realloc(pcLine, uPhysLineLength);
      if (pcLine == NULL)
      {perror(pcPgmName); exit(EXIT_FAILURE);}
   }
   pcLine[uLineLength] = '\0';

   return pcLine;
}

int main(int argc, char *argv[])
{
   char *pcLine;
   DynArray_T oTokens;
   int iRet;

   pcPgmName = argv[0];
   printf("%% ");
   while ((pcLine = readLine(stdin)) != NULL)
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
