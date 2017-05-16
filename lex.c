/*--------------------------------------------------------------------*/
/* lex.c                                                              */
/* Author: Nate Wilson                                                */
/*--------------------------------------------------------------------*/

#include "token.h"
#include "lex.h"
#include "ish.h"
#include "dynarray.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>


/* read in a line from psFile, then return that line in string form */
char *lex_readLine(FILE *psFile)
{
   enum {INITIAL_LINE_LENGTH = 2};
   enum {GROWTH_FACTOR = 2};

   size_t uLineLength = 0;
   size_t uPhysLineLength = INITIAL_LINE_LENGTH;
   char *pcLine;
   int iChar;
   const char *pcPgmName;
   
   assert(psFile != NULL);

   pcPgmName = getPgmName();
   
   /* If no lines remain, return NULL. */
   if (feof(psFile))
      return NULL;
   iChar = fgetc(psFile);
   if (iChar == EOF)
      return NULL;

   /* Allocate memory for the string. */
   pcLine = (char*)malloc(uPhysLineLength);
   if (pcLine == NULL)
   {perror(pcPgmName); exit(EXIT_FAILURE);}
   /* Read characters into the string. */
   while ((iChar != '\n') && (iChar != EOF))
   {
      if (uLineLength == uPhysLineLength)
      {
         uPhysLineLength *= GROWTH_FACTOR;
         pcLine = (char*)realloc(pcLine, uPhysLineLength);
         if (pcLine == NULL)
         {perror(pcPgmName); exit(EXIT_FAILURE);}
      }
      pcLine[uLineLength] = (char)iChar;
      uLineLength++;
      iChar = fgetc(psFile);
   }

   /* Append a null character to the string. */
   if (uLineLength == uPhysLineLength)
   {
      uPhysLineLength++;
      pcLine = (char*)realloc(pcLine, uPhysLineLength);
      if (pcLine == NULL)
      {perror(pcPgmName); exit(EXIT_FAILURE);}
   }
   pcLine[uLineLength] = '\0';

   return pcLine;
}

/* Write all tokens in oTokens to stdout in same sequence they 
   came in and according to spec.  */
void lex_writeTokens(DynArray_T oTokens)
{
   size_t u;
   size_t uLength;
   Token_T oToken;

   assert(oTokens != NULL);

   uLength = DynArray_getLength(oTokens);

   for (u = 0; u < uLength; u++)
   {
      oToken = DynArray_get(oTokens, u);
      if (Token_isOrdinary(oToken))
         printf("Token: %s (ordinary)\n", Token_getValue(oToken));
      else
         printf("Token: %s (special)\n", Token_getValue(oToken));
   }
}

/* Free all of the tokens in oTokens. */
void lex_freeTokens(DynArray_T oTokens)
{
   size_t u;
   size_t uLength;
   Token_T oToken;

   assert(oTokens != NULL);

   uLength = DynArray_getLength(oTokens);

   for (u = 0; u < uLength; u++)
   {
      oToken = DynArray_get(oTokens, u);
      Token_free(oToken);
   }
}

/* add a special token using the char c to oTokens */
static void lex_addSpecialToken(char c, DynArray_T oTokens)
{
   char pcBuffer[2];
   Token_T oToken;
   int iSuccessful;
   const char *pcPgmName;
   pcPgmName = getPgmName();
   
   pcBuffer[0] = c;
   pcBuffer[1] = '\0';
   oToken = Token_new(TOKEN_SPECIAL, pcBuffer);
   iSuccessful = DynArray_add(oTokens, oToken);
   if (! iSuccessful)
   {
      fprintf(stderr, "%s: insufficient memory\n", pcPgmName);
      exit(EXIT_FAILURE);
   }
}

/*  add an ordinary token to oTokens using pcBuffer. takes uBufferIndex
    also which is needed to properly create the token  */
static void lex_addOrdinaryToken(char *pcBuffer,
                      size_t uBufferIndex,
                      DynArray_T oTokens)
{
   Token_T oToken;
   int iSuccessful;
   const char *pcPgmName;
   pcPgmName = getPgmName();
   assert(pcBuffer != NULL);
   pcBuffer[uBufferIndex] = '\0';
   oToken = Token_new(TOKEN_ORDINARY, pcBuffer);
   iSuccessful = DynArray_add(oTokens, oToken);
   if (! iSuccessful)
   {
      fprintf(stderr, "%s: insufficient memory\n", pcPgmName);
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
   size_t uBufferIndex = 0;

   char c;
   
   const char *pcPgmName = getPgmName();
   
   DynArray_T oTokens;

   assert(pcLine != NULL);

   /* Create an empty token DynArray object. */
   oTokens = DynArray_new(0);
   if (oTokens == NULL)
   {
      fprintf(stderr, "%s: insufficient memory\n", pcPgmName);
      exit(EXIT_FAILURE);
   }
   /* Allocate memory for a buffer that is large enough to store the
      largest token that might appear within pcLine. */
   pcBuffer = (char*)malloc(strlen(pcLine) + 1);
   if (pcBuffer == NULL)
   {
      fprintf(stderr, "%s: insufficient memory\n", pcPgmName);
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
               lex_addSpecialToken(c, oTokens);
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
               lex_addOrdinaryToken(pcBuffer, uBufferIndex, oTokens);
               uBufferIndex = 0;
               free(pcBuffer);
               return oTokens;
            }
            else if ((c == '>') || (c == '<'))
            {
               lex_addSpecialToken(c, oTokens);
               eState = STATE_SPECIAL;
            }
            else if (c == '\"')
            {
               eState = STATE_ESCAPE_IN;
            }
            else if (isspace(c))
            {
               lex_addOrdinaryToken(pcBuffer, uBufferIndex, oTokens);
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
               lex_addSpecialToken(c, oTokens);
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
               lex_addOrdinaryToken(pcBuffer, uBufferIndex, oTokens);
               uBufferIndex = 0;
               free(pcBuffer);
               return oTokens;
            }
            else if ((c == '>') || (c == '<'))
            {
               lex_addOrdinaryToken(pcBuffer, uBufferIndex, oTokens);
               lex_addSpecialToken(c, oTokens);
               uBufferIndex = 0;               
               eState = STATE_SPECIAL;
            }
            else if (c == '\"')
            {
               eState = STATE_ESCAPE_IN;
            }
            else if (isspace(c))
            {
               lex_addOrdinaryToken(pcBuffer, uBufferIndex, oTokens);
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
