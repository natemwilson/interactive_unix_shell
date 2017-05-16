/*--------------------------------------------------------------------*/
/* ish.c                                                           */
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
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>


/* program name, filled in by main */
static const char *pcPgmName;

const char *getPgmName(void)
{
   return pcPgmName;
}

/* in lieu of a true boolean type */
enum {FALSE, TRUE};

/* takes pointer apcArgv array, and a command oCommand, and stores
   oCommand name and args properly into the array, adding a null 
   terminator to the end, and allocating along the way
 Returns pointer to allocated apcArgv array or a NULL if command is empty*/
static char **ish_allocateAndFillArgvArray(Command_T oCommand)
{
   size_t uLength;
   size_t uIndex;
   Token_T oToken;
   char **apcArgv;
   
   uLength = DynArray_getLength(Command_getTokens(oCommand));

   apcArgv = malloc(sizeof(char *) * (uLength+1));
   if (apcArgv == NULL)
   {perror(pcPgmName); exit(EXIT_FAILURE);}
   
   for (uIndex = 0; uIndex < uLength; uIndex++)
   {
      oToken = DynArray_get(Command_getTokens(oCommand), uIndex);

      apcArgv[uIndex] = malloc(strlen(Token_getValue(oToken)) + 1);
      if (apcArgv[uIndex] == NULL)
      {perror(pcPgmName); exit(EXIT_FAILURE);}
      
      strcpy(apcArgv[uIndex], Token_getValue(oToken));
   }

   /* add null terminator according to execvp spec */
   apcArgv[uLength] = NULL;

   return apcArgv;
}


/* free memory allocated associated with the apcArgv string array */
static void ish_freeArgvArray(char *apcArgv[])
{
   size_t uIndex = 0;
   
   assert(apcArgv!=NULL);
   
   while (apcArgv[uIndex] != NULL)
   {
      free(apcArgv[uIndex]);
      uIndex++;
   }
   free(apcArgv);
}

/* is oCommand one of the four implemented builtins?
  return True is yes, False if no*/
static int ish_isBuiltIn(Command_T oCommand)
{
   DynArray_T oTokens;
   Token_T oToken;
   
   oTokens = Command_getTokens(oCommand);

   oToken = DynArray_get(oTokens, 0);

   if ((strcmp(Token_getValue(oToken), "setenv")   == 0) ||
       (strcmp(Token_getValue(oToken), "unsetenv") == 0) ||
       (strcmp(Token_getValue(oToken), "cd")       == 0) ||
       (strcmp(Token_getValue(oToken), "exit")     == 0))
      return TRUE;
   else
      return FALSE;
}

/* redirect stdin and stdout according to the specss of oCommand */
static void ish_handleRedirection(Command_T oCommand)
{
   char *pcStdin;
   char *pcStdout;
   int iFd;
   int iRet;

   /* get input/output redirection strings */
   pcStdin = Command_getStdin(oCommand);
   pcStdout = Command_getStdout(oCommand);

   /* handle stdout first */
   if (pcStdout != NULL)
   {
      /* The permissions of the newly-created file. */
      enum {PERMISSIONS = 0600};
      /* create the file/overwrite if it exists */
      iFd = creat(pcStdout, PERMISSIONS);
      if (iFd == -1) {perror(pcPgmName); exit(EXIT_FAILURE); }
      /* close, dup, close pattern redirects output */
      iRet = close(1);
      if (iRet == -1) {perror(pcPgmName); exit(EXIT_FAILURE); }
      iRet = dup(iFd);
      if (iRet == -1) {perror(pcPgmName); exit(EXIT_FAILURE); }
      iRet = close(iFd);
      if (iRet == -1) {perror(pcPgmName); exit(EXIT_FAILURE); }
   }

   /* handle stdin next */
   if (pcStdin != NULL)
   {
      /* open a file for reading */
      iFd = open(pcStdin, O_RDONLY);
      if (iFd == -1) {perror(pcPgmName); exit(EXIT_FAILURE); }
      /* close dup close pattern redirects input */
      iRet = close(0);
      if (iRet == -1) {perror(pcPgmName); exit(EXIT_FAILURE); }
      iRet = dup(iFd);
      if (iRet == -1) {perror(pcPgmName); exit(EXIT_FAILURE); }
      iRet = close(iFd);
      if (iRet == -1) {perror(pcPgmName); exit(EXIT_FAILURE); }
   }
}

