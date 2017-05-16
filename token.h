/*--------------------------------------------------------------------*/
/* token.h                                                          */
/* Author: Nate Wilson                                                */
/*--------------------------------------------------------------------*/


#ifndef TOKEN_INCLUDED
#define TOKEN_INCLUDED
/* command_t will be an object to the user but is in reality a
   pointer to a command structure */
typedef struct Token *Token_T;

/* A Token object can be either special or ordinary. */
enum TokenType {TOKEN_ORDINARY, TOKEN_SPECIAL};

/* Create and return a token whose type is eTokenType and whose
   value consists of string pcValue.  The caller owns the token. */
Token_T Token_new(enum TokenType eTokenType, char *pcValue);

/* free memory allocated for oToken */
void Token_free(Token_T oToken);

/* is oToken ordinary? return 1 if true */
int Token_isOrdinary(Token_T oToken);

/* is oToken special? return 1 if true  */
int Token_isSpecial(Token_T oToken);

/* return value associated with oToken */
char *Token_getValue(Token_T oToken);

#endif
