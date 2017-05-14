/*--------------------------------------------------------------------*/
/* command.h                                                          */
/* Author: Nate Wilson                                                */
/*--------------------------------------------------------------------*/


#ifndef COMMAND_INCLUDED
#define COMMAND_INCLUDED

#include "dynarray.h"
#include <stddef.h>
#include "lex.h"

/* command_t will be an object to the user but is in reality a 
   pointer to a command structure */
typedef struct Command *Command_T;

/* write oCommand to stdout */
void Command_writeCommand(Command_T oCommand);

/* return the token array stored in oCommand*/
DynArray_T Command_getTokens(Command_T oCommand);

/* return a string name representing oCommand input redirection. 
   return null if stdin  */
char *Command_getStdin(Command_T oCommand);

/* return a string name representing oCommand output redirection. 
   return null if stdout  */
char *Command_getStdout(Command_T oCommand);

/* take a dynarray oTokens and create the return a command_t */
Command_T Command_createCommand(DynArray_T oTokens);

/* free memory allocated during creation of oCommand  */
void Command_freeCommand(Command_T oCommand);

#endif