/* handle one of the four builtin commands. should not be called 
   unless oCommand is a built in command, take pcLine in case of
   need to free it */
static void ish_handleBuiltIn(Command_T oCommand, char *pcLine)
{
   DynArray_T oTokens;
   size_t uLength;
   Token_T oCmdName;
   Token_T oCmdArg1;
   Token_T oCmdArg2;
   char *pcHome;
   int iRet;

   assert(pcLine != NULL);
   assert(ish_isBuiltIn(oCommand)); 
   
   oTokens = Command_getTokens(oCommand);
   oCmdName = DynArray_get(oTokens, 0);
   uLength = DynArray_getLength(oTokens);

   /* handle exit */
   if (strcmp(Token_getValue(oCmdName), "exit") == 0)
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
      exit(0);
   }

   /* handle setenv */
   if (strcmp(Token_getValue(oCmdName), "setenv") == 0)
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
         oCmdArg1 = DynArray_get(oTokens, 1);
         oCmdArg2 = DynArray_get(oTokens, 2);
         setenv(Token_getValue(oCmdArg1),
                Token_getValue(oCmdArg2), TRUE);
         return;
      }
      if (uLength == 2) /* % setenv a -- default sets to empty string */
      {
         oCmdArg1 = DynArray_get(oTokens, 1);
         setenv(Token_getValue(oCmdArg1), "", TRUE);
         return;
      }
   }
   /* handle unsetenv */
   if (strcmp(Token_getValue(oCmdName), "unsetenv") == 0)
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
         oCmdArg1 = DynArray_get(oTokens, 1);
         unsetenv(Token_getValue(oCmdArg1));
         return;
      }
   }
   /* handle cd */
   if (strcmp(Token_getValue(oCmdName), "cd") == 0)
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
            iRet = chdir(pcHome);
            /*should i do some error checking here? */
            if (iRet == -1)
               fprintf(stderr, "%s: No such file or directory\n",
                       pcPgmName);
            return;
         }
      }
      if (uLength == 2) /* % cd path/to/wherever */
      {
         oCmdArg1 = DynArray_get(oTokens, 1);
         iRet = chdir(Token_getValue(oCmdArg1));
         if (iRet == -1)
            fprintf(stderr, "%s: No such file or directory\n",
                    pcPgmName);
         return;
      }
   }
}

/* implements the shell command execution program with 4 builtins 
   and input/output redirection. argc is the number of command line
   arguments and argv are those arguments. return 0 if successful. */
int main(int argc, char *argv[])
{
   char *pcLine;
   DynArray_T oTokens;
   int iRet;
   Command_T oCommand;
   pid_t iPid;
   char **apcArgv = NULL;

   pcPgmName = argv[0];
   printf("%% ");
   while ((pcLine = lex_readLine(stdin)) != NULL)
   {  printf("%s\n", pcLine);
      iRet = fflush(stdout);
      if (iRet == EOF)
      {perror(pcPgmName); exit(EXIT_FAILURE);}
      oTokens = lex_lexLine(pcLine);
      if (oTokens != NULL) /* do we have a valid token array? */
      {  oCommand = Command_createCommand(oTokens);
         if (oCommand != NULL) /* do we have a valid command */
         {  
            if (ish_isBuiltIn(oCommand)) /* is cmd a builtin?*/
               ish_handleBuiltIn(oCommand, pcLine);
            else /* if the command is NOT a builtin */
            {
               apcArgv = ish_allocateAndFillArgvArray(oCommand);
               iPid = fork();
               if (iPid == 0) /* child process */
               {  /* handle redirection here */
                  if ((Command_getStdin(oCommand) != NULL) ||
                      (Command_getStdout(oCommand) != NULL))
                     ish_handleRedirection(oCommand);
                  execvp(apcArgv[0], apcArgv);
                  perror(pcPgmName);
                  exit(EXIT_FAILURE); }
               iPid = wait(NULL);
               if (iPid == -1) {perror(argv[0]); exit(EXIT_FAILURE); }
               ish_freeArgvArray(apcArgv); /* free the argv array */
            }
            Command_freeCommand(oCommand);/*free cmd struct & intrnls */
         }
         lex_freeTokens(oTokens); /* free each token in oTokens */
         DynArray_free(oTokens); /* free dynarray struct */
      }
      free(pcLine);
      printf("%% ");
   }
   printf("\n");
   return 0;}
