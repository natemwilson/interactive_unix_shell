/* lex.h
   author: nate wilson
*/

#ifndef LEX_INCLUDED
#define LEX_INCLUDED

#include "dynarray.h"

/* A Token object can be either special or ordinary. */
enum TokenType {TOKEN_ORDINARY, TOKEN_SPECIAL};

/* Write all tokens in oTokens in logical order to stdout.  */
void lex_writeTokens(DynArray_T oTokens);


/* Free all of the tokens in oTokens. */
void lex_freeTokens(DynArray_T oTokens);

/* Create and return a token whose type is eTokenType and whose
   value consists of string pcValue.  The caller owns the token. */
struct Token *lex_newToken(enum TokenType eTokenType,
                           char *pcValue);

DynArray_T lex_lexLine(const char *pcLine);

#endif
