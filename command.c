/* command.c
   author nate wilson
*/

#include "command.h"
#include "ish.h"
#include "dynarray.h"
#include "lex.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

struct Command
{
   DynArray_T oTokens;
   char *pcStdin;
   char *pcStdout;
};


Command_T Command_createCommand(DynArray_T oTokens)
{
   size_t uIndex;
   size_t uLength;
   size_t uStdinTokenCount;
   size_t uStdoutTokenCount;
   struct Token *psToken;
   struct Token *psNextToken;
   Command_T oCommand;
   char *pcStdinReferenceString  = "<";
   char *pcStdoutReferenceString = ">";

   assert(oTokens != NULL);

   uLength = DynArray_getLength(oTokens);

   assert(uLength > 0);

   char *pcPgmName = getPgmName();
      
   /*  It is an error for the DynArray object to begin with an 
       special  token. */
   psToken = DynArray_get(oTokens, 0);
   if (psToken->eType == TOKEN_SPECIAL)
   {
      fprintf(stderr, "%s: missing command name", pcPgmName);
      return NULL;
      /* how should this error be handled?? */
   }

   /* it is also an error for a DynArray to end with a special token*/
   psToken = DynArray_get(oTokens, uLength - 1);
   if (psToken->eType == TOKEN_SPECIAL)
   {
      if (strcmp(psToken->pcValue, pcStdinReferenceString) == 0)
      {  
         fprintf(stderr,
                 "%s: standard input redirection without file name",
                 pcPgmName);
         return NULL;
      }
      else
      {
         fprintf(stderr,
                 "%s: standard output redirection without file name",
                 pcPgmName);
         return NULL;
      }  
   }
   
   
   /* allocate command struct, set address to oCommand*/
   oCommand = (struct Command*)malloc(sizeof(struct Command));
   /* error check!!*/
   if (oCommand == NULL)
      return NULL;
   /* set tokens array */
   oCommand->oTokens = oTokens;
   /* initialize stdin and out strings*/
   oCommand->pcStdin = NULL;
   oCommand->pcStdout = NULL;


   /* multiple redirection check */
   uStdinTokenCount = 0;
   uStdOutTokenCount = 0;
   /* now check for only one std in and one stdout token */
   for (uIndex = 0; uIndex < uLength; uIndex++)
   {
      psToken = DynArray_get(oCommand->oTokens, uIndex);
      if (psToken->eType == TOKEN_SPECIAL)
      {
         if (strcmp(psToken->pcValue, pcStdinReferenceString) == 0)
         {
            uStdinTokenCount++;
         }
         else
         {
            uStdoutTokenCount++;
         }
      }                
   }
   if (uStdinTokenCount > 1)
   {
      fprintf(stderr, "%s: multiple redirection of standard input",
              pcPgmName);
      return NULL;
   }
   if (uStdoutTokenCount > 1)
   {
      fprintf(stderr, "%s: multiple redirection of standard output",
              pcPgmName);
      return NULL;
   }

   /* command creation loop */
   /* we can stop checking at length - 1 because we checked the end 
      of the array above */
   for (uIndex = 0; uIndex < uLength-1; uIndex++)
   {
      /* for each element, 
         if special, 
         check special type,
         allocate for proper string size and set accordingly */
      psToken = DynArray_get(oCommand->oTokens, uIndex);

      if (psToken->eType == TOKEN_SPECIAL)
      {        
         psNextToken = DynArray_get(oCommand->oTokens, uIndex+1);
         /* do not allow a special token immediately after a special token */
         if (psNextToken->eType == TOKEN_SPECIAL)
         {
            if (strcmp(psToken->pcValue, pcStdinReferenceString) == 0)
            {
               fprintf(stderr,
                   "%s: standard output redirection without file name",
                       pcPgmName);
               return NULL;
            }
            else
            {
               fprintf(stderr,
                    "%s: standard input redirection without file name",
                       pcPgmName);
               return NULL;
            }
         }
         
         /* if curr special token is stdin, set stdin in of command */
         if (strcmp(psToken->pcValue, pcStdinReferenceString) == 0)
         {
            
            oCommand->pcStdin =
               (char*)malloc(strlen(psNextToken->pcValue) + 1);
            /* error check! */
            strcpy(oCommand->pcStdin, psNextToken->pcValue);
         }
         else
         {
            oCommand->pcStdout =
               (char*)malloc(strlen(psNextToken->pcValue) + 1);
            /* error check ! */
            strcpy(oCommand->pcStdout, psNextToken->pcValue);
         }
         DynArray_removeAt(oCommand->oTokens, uIndex);
         DynArray_removeAt(oCommand->oTokens, uIndex);
         /* should I do some freeing here? */
         uLength = uLength - 2;
         uIndex = uIndex - 1;
      }
   }
   return oCommand;
}
