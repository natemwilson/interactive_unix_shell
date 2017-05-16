/*--------------------------------------------------------------------
  command.c                                                          
  Author: Nate Wilson                                                
  Description: ADT representing a shell command, with information 
  about the command's name, arguments and input/output redirection
  --------------------------------------------------------------------*/

#include "token.h"
#include "command.h"
#include "ish.h"
#include "dynarray.h"
#include "lex.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* structure that will be used to store command name, args, and 
   input output redirection */
struct Command
{
    /* first item is cmd name, all following items are cmd args */ 
   DynArray_T oTokens;
    /* string representation of stdin redirection */
   char *pcStdin;
    /* string representation of stdout redirection */
   char *pcStdout;
};

/* return pcStdin of oCommand */
char *Command_getStdin(Command_T oCommand)
{
   assert(oCommand != NULL);
   
   return oCommand->pcStdin;
}

/* return pcStdout of oCommand */
char *Command_getStdout(Command_T oCommand)
{
   assert(oCommand != NULL);
   
   return oCommand->pcStdout;
}

/* return oTokens of oCommand,
   otokens[i=0] == cmd name, otokens[i>0] == cmd args */
DynArray_T Command_getTokens(Command_T oCommand)
{
   assert(oCommand != NULL);
   
   return oCommand->oTokens;
}

/* free dynamically allocated memory associated with oCommand */
void Command_freeCommand(Command_T oCommand)
{
   assert(oCommand != NULL);
   /* from free man page: If ptr is NULL, no operation is performed. */
   free(oCommand->pcStdin);
   free(oCommand->pcStdout);
   free(oCommand);
}

/* write oCommand to stdout according to spec at
   http://www.cs.princeton.edu/courses/archive/spr17/
   cos217/asgts/07shell/shellsupplementary.html */
void Command_writeCommand(Command_T oCommand)
{
   size_t uLength; /* length of cmd token array */
   size_t uIndex; /* index used for looping*/
   Token_T oToken; /* current token, multiple uses*/
   
   assert(oCommand != NULL);
   
   /* do not attempt to write a command with no tokens */
   uLength = DynArray_getLength(oCommand->oTokens);
   assert(uLength > 0);

   /* print command name */
   oToken = DynArray_get(oCommand->oTokens, 0);
   printf("Command name: %s\n", Token_getValue(oToken));

   /* print command args */
   for (uIndex = 1; uIndex < uLength; uIndex++)
   {
      oToken = DynArray_get(oCommand->oTokens, uIndex);
      printf("Command arg: %s\n", Token_getValue(oToken));
   }
   
   /* print stdin/stdout */
   if (oCommand->pcStdin != NULL)
      printf("Command stdin: %s\n", oCommand->pcStdin);
   if (oCommand->pcStdout != NULL)
      printf("Command stdout: %s\n", oCommand->pcStdout);
}

/* take a token array created by the lexical analyzer, 
   and return a Command_T object, as described in 
   Command struct definition above */
