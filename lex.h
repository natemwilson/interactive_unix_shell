/* lex.h
   author: nate wilson
*/

#ifndef LEX_INCLUDED
#define LEX_INCLUDED

#include "dynarray.h"
#include <stdio.h>


/* Write all tokens in oTokens in logical order to stdout.  */
void lex_writeTokens(DynArray_T oTokens);

/* Free all of the tokens in oTokens. */
void lex_freeTokens(DynArray_T oTokens);

/* perform lexical analysis on a string pcLine, returns a token array*/
DynArray_T lex_lexLine(const char *pcLine);

/* read in a line from psFile, then return that line in string form */
char *lex_readLine(FILE *psFile);

#endif
