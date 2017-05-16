/* token.c
   author: nate wilson
   description: adt that stores a string and deems it either 
   special or ordinary
*/

#include "ish.h"
#include "token.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* A Token is either special or ordinary, and is expressed as string. */
struct Token
{
   /* The type of the token. */
   enum TokenType eType;

   /* The string which is the token's value. */
   char *pcValue;
};


/* Create and return a token whose type is eTokenType and whose
   value consists of string pcValue.  The caller owns the token. */
Token_T Token_new(enum TokenType eTokenType,
                                  char *pcValue)
{
   struct Token *psToken;
   const char *pcPgmName;

   assert(pcValue != NULL);

   pcPgmName = getPgmName();

   psToken = (struct Token*)malloc(sizeof(struct Token));
   if (psToken == NULL)
   {perror(pcPgmName); exit(EXIT_FAILURE);}

   psToken->eType = eTokenType;
   psToken->pcValue = (char*)malloc(strlen(pcValue) + 1);
   if (psToken->pcValue == NULL)
   { perror(pcPgmName); exit(EXIT_FAILURE);}

   strcpy(psToken->pcValue, pcValue);

   return psToken;
}

void Token_free(Token_T oToken)
{
   free(oToken->pcValue);
   free(oToken);
}

int Token_isOrdinary(Token_T oToken)
{
   assert(oToken != NULL);
   return (oToken->eType == TOKEN_ORDINARY);
}

int Token_isSpecial(Token_T oToken)
{
   assert(oToken != NULL);
   return (oToken->eType == TOKEN_SPECIAL);
}

char *Token_getValue(Token_T oToken)
{
   assert(oToken != NULL);
   return oToken->pcValue;
}
