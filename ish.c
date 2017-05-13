/*--------------------------------------------------------------------*/
/* ish.c                                                           */
/* Author: Nate Wilson                                                */
/*--------------------------------------------------------------------*/

#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
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

const char *getPgmName()
{
   return pcPgmName;
}

/* in lieu of a true boolean type */
enum {FALSE, TRUE};

static char **ish_allocateAndFillArgvArray(char *apcArgv[], Command_T oCommand)
{
   size_t uLength;
   size_t uIndex;
   struct Token *psToken;

   uLength = DynArray_getLength(Command_getTokens(oCommand));

   apcArgv = malloc(sizeof(char *) * (uLength+1));
   if (apcArgv == NULL)
   {
      /* error check!! */
   }
   for (uIndex = 0; uIndex < uLength; uIndex++)
   {
      psToken = DynArray_get(Command_getTokens(oCommand), uIndex);

      apcArgv[uIndex] = malloc((sizeof(char)) *
                               (strlen(psToken->pcValue) + 1));
      strcpy(apcArgv[uIndex], psToken->pcValue);
   }

   apcArgv[uLength] = NULL;

   return apcArgv;
}



static void ish_freeArgvArray(char *apcArgv[])
{
   size_t uIndex = 0;
   while (apcArgv[uIndex] != NULL)
   {
      free(apcArgv[uIndex]);
      uIndex++;
   }
   free(apcArgv);
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

static int ish_isBuiltIn(Command_T oCommand)
{
   DynArray_T oTokens;
   struct Token *psToken;
   
   oTokens = Command_getTokens(oCommand);

   psToken = DynArray_get(oTokens, 0);

   if ((strcmp(psToken->pcValue, "setenv")   == 0) ||
       (strcmp(psToken->pcValue, "unsetenv") == 0) ||
       (strcmp(psToken->pcValue, "cd")       == 0) ||
       (strcmp(psToken->pcValue, "exit")     == 0))
      return TRUE;
   else
      return FALSE;
}

void ish_handleRedirection(Command_T oCommand)
{
   char *pcStdin;
   char *pcStdout;
   int iFd;
   int iRet;
   
   pcStdin = Command_getStdin(oCommand);
   pcStdout = Command_getStdout(oCommand);

   /* handle stdout first */
   

   /* The permissions of the newly-created file. */
   if (pcStdout != NULL)
   {
      enum {PERMISSIONS = 0600};
      
      iFd = creat(pcStdout, PERMISSIONS);
      if (iFd == -1) {perror(pcPgmName); exit(EXIT_FAILURE); }
      
      iRet = close(1);
      if (iRet == -1) {perror(pcPgmName); exit(EXIT_FAILURE); }
      
      iRet = dup(iFd);
      if (iRet == -1) {perror(pcPgmName); exit(EXIT_FAILURE); }

      iRet = close(iFd);
      if (iRet == -1) {perror(pcPgmName); exit(EXIT_FAILURE); }
   }
   if (pcStdin != NULL)
   {
      iFd = open(pcStdin, O_RDONLY);
      if (iFd == -1) {perror(pcPgmName); exit(EXIT_FAILURE); }

      iRet = close(0);
      if (iRet == -1) {perror(pcPgmName); exit(EXIT_FAILURE); }

      iRet = dup(iFd);
      if (iRet == -1) {perror(pcPgmName); exit(EXIT_FAILURE); }

      iRet = close(iFd);
      if (iRet == -1) {perror(pcPgmName); exit(EXIT_FAILURE); }
      
   }
       
}

static void ish_handleBuiltIn(Command_T oCommand, char *pcLine)
{
   DynArray_T oTokens;
   size_t uLength;
   struct Token *psCmdName;
   struct Token *psCmdArg1;
   struct Token *psCmdArg2;
   char *pcHome;
   
   oTokens = Command_getTokens(oCommand);

   psCmdName = DynArray_get(oTokens, 0);
   uLength = DynArray_getLength(oTokens);
   /* handle exit */
   if (strcmp(psCmdName->pcValue, "exit") == 0)
   {
      if (uLength > 1)
      {
         fprintf(stderr, "%s: too many arguments\n", pcPgmName);
         return;
      }
      /* deallocate  */
      
      Command_freeCommand(oCommand);
      lex_freeTokens(oTokens);
      DynArray_free(oTokens);
      free(pcLine);
      printf("\n");
      exit(0);
   }

   /* handle setenv */
   if (strcmp(psCmdName->pcValue, "setenv") == 0)
   {
      /* error for setenv to have 0 or more than 2 args. */
      if (uLength == 1) /* % setenv */
      {
         fprintf(stderr, "%s: missing variable\n", pcPgmName);
         return;
      }
      if (uLength > 3) /* % setenv a b c*/
      {
         fprintf(stderr, "%s: too many arguments\n", pcPgmName);
         return;
      }
      if (uLength == 3) /* % setenv a b */
      {
         psCmdArg1 = DynArray_get(oTokens, 1);
         psCmdArg2 = DynArray_get(oTokens, 2);
         setenv(psCmdArg1->pcValue, psCmdArg2->pcValue, TRUE);
         return;
      }
      if (uLength == 2) /* % setenv a -- default sets to empty string */
      {
         psCmdArg1 = DynArray_get(oTokens, 1);
         setenv(psCmdArg1->pcValue, "", TRUE);
         return;
      }
   }
   /* handle unsetenv */
   if (strcmp(psCmdName->pcValue, "unsetenv") == 0)
   {
      /* It is an error for an unsetenv command to have zero command-line arguments or more than one command-line argument.*/
      if (uLength == 1)
      {
         fprintf(stderr, "%s: missing variable\n", pcPgmName);
         return;
      }
      if (uLength > 2)
      {
         fprintf(stderr, "%s: too many arguments\n", pcPgmName);
         return;
      }
      if (uLength == 2)
      {
         psCmdArg1 = DynArray_get(oTokens, 1);
         unsetenv(psCmdArg1->pcValue);
         return;
      }
      
   }
   /* handle cd */
   if (strcmp(psCmdName->pcValue, "cd") == 0)
   {
      /*  It is an error for a cd to have more than one argument. */
      if (uLength > 2)
      {
         fprintf(stderr, "%s: too many arguments\n", pcPgmName);
         return;
      }
      if (uLength == 1) /* cd [$HOME] unless home doesnt exist*/
      {
         pcHome = getenv("HOME");
         if (pcHome == NULL)
         {
            fprintf(stderr, "%s: HOME not set\n", pcPgmName);
            return;              
         }
         else
         {
            chdir(pcHome);
            /*should i do some error checking here? */
            return;
         }
      }
      if (uLength == 2) /* % cd path/to/wherever */
      {
         psCmdArg1 = DynArray_get(oTokens, 1);
         chdir(psCmdArg1->pcValue);
         return;
      }
   }
}

int main(int argc, char *argv[])
{
   char *pcLine;
   DynArray_T oTokens;
   int iRet;
   Command_T oCommand;
   pid_t iPid;
   char **apcArgv;

   pcPgmName = argv[0];
   printf("%% ");
   while ((pcLine = readLine(stdin)) != NULL)
   {
      printf("%s\n", pcLine);
      iRet = fflush(stdout);
      if (iRet == EOF)
      {perror(pcPgmName); exit(EXIT_FAILURE);}
      oTokens = lex_lexLine(pcLine);
      /* do we have a valid token array? */
      if (oTokens != NULL)
      {
         oCommand = Command_createCommand(oTokens);
         /* do we have a valid command */
         if (oCommand != NULL)
         {
            /* is the command setenv, unsetenv, cd or exit? */
            if (ish_isBuiltIn(oCommand))
               ish_handleBuiltIn(oCommand, pcLine);
            else /* if the command is NOT a builtin */
            {
               iPid = fork();
               if (iPid == 0)
               {
                  /* handle redirection here */
                  if ((Command_getStdin(oCommand) != NULL) ||
                      (Command_getStdout(oCommand) != NULL))
                     ish_handleRedirection(oCommand);
                  /* this causes a warning, you should do error checking \
                     below depending on exactly how to handle the other
                     errors above */
                  apcArgv = ish_allocateAndFillArgvArray(apcArgv,
                                                         oCommand);
                  execvp(apcArgv[0], apcArgv);
                  perror(apcArgv[0]);
                  /* free the argv array here */
                  ish_freeArgvArray(apcArgv);
                  exit(EXIT_FAILURE);
               }
               iPid = wait(NULL);
               if (iPid == -1) {perror(argv[0]); exit(EXIT_FAILURE); }
            }
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