Command_T Command_createCommand(DynArray_T oTokens)
{
   size_t uIndex; /* used for looping */
   size_t uLength; /* length of cmd token array */
   size_t uStdinTokenCount, uStdoutTokenCount; /* n stdin/out tokens */
   Token_T oToken, oNextToken; /* token pointers  */
   Command_T oCommand; /* command to create and return*/
   char *pcStdinReferenceString  = "<"; /* used for string comparing */
   const char *pcPgmName; /* the program name */
   
   assert(oTokens != NULL);

   pcPgmName = getPgmName();
   /* account for the empty cmd case, silently fail */
   uLength = DynArray_getLength(oTokens);
   if (uLength == 0) return NULL;
   /*  It is an error for the DynArray object to begin with a 
       special  token. */
   oToken = DynArray_get(oTokens, 0);
   if (Token_isSpecial(oToken))
   {
      fprintf(stderr, "%s: missing command name\n", pcPgmName);
      return NULL;
   }
   /* it is also an error for a DynArray to end with a special token*/
   oToken = DynArray_get(oTokens, uLength - 1);
   if (Token_isSpecial(oToken))
   {
      if (strcmp(Token_getValue(oToken), pcStdinReferenceString) == 0)
      {  
         fprintf(stderr,
                 "%s: standard input redirection without file name\n",
                 pcPgmName);
         return NULL;
      }
      else
      {
         fprintf(stderr,
                 "%s: standard output redirection without file name\n",
                 pcPgmName);
         return NULL;
      }  
   }
   /* past initial error checking, now build the command */
   /* allocate command struct, set address to oCommand*/
   oCommand = (struct Command*)malloc(sizeof(struct Command));
   if (oCommand == NULL)
   {perror(pcPgmName); exit(EXIT_FAILURE);}
   /* set tokens array */
   oCommand->oTokens = oTokens;
   /* initialize stdin and out strings*/
   oCommand->pcStdin = NULL;
   oCommand->pcStdout = NULL;
   /* multiple redirection check */
   uStdinTokenCount = 0;
   uStdoutTokenCount = 0;
   /* now check for only one std in and one stdout token */
   for (uIndex = 0; uIndex < uLength; uIndex++) {
      oToken = DynArray_get(oCommand->oTokens, uIndex);
      if (Token_isSpecial(oToken)) {
         if (strcmp(Token_getValue(oToken), pcStdinReferenceString) == 0)
            uStdinTokenCount++;
         else
            uStdoutTokenCount++;
      }                
   }
   if (uStdinTokenCount > 1)
   {
      fprintf(stderr, "%s: multiple redirection of standard input\n",
              pcPgmName);
      free(oCommand);
      return NULL;
   }
   if (uStdoutTokenCount > 1)
   {
      fprintf(stderr, "%s: multiple redirection of standard output\n",
              pcPgmName);
      free(oCommand);
      return NULL;
   }

   /* command creation loop */
   /* we can stop checking at length - 1 because we checked the end 
      of the array above */
   /* for each element, if special, check special type,allocate
      for proper string size and set accordingly */
   for (uIndex = 0; uIndex < uLength-1; uIndex++)
   {  
      oToken = DynArray_get(oCommand->oTokens, uIndex);
      if (Token_isSpecial(oToken))
      {  /*if special get the next token, the redirect string*/
         oNextToken = DynArray_get(oCommand->oTokens, uIndex+1);
         /* do not allow a special token immediately after a special token */
         if (Token_isSpecial(oNextToken))
         {   /* if stdin  */
            if (strcmp(Token_getValue(oToken),
                       pcStdinReferenceString) == 0) {
               fprintf(stderr,
                   "%s: standard output redirection without file name",
                       pcPgmName);
               free(oCommand);
               return NULL; }
            else /* if stdout*/ {
               fprintf(stderr,
                    "%s: standard input redirection without file name",
                       pcPgmName);
               free(oCommand);
               return NULL; }
         }
         /* if curr special token is stdin, set stdin in of command */
         if (strcmp(Token_getValue(oToken), pcStdinReferenceString) == 0)
         {         
            oCommand->pcStdin =
               (char*)malloc(strlen(Token_getValue(oNextToken)) + 1);
            if (oCommand->pcStdin == NULL) {
               perror(pcPgmName);
               free(oCommand);
               exit(EXIT_FAILURE);
            }
            strcpy(oCommand->pcStdin, Token_getValue(oNextToken));
         }
         else /*else it is stdout*/
         {
            oCommand->pcStdout =
               (char*)malloc(strlen(Token_getValue(oNextToken)) + 1);
            if (oCommand->pcStdout == NULL) {
               perror(pcPgmName);
               free(oCommand);
               exit(EXIT_FAILURE);
            }
            strcpy(oCommand->pcStdout, Token_getValue(oNextToken));
         }
         /* remove the special token and the one following it */
         (void) DynArray_removeAt(oCommand->oTokens, uIndex);
         (void) DynArray_removeAt(oCommand->oTokens, uIndex);
         /* free those tokens and their internals */
         Token_free(oToken);
         Token_free(oNextToken);
         /* update loop parameters to reflect new structure */
         uLength = uLength - 2;
         uIndex = uIndex - 1;
      }
   }
   return oCommand;
}
