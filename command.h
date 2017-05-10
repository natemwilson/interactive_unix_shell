/*--------------------------------------------------------------------*/
/* command.h                                                          */
/* Author: Nate Wilson                                                */
/*--------------------------------------------------------------------*/


#ifndef COMMAND_INCLUDED
#define COMMAND_INCLUDED

#include <stddef.h>

typedef struct Command *Command_T;

/* take a dynarray and create the return a command */
Command_T Command_createCommand(DynArray_T);

#endif
