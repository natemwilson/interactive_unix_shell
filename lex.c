/*--------------------------------------------------------------------*/
/* lex.c                                                              */
/* Author: Nate Wilson                                                */
/*--------------------------------------------------------------------*/

#include "ish.h"
#include "dynarray.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>


/* A Token object can be either special or ordinary. */
enum TokenType {TOKEN_ORDINARY, TOKEN_SPECIAL};

/*--------------------------------------------------------------------*/

/* A Token is either special or ordinary, and is expressed as a string. */

struct Token
{
   /* The type of the token. */
   enum TokenType eType;

   /* The string which is the token's value. */
   char *pcValue;
};


/* Write all tokens in oTokens in logical order to stdout.  */
void lex_writeTokens(DynArray_T oTokens)
{
   size_t u;
   size_t uLength;
   struct Token *psToken;

   assert(oTokens != NULL);

   uLength = DynArray_getLength(oTokens);

   for (u = 0; u < uLength; u++)
   {
      psToken = DynArray_get(oTokens, u);
      if (psToken->eType == TOKEN_ORDINARY)
         printf("Token: %s (ordinary)\n", psToken->pcValue);
      else
         printf("Token: %s (special)\n", psToken->pcValue);
   }
}

/* Free all of the tokens in oTokens. */

void lex_freeTokens(DynArray_T oTokens)
{
   size_t u;
   size_t uLength;
   struct Token *psToken;

   assert(oTokens != NULL);

   uLength = DynArray_getLength(oTokens);

   for (u = 0; u < uLength; u++)
   {
      psToken = DynArray_get(oTokens, u);
      free(psToken->pcValue);
      free(psToken);
   }
}

/* Create and return a token whose type is eTokenType and whose
   value consists of string pcValue.  The caller owns the token. */

static struct Token *lex_newToken(enum TokenType eTokenType,
                       char *pcValue)
{
   struct Token *psToken;

   assert(pcValue != NULL);

   psToken = (struct Token*)malloc(sizeof(struct Token));
   if (psToken == NULL)
   {
      fprintf(stderr, "insufficient memory\n");
      exit(EXIT_FAILURE);
   }
   psToken->eType = eTokenType;
   psToken->pcValue = (char*)malloc(strlen(pcValue) + 1);
   if (psToken->pcValue == NULL)
   {
      fprintf(stderr, "insufficient memory\n");
      exit(EXIT_FAILURE);
   }
   strcpy(psToken->pcValue, pcValue);

   return psToken;
}


/* add a special token using the char c to oTokens */
static void addSpecialToken(char c, DynArray_T oTokens)
{
   char pcBuffer[2];
   struct Token *psToken;
   int iSuccessful;
   
   pcBuffer[0] = c;
   pcBuffer[1] = '\0';
   psToken = lex_newToken(TOKEN_SPECIAL, pcBuffer);
   iSuccessful = DynArray_add(oTokens, psToken);
   if (! iSuccessful)
   {
      fprintf(stderr, "insufficient memory\n");
      exit(EXIT_FAILURE);
   }
}


/*  add an ordinary token to tokens using pcBuffer */
static void addOrdinaryToken(char *pcBuffer,
                      size_t uBufferIndex,
                      DynArray_T oTokens)
{
   struct Token *psToken;
   int iSuccessful;
   
   pcBuffer[uBufferIndex] = '\0';
   psToken = lex_newToken(TOKEN_ORDINARY, pcBuffer);
   iSuccessful = DynArray_add(oTokens, psToken);
   if (! iSuccessful)
   {
      fprintf(stderr, "insufficient memory\n");
      exit(EXIT_FAILURE);
   }
}

/* take a string pcLine and return a token array of ordinary and 
   special tokens.  return NULL if failure occurs*/
