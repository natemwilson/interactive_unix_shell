/* lex.h
   author: nate wilson
*/

#ifndef LEX_INCLUDED
#define LEX_INCLUDED

#include "dynarray.h"
#include <stdio.h>


/* A Token object can be either special or ordinary. */
enum TokenType {TOKEN_ORDINARY, TOKEN_SPECIAL};

/* A Token is either special or ordinary, and is expressed as string. */
struct Token
{
   /* The type of the token. */
   enum TokenType eType;

   /* The string which is the token's value. */
   char *pcValue;
};

/* Write all tokens in oTokens in logical order to stdout.  */
void lex_writeTokens(DynArray_T oTokens);

/* Free all of the tokens in oTokens. */
void lex_freeTokens(DynArray_T oTokens);

/* perform lexical analysis on a string pcLine, returns a token array*/
DynArray_T lex_lexLine(const char *pcLine);

/* read in a line from psFile, then return that line in string form */
char *lex_readLine(FILE *psFile);

#endif
