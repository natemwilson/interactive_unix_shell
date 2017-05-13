/*--------------------------------------------------------------------*/
/* command.h                                                          */
/* Author: Nate Wilson                                                */
/*--------------------------------------------------------------------*/


#ifndef COMMAND_INCLUDED
#define COMMAND_INCLUDED

#include "dynarray.h"
#include <stddef.h>
#include "lex.h"

typedef struct Command *Command_T;

/* write command to stdout */
void Command_writeCommand(Command_T oCommand);

/* return the token array in a command*/
DynArray_T Command_getTokens(Command_T oCommand);

/* return a string name representing the command input redirection. 
   return null if stdin  */
char *Command_getStdin(Command_T oCommand);

/* return a string name representing the command output redirection. 
   return null if stdout  */
char *Command_getStdout(Command_T oCommand);

/* take a dynarray and create the return a command */
Command_T Command_createCommand(DynArray_T oTokens);

/* free memory allocated during command creation */
void Command_freeCommand(Command_T oCommand);

#endif