DynArray_T lex_lexLine(const char *pcLine)
{
   /* lexLine() uses a DFA approach.  It "reads" its characters from
      pcLine. The DFA has these three states: */
   enum LexState {STATE_START, STATE_ORDINARY, STATE_SPECIAL, STATE_ESCAPE_IN, STATE_ESCAPE_OUT};

   /* The current state of the DFA. */
   enum LexState eState = STATE_START;

   /* An index into pcLine. */
   size_t uLineIndex = 0;

   /* Pointer to a buffer in which the characters comprising each
      token are accumulated. */
   char *pcBuffer;

   /* An index into the buffer. */
   int uBufferIndex = 0;

   char c;
   
   char *pcPgmName = getPgmName();
   
   DynArray_T oTokens;


   assert(pcLine != NULL);

   /* Create an empty token DynArray object. */
   oTokens = DynArray_new(0);
   if (oTokens == NULL)
   {
      fprintf(stderr, "insufficient memory\n");
      exit(EXIT_FAILURE);
   }
   /* Allocate memory for a buffer that is large enough to store the
      largest token that might appear within pcLine. */
   pcBuffer = (char*)malloc(strlen(pcLine) + 1);
   if (pcBuffer == NULL)
   {
      fprintf(stderr, "insufficient memory\n");
      exit(EXIT_FAILURE);
   }

   for (;;)
   {
      /* read next char */
      c = pcLine[uLineIndex++];
      switch (eState)
      {
         case STATE_START:
            if (c == '\0')
            {
               free(pcBuffer);
               return oTokens;
            }
            else if ((c == '>') || (c == '<'))
            {
               addSpecialToken(c, oTokens);
               eState = STATE_SPECIAL;
            }
            else if (c == '\"')
            {
               eState = STATE_ESCAPE_IN;
            }
            else if (isspace(c))
            {
               eState = STATE_START;
            }
            else
            {
               pcBuffer[uBufferIndex++] = c;
               eState = STATE_ORDINARY;
            }
            break;

         case STATE_ESCAPE_IN:
            if (c == '\0')
            {
               free(pcBuffer);
               fprintf(stderr, "%s: unmatched quote\n", pcPgmName );
               lex_freeTokens(oTokens);
               DynArray_free(oTokens);
               return NULL;
            }
            else if ((c == '>') || (c == '<'))
            {
               pcBuffer[uBufferIndex++] = c;
               eState = STATE_ESCAPE_IN;
            }
            else if (c == '\"')
            {
               eState = STATE_ESCAPE_OUT;
            }
            else if (isspace(c))
            {
               pcBuffer[uBufferIndex++] = c;
               eState = STATE_ESCAPE_IN;
            }
            else
            {
               pcBuffer[uBufferIndex++] = c;
               eState = STATE_ESCAPE_IN;
            }
            break;
         case STATE_ESCAPE_OUT:
            if (c == '\0')
            {
               addOrdinaryToken(pcBuffer, uBufferIndex, oTokens);
               uBufferIndex = 0;
               free(pcBuffer);
               return oTokens;
            }
            else if ((c == '>') || (c == '<'))
            {
               addSpecialToken(c, oTokens);
               eState = STATE_SPECIAL;
            }
            else if (c == '\"')
            {
               eState = STATE_ESCAPE_IN;
            }
            else if (isspace(c))
            {
               addOrdinaryToken(pcBuffer, uBufferIndex, oTokens);
               uBufferIndex = 0;
               eState = STATE_START;
            }
            else
            {
               pcBuffer[uBufferIndex++] = c;
               eState = STATE_ORDINARY;
            }
            break;
         case STATE_SPECIAL:
            if (c == '\0')
            {
               free(pcBuffer);
               return oTokens;
            }
            else if ((c == '>') || (c == '<'))
            {
               addSpecialToken(c, oTokens);
               uBufferIndex = 0;
               eState = STATE_SPECIAL;
            }
            else if (c == '\"')
            {
               eState = STATE_ESCAPE_IN;
            }
            else if (isspace(c))
            {
               uBufferIndex = 0;
               eState = STATE_START;
            }
            else
            {
               pcBuffer[uBufferIndex++] = c;
               eState = STATE_ORDINARY;
            }
            break;
         case STATE_ORDINARY:
            if (c == '\0')
            {
               addOrdinaryToken(pcBuffer, uBufferIndex, oTokens);
               uBufferIndex = 0;
               free(pcBuffer);
               return oTokens;
            }
            else if ((c == '>') || (c == '<'))
            {
               addOrdinaryToken(pcBuffer, uBufferIndex, oTokens);
               addSpecialToken(c, oTokens);
               uBufferIndex = 0;               
               eState = STATE_SPECIAL;
            }
            else if (c == '\"')
            {
               eState = STATE_ESCAPE_IN;
            }
            else if (isspace(c))
            {
               addOrdinaryToken(pcBuffer, uBufferIndex, oTokens);
               uBufferIndex = 0;
               eState = STATE_START;
            }
            else
            {
               pcBuffer[uBufferIndex++] = c;
               eState = STATE_ORDINARY;
            }
            break;

         default:
            assert(0);
      }
   }
}

