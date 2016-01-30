/* javamode.c
 *
 * Copyright (c) 1999 Chris Smith
 *
 * This file is distributed under the GPL, version 2 or at your option any
 * later version.  See COPYING for details.
 */

#include <string.h>
#include <ctype.h>

#include "lpecomm.h"
#include "highlight.h"
#include "mode-utils.h"

int mode_accept(buffer *buf)
{
    char *suffix;

    suffix = strrchr(buf->name, '.');
    if (suffix == NULL) return 0;

    return mode_util_accept_extensions ( suffix, 0, 1, ".java" ) ||
    	   mode_util_accept_on_request ( buf->text->txt, 0, 1, "java" );
}

void mode_init(buffer *buf)
{
    if (buf->mode_name == NULL)
    {
        buf->hardtab = mode_util_get_option_with_default ( "javamode", "hardtab", 1 );
	buf->autoindent = mode_util_get_option_with_default ( "javamode", "autoindent", 1 );
	buf->offerhelp = mode_util_get_option_with_default ( "javamode", "offerhelp", 1 );
	buf->highlight = mode_util_get_option_with_default ( "javamode", "highlight", 1 );
	buf->flashbrace = mode_util_get_option_with_default ( "javamode", "flashbrace", 1 );
    }

    buf->mode_name = "javamode";

    buf->state_valid = buf->text;
    buf->state_valid_num = 0;
    buf->text->start_state = 0;
}

void mode_enter(buffer *buf)
{
    mode_util_set_slang_color ( "javamode", "ident", COLOR_IDENT, COLOR_IDENT_FG, COLOR_IDENT_BG );
    mode_util_set_slang_color ( "javamode", "symbol", COLOR_SYMBOL, COLOR_SYMBOL_FG, COLOR_SYMBOL_BG );
    mode_util_set_slang_color ( "javamode", "brace", COLOR_BRACE, COLOR_BRACE_FG, COLOR_BRACE_BG );
    mode_util_set_slang_color ( "javamode", "comment", COLOR_COMMENT, COLOR_COMMENT_FG, COLOR_COMMENT_BG );
    mode_util_set_slang_color ( "javamode", "keyword", COLOR_KEYWORD, COLOR_KEYWORD_FG, COLOR_KEYWORD_BG );
    mode_util_set_slang_color ( "javamode", "preproc", COLOR_PREPROC, COLOR_PREPROC_FG, COLOR_PREPROC_BG );
    mode_util_set_slang_color ( "javamode", "string", COLOR_STRING, COLOR_STRING_FG, COLOR_STRING_BG );
    mode_util_set_slang_color ( "javamode", "number", COLOR_NUMBER, COLOR_NUMBER_FG, COLOR_NUMBER_BG );
    mode_util_set_slang_color ( "javamode", "stringe", COLOR_STRINGE, COLOR_STRINGE_FG, COLOR_STRINGE_BG );
    mode_util_set_slang_color ( "javamode", "illegal", COLOR_ILLEGAL, COLOR_ILLEGAL_FG, COLOR_ILLEGAL_BG );
    mode_util_set_slang_color ( "javamode", "debug", COLOR_DEBUG, COLOR_DEBUG_FG, COLOR_DEBUG_BG );
}

/* Java implementation of brace flashing
 *
 * Rules:
 * 1. Braces are (), [], and {}, and they must be properly nested
 * 2. Ignore anything inside quotation marks (single or double)
 * 3. Disregard rule 2 if the quote is escaped
 * 4. Ignore anything in a C-style comment (slash-star)
 * 5. If currently in a comment, don't match with anything outside of it
 * 6. Print a silent warning in the minibuf for known mismatched braces
 */
int mode_flashbrace(buffer *buf)
{
    int is_comment;
    char brace_stack[1024];
    int sp;
    char ch, quote;

    if (buf->pos.col == 0) return 0;
    ch = buf->pos.line->txt[buf->pos.col - 1];
    if ((ch != ')') && (ch != ']') && (ch != '}')) return 0;

    brace_stack[0] = ch;
    sp = 1;
    buf->pos.col--;

    quote = '\0';
    is_comment = 0;

    do {
        char last_ch;

        while (buf->pos.col <= 0)
        {
            if (buf->pos.line == buf->scrollpos) return 0;
            buf->pos.line = buf->pos.line->prev;
            buf->linenum--;
            buf->pos.col = strlen(buf->pos.line->txt);
        }

        buf->pos.col--;
        last_ch = ch;
        ch = buf->pos.line->txt[buf->pos.col];

        if (is_comment)
        {
            if ((ch == '/') && (last_ch == '*')) is_comment = 0;
            continue;
        }

        if (quote != '\0')
        {
            if ((ch == '*') && (last_ch == '/'))
            {
                is_comment = 1;
                ch = '\0';  /* must find another '*' to close comment */
            }
            else if (ch == quote) quote = '\0';
            else if ((last_ch == quote) && (ch == '\\')) quote = 0;

            continue;
        }

        switch (ch)
        {
            case ')':
            case ']':
            case '}':
                if (sp == 1024) return 0;
                brace_stack[sp++] = ch;
                break;

            case '(':
                if (brace_stack[--sp] != ')')
                {
                    return -1;
                }
                break;

            case '[':
                if (brace_stack[--sp] != ']')
                {
                    return -1;
                }
                break;

            case '{':
                if (brace_stack[--sp] != '}')
                {
                    return -1;
                }
                break;

            case '\'':
            case '\"':
                quote = ch;
                break;

            case '\\':
                if ((last_ch == '\'') || (last_ch == '\"')) quote = last_ch;
                break;

            case '*':
                if (last_ch == '/')
                {
                    is_comment = 1;
                    ch = '\0';  /* must find another '*' to close comment */
                }
                break;

            case '/':
                if (last_ch == '*') return 0; /* we were in a comment */
                break;
        }
    } while (sp != 0);

    set_scr_col(buf);
    return 1;
}

enum _states
{
    STATE_BEGIN = 0,
    STATE_WHITESPACE,
    STATE_WORD,
    STATE_SLASH,
    STATE_COMMENT,
    STATE_COMMENT_STAR,
    STATE_COMMENT_DONE,
    STATE_LINE_COMMENT,
    STATE_STRING_CONST,
    STATE_STRING_CONST_ESC,
    STATE_STRING_CONST_ESCOCT_1,
    STATE_STRING_CONST_ESCOCT_2,
    STATE_CHAR_CONST,
    STATE_CHAR_CONST_ESC,
    STATE_CHAR_CONST_ESCOCT_1,
    STATE_CHAR_CONST_ESCOCT_2,
    STATE_0,
    STATE_NUM_WHOLE,
    STATE_NUM_HEX,
    STATE_NUM_LEAD0_OCT,
    STATE_NUM_LEAD0,
    STATE_NUM_SUFF,
    STATE_DOT,
    STATE_NUM_FRACT,
    STATE_FRACT_SUFF_E,
    STATE_FRACT_E_SIGN,
    STATE_FRACT_E_VAL,
    STATE_FRACT_SUFF,
    STATE_PANIC
};

/* This deserves a little explanation to say the least.  This table is used to
 * compare identifiers against valid keywords and decide whether to color them
 * as keywords.  Keywords are spelled out in columns of the array, and must be
 * in alphabetical order (by ASCII code -- so capital letters first.  The
 * number, w, is the number of entries in the table, including this one, that
 * begin with the same sequence of letters.
 *
 * For example, since there are four C keywords that begin with the letter 'c',
 * kwtbl[0][2].w == 4.  There are two keywords beginning 'con', and so
 * kwtbl[2][4].w == 2.
 */

#define N_KEYWORDS 59

typedef struct
{
    unsigned char ch;
    unsigned char w;
} kwtbl_ent;

kwtbl_ent kwtbl[][N_KEYWORDS] = {
{ {'a',1}, {'b',4}, {'b',3}, {'b',2}, {'b',1}, {'c',7}, {'c',6}, {'c',5}, {'c',4}, {'c',3}, {'c',2}, {'c',1}, {'d',3}, {'d',2}, {'d',1}, {'e',2}, {'e',1}, {'f',6}, {'f',5}, {'f',4}, {'f',3}, {'f',2}, {'f',1}, {'g',2}, {'g',1}, {'i',7}, {'i',6}, {'i',5}, {'i',4}, {'i',3}, {'i',2}, {'i',1}, {'l',1}, {'n',3}, {'n',2}, {'n',1}, {'o',2}, {'o',1}, {'p',4}, {'p',3}, {'p',2}, {'p',1}, {'r',2}, {'r',1}, {'s',5}, {'s',4}, {'s',3}, {'s',2}, {'s',1}, {'t',6}, {'t',5}, {'t',4}, {'t',3}, {'t',2}, {'t',1}, {'v',3}, {'v',2}, {'v',1}, {'w',1} },
{ {'b',1}, {'o',1}, {'r',1}, {'y',2}, {'y',1}, {'a',3}, {'a',2}, {'a',1}, {'h',1}, {'l',1}, {'o',2}, {'o',1}, {'e',1}, {'o',2}, {'o',1}, {'l',1}, {'x',1}, {'a',1}, {'i',2}, {'i',1}, {'l',1}, {'o',1}, {'u',1}, {'e',1}, {'o',1}, {'f',1}, {'m',2}, {'m',1}, {'n',4}, {'n',3}, {'n',2}, {'n',1}, {'o',1}, {'a',1}, {'e',1}, {'u',1}, {'p',1}, {'u',1}, {'a',1}, {'r',2}, {'r',1}, {'u',1}, {'e',2}, {'e',1}, {'h',1}, {'t',1}, {'u',1}, {'w',1}, {'y',1}, {'h',3}, {'h',2}, {'h',1}, {'r',3}, {'r',2}, {'r',1}, {'a',1}, {'o',2}, {'o',1}, {'h',1} },
{ {'s',1}, {'o',1}, {'e',1}, {'t',1}, {'v',1}, {'s',2}, {'s',1}, {'t',1}, {'a',1}, {'a',1}, {'n',2}, {'n',1}, {'f',1}, {' ',0}, {'u',1}, {'s',1}, {'t',1}, {'l',1}, {'n',2}, {'n',1}, {'o',1}, {'r',1}, {'t',1}, {'n',1}, {'t',1}, {' ',0}, {'p',2}, {'p',1}, {'n',1}, {'s',1}, {'t',2}, {'t',1}, {'n',1}, {'t',1}, {'w',1}, {'l',1}, {'e',1}, {'t',1}, {'c',1}, {'i',1}, {'o',1}, {'b',1}, {'s',1}, {'t',1}, {'o',1}, {'a',1}, {'p',1}, {'i',1}, {'n',1}, {'i',1}, {'r',2}, {'r',1}, {'a',1}, {'u',1}, {'y',1}, {'r',1}, {'i',1}, {'l',1}, {'i',1} },
{ {'t',1}, {'l',1}, {'a',1}, {'e',1}, {'a',1}, {'e',1}, {'t',1}, {'c',1}, {'r',1}, {'s',1}, {'s',1}, {'t',1}, {'a',1}, {' ',0}, {'b',1}, {'e',1}, {'e',1}, {'s',1}, {'a',2}, {'a',1}, {'a',1}, {' ',0}, {'u',1}, {'e',1}, {'o',1}, {' ',0}, {'l',1}, {'o',1}, {'e',1}, {'t',1}, {' ',0}, {'e',1}, {'g',1}, {'i',1}, {' ',0}, {'l',1}, {'r',1}, {'e',1}, {'k',1}, {'v',1}, {'t',1}, {'l',1}, {'t',1}, {'u',1}, {'r',1}, {'t',1}, {'e',1}, {'t',1}, {'c',1}, {'s',1}, {'o',2}, {'o',1}, {'n',1}, {'e',1}, {' ',0}, {' ',0}, {'d',1}, {'a',1}, {'l',1} },
{ {'r',1}, {'e',1}, {'k',1}, {' ',0}, {'l',1}, {' ',0}, {' ',0}, {'h',1}, {' ',0}, {'s',1}, {'t',1}, {'i',1}, {'u',1}, {' ',0}, {'l',1}, {' ',0}, {'n',1}, {'e',1}, {'l',2}, {'l',1}, {'t',1}, {' ',0}, {'r',1}, {'r',1}, {' ',0}, {' ',0}, {'e',1}, {'r',1}, {'r',1}, {'a',1}, {' ',0}, {'r',1}, {' ',0}, {'v',1}, {' ',0}, {' ',0}, {'a',1}, {'r',1}, {'a',1}, {'a',1}, {'e',1}, {'i',1}, {' ',0}, {'r',1}, {'t',1}, {'i',1}, {'r',1}, {'c',1}, {'h',1}, {' ',0}, {'w',2}, {'w',1}, {'s',1}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {'t',1}, {'e',1} },
{ {'a',1}, {'a',1}, {' ',0}, {' ',0}, {'u',1}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {'n',1}, {'l',1}, {' ',0}, {'e',1}, {' ',0}, {'d',1}, {' ',0}, {' ',0}, {'l',1}, {' ',0}, {' ',0}, {'e',1}, {'i',1}, {' ',0}, {' ',0}, {'m',1}, {'t',1}, {' ',0}, {'n',1}, {' ',0}, {'f',1}, {' ',0}, {'e',1}, {' ',0}, {' ',0}, {'t',1}, {' ',0}, {'g',1}, {'t',1}, {'c',1}, {'c',1}, {' ',0}, {'n',1}, {' ',0}, {'c',1}, {' ',0}, {'h',1}, {'r',1}, {' ',0}, {' ',0}, {'s',1}, {'i',1}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {'i',1}, {' ',0} },
{ {'c',1}, {'n',1}, {' ',0}, {' ',0}, {'e',1}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {'u',1}, {'t',1}, {' ',0}, {' ',0}, {' ',0}, {'s',1}, {' ',0}, {' ',0}, {'y',1}, {' ',0}, {' ',0}, {' ',0}, {'c',1}, {' ',0}, {' ',0}, {'e',1}, {' ',0}, {' ',0}, {'c',1}, {' ',0}, {'a',1}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {'o',1}, {' ',0}, {'e',1}, {'e',1}, {'t',1}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {'o',1}, {' ',0}, {' ',0}, {' ',0}, {'e',1}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {'l',1}, {' ',0} },
{ {'t',1}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {'e',1}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {'n',1}, {' ',0}, {' ',0}, {'e',1}, {' ',0}, {'c',1}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {'r',1}, {' ',0}, {' ',0}, {' ',0}, {'e',1}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {'n',1}, {' ',0}, {' ',0}, {' ',0}, {'n',1}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {'e',1}, {' ',0} },
{ {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {'t',1}, {' ',0}, {' ',0}, {'o',1}, {' ',0}, {'e',1}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {'d',1}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {'i',1}, {' ',0}, {' ',0}, {' ',0}, {'t',1}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0} },
{ {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {'s',1}, {' ',0}, {' ',0}, {'f',1}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {'z',1}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0} },
{ {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {'e',1}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0} },
{ {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {'d',1}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0} },
{ {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0}, {' ',0} }
};

/* Modifies the values of kwpos and kwend with respect to the next character of
 * an identifier.  kwpos points to the current position in the keyword table;
 * kwend to the first invalid position; n is the number of characters already
 * processed; and ch is the next character to process.  The function will
 * update *kwpos and *kwend to take into account the new character.  It will
 * set *kwpos to -1 if it determines that the word is not a keyword.
 */
static void check_kw(int *kwpos, int *kwend, int n, char ch)
{
    int i;

    if (*kwpos == -1) return;

    while ((kwtbl[n][*kwpos].w == 0) && (*kwpos < *kwend))
        (*kwpos)++;

    for (i = *kwpos; i < *kwend; i += kwtbl[n][i].w)
        if (ch == kwtbl[n][i].ch) break;

    if (i < *kwend)
    {
        *kwpos = i;
        *kwend = i + kwtbl[n][i].w;
    }

    else *kwpos = -1;
}

/* Utility function that does most of the work of the highlighter.  It scans
 * the text using something resembling a DFA-based lexical scanner, except that
 * keywords are read from the table described above.  The text to scan is
 * passed in txt, and pointers to variables containing a starting position and
 * starting state in idx and state.  On return, idx and state are updated to
 * reflaect the next place to start scanning, and the return value is the
 * palette index to use when drawing the text.
 */
static int get_color(char *txt, int *idx, int *state)
{
    int start_idx;
    int kwpos, kwend;

    kwpos = 0;
    kwend = N_KEYWORDS;

    start_idx = *idx;

    while (1)  /* loop until we return a color */
    {
        switch (*state)
        {
            case STATE_BEGIN:
                if (isspace(txt[*idx]))
                {
                    *state = STATE_WHITESPACE;
                    (*idx)++;
                }

                else if (isalpha(txt[*idx]) || (txt[*idx] == '_'))
                {
                    check_kw(&kwpos, &kwend, 0, txt[*idx]);
                    *state = STATE_WORD;
                    (*idx)++;
                }

                else switch (txt[*idx])
                {
                    case '0':
                        *state = STATE_0;
                        (*idx)++;
                        break;

                    case '1':
                    case '2':
                    case '3':
                    case '4':
                    case '5':
                    case '6':
                    case '7':
                    case '8':
                    case '9':
                        *state = STATE_NUM_WHOLE;
                        (*idx)++;
                        break;

                    case '.':
                        *state = STATE_DOT;
                        (*idx)++;
                        break;

                    case '/':
                        *state = STATE_SLASH;
                        (*idx)++;
                        break;

                    case '\"':
                        *state = STATE_STRING_CONST;
                        (*idx)++;
                        break;

                    case '\'':
                        *state = STATE_CHAR_CONST;
                        (*idx)++;
                        break;

                    case '\0':
                        return COLOR_IDENT;

                    case '(':
                    case '[':
                    case '{':
                    case '}':
                    case ']':
                    case ')':
                        (*idx)++;
                        return COLOR_BRACE;

                    default:
                        (*idx)++;
                        return COLOR_SYMBOL;
                }
                break;

            case STATE_WHITESPACE:
                while (isspace(txt[*idx])) (*idx)++;
                *state = STATE_BEGIN;
                return COLOR_IDENT;

            case STATE_WORD:
                while (isalnum(txt[*idx]) || (txt[*idx] == '_'))
                {
                    check_kw(&kwpos, &kwend, (*idx) - start_idx, txt[*idx]);
                    (*idx)++;
                }

                *state = STATE_BEGIN;

                if ((kwpos != -1) && (kwtbl[*idx - start_idx][kwpos].w == 0))
                    return COLOR_KEYWORD;
                else
                    return COLOR_IDENT;

            case STATE_SLASH:
                switch (txt[*idx])
                {
                    case '*':
                        *state = STATE_COMMENT;
                        (*idx)++;
                        break;

                    case '/':
                        *state = STATE_LINE_COMMENT;
                        (*idx)++;
                        break;

                    default:
                        *state = STATE_BEGIN;
                        return COLOR_SYMBOL;
                }
                break;

            case STATE_COMMENT:
                while ((txt[*idx] != '*') && (txt[*idx] != '\0')) (*idx)++;
                if (txt[*idx] == '\0') return COLOR_COMMENT;
                *state = STATE_COMMENT_STAR;
                (*idx)++;
                break;

            case STATE_COMMENT_STAR:
                switch (txt[*idx])
                {
                    case '/':
                        *state = STATE_COMMENT_DONE;
                        (*idx)++;
                        break;

                    case '*':
                        (*idx)++;
                        break;

                    case '\0':
                        *state = STATE_COMMENT;
                        return COLOR_COMMENT;

                    default:
                        *state = STATE_COMMENT;
                        (*idx)++;
                }
                break;

            case STATE_COMMENT_DONE:
                *state = STATE_BEGIN;
                return COLOR_COMMENT;

            case STATE_LINE_COMMENT:
                while (txt[*idx] != '\0') (*idx)++;
                *state = STATE_BEGIN;
                return COLOR_COMMENT;

            case STATE_STRING_CONST:
                switch (txt[*idx])
                {
                    case '\"':
                        (*idx)++;
                        *state = STATE_BEGIN;
                        return COLOR_STRING;

                    case '\\':
                        *state = STATE_STRING_CONST_ESC;
                        return COLOR_STRING;

                    case '\0':
                        return COLOR_STRING;

                    default:
                        (*idx)++;
                }
                break;

            case STATE_STRING_CONST_ESC:
                (*idx)++;

                switch (txt[*idx])
                {
                    case '\0':
                        *state = STATE_STRING_CONST;
                        return COLOR_STRINGE;

                    case '0':
                    case '1':
                    case '2':
                    case '3':
                        *state = STATE_STRING_CONST_ESCOCT_1;
                        (*idx)++;
                        break;

                    case '4':
                    case '5':
                    case '6':
                    case '7':
                        *state = STATE_STRING_CONST_ESCOCT_2;
                        (*idx)++;
                        break;

                    case 'b':
                    case 't':
                    case 'n':
                    case 'f':
                    case 'r':
                    case '\\':
                    case '\'':
                    case '\"':
                        *state = STATE_STRING_CONST;
                        (*idx)++;
                        return COLOR_STRINGE;

                    default:
                        *state = STATE_STRING_CONST;
                        (*idx)++;
                        return COLOR_ILLEGAL;
                }
                break;

            case STATE_STRING_CONST_ESCOCT_1:
                switch (txt[*idx])
                {
                    case '0':
                    case '1':
                    case '2':
                    case '3':
                    case '4':
                    case '5':
                    case '6':
                    case '7':
                        *state = STATE_STRING_CONST_ESCOCT_2;
                        (*idx)++;
                        break;

                    default:
                        *state = STATE_STRING_CONST;
                        return COLOR_STRINGE;
                }
                break;

            case STATE_STRING_CONST_ESCOCT_2:
                if ((txt[*idx] >= '0') && (txt[*idx] <= '7')) (*idx)++;
                *state = STATE_STRING_CONST;
                return COLOR_STRINGE;

            case STATE_CHAR_CONST:
                switch (txt[*idx])
                {
                    case '\'':
                        (*idx)++;
                        *state = STATE_BEGIN;
                        return COLOR_STRING;

                    case '\\':
                        *state = STATE_CHAR_CONST_ESC;
                        return COLOR_STRING;

                    case '\0':
                        *state = STATE_BEGIN;
                        return COLOR_STRING;

                    default:
                        (*idx)++;
                }
                break;

            case STATE_CHAR_CONST_ESC:
                (*idx)++;

                switch (txt[*idx])
                {
                    case '\0':
                        *state = STATE_CHAR_CONST;
                        return COLOR_STRINGE;

                    case '0':
                    case '1':
                    case '2':
                    case '3':
                        *state = STATE_CHAR_CONST_ESCOCT_1;
                        (*idx)++;
                        break;

                    case '4':
                    case '5':
                    case '6':
                    case '7':
                        *state = STATE_CHAR_CONST_ESCOCT_2;
                        (*idx)++;
                        break;

                    case 'b':
                    case 't':
                    case 'n':
                    case 'f':
                    case 'r':
                    case '\\':
                    case '\'':
                    case '\"':
                        *state = STATE_CHAR_CONST;
                        (*idx)++;
                        return COLOR_STRINGE;

                    default:
                        *state = STATE_CHAR_CONST;
                        (*idx)++;
                        return COLOR_ILLEGAL;
                }
                break;

            case STATE_CHAR_CONST_ESCOCT_1:
                switch (txt[*idx])
                {
                    case '0':
                    case '1':
                    case '2':
                    case '3':
                    case '4':
                    case '5':
                    case '6':
                    case '7':
                        *state = STATE_CHAR_CONST_ESCOCT_2;
                        (*idx)++;
                        break;

                    default:
                        *state = STATE_CHAR_CONST;
                        return COLOR_STRINGE;
                }
                break;

            case STATE_CHAR_CONST_ESCOCT_2:
                if ((txt[*idx] >= '0') && (txt[*idx] <= '7')) (*idx)++;
                *state = STATE_CHAR_CONST;
                return COLOR_STRINGE;

            case STATE_0:
                switch (txt[*idx])
                {
                    case 'x':
                    case 'X':
                        *state = STATE_NUM_HEX;
                        (*idx)++;
                        break;

                    case '0':
                    case '1':
                    case '2':
                    case '3':
                    case '4':
                    case '5':
                    case '6':
                    case '7':
                        *state = STATE_NUM_LEAD0_OCT;
                        (*idx)++;
                        break;

                    case '8':
                    case '9':
                        *state = STATE_NUM_LEAD0;
                        (*idx)++;
                        break;

                    case '.':
                        *state = STATE_NUM_FRACT;
                        (*idx)++;
                        break;

                    case 'L':
                    case 'l':
                        *state = STATE_NUM_SUFF;
                        (*idx)++;
                        break;

                    default:
                        if (isalpha(txt[*idx]) || (txt[*idx] == '_'))
                        {
                            *state = STATE_PANIC;
                            (*idx)++;
                        }
                        else
                        {
                            *state = STATE_BEGIN;
                            return COLOR_NUMBER;
                        }
                }
                break;

            case STATE_NUM_HEX:
                if (isxdigit(txt[*idx]))
                {
                    (*idx)++;
                }
                else switch (txt[*idx])
                {
                    case 'L':
                    case 'l':
                        *state = STATE_NUM_SUFF;
                        (*idx)++;
                        break;

                    case '\0':
                        *state = STATE_BEGIN;
                        return COLOR_NUMBER;

                    default:
                        if (isalpha(txt[*idx]) || (txt[*idx] == '_'))
                        {
                            *state = STATE_PANIC;
                            (*idx)++;
                        }
                        else
                        {
                            *state = STATE_BEGIN;
                            return COLOR_NUMBER;
                        }
                }
                break;

            case STATE_NUM_WHOLE:
                while (isdigit(txt[*idx])) (*idx)++;

                switch (txt[*idx])
                {
                    case '.':
                        *state = STATE_NUM_FRACT;
                        (*idx)++;
                        break;

                    case 'L':
                    case 'l':
                        *state = STATE_NUM_SUFF;
                        (*idx)++;
                        break;

                    case '\0':
                        *state = STATE_BEGIN;
                        return COLOR_NUMBER;

                    default:
                        if (isalpha(txt[*idx]) || (txt[*idx] == '_'))
                        {
                            *state = STATE_PANIC;
                            (*idx)++;
                        }
                        else
                        {
                            *state = STATE_BEGIN;
                            return COLOR_NUMBER;
                        }
                }
                break;

            case STATE_DOT:
                switch (txt[*idx])
                {
                    case '0':
                    case '1':
                    case '2':
                    case '3':
                    case '4':
                    case '5':
                    case '6':
                    case '7':
                    case '8':
                    case '9':
                        *state = STATE_NUM_FRACT;
                        (*idx)++;
                        break;

                    case '\0':
                        *state = STATE_BEGIN;
                        return COLOR_SYMBOL;

                    default:
                        *state = STATE_BEGIN;
                        return COLOR_SYMBOL;
                }
                break;

            case STATE_NUM_LEAD0_OCT:
                switch (txt[*idx])
                {
                    case '0':
                    case '1':
                    case '2':
                    case '3':
                    case '4':
                    case '5':
                    case '6':
                    case '7':
                        (*idx)++;
                        break;

                    case '8':
                    case '9':
                        *state = STATE_NUM_LEAD0;
                        (*idx)++;
                        break;

                    case '.':
                        *state = STATE_NUM_FRACT;
                        (*idx)++;
                        break;

                    case 'L':
                    case 'l':
                        *state = STATE_NUM_SUFF;
                        (*idx)++;
                        break;

                    case '\0':
                        *state = STATE_BEGIN;
                        return COLOR_NUMBER;

                    default:
                        if (isalpha(txt[*idx]) || (txt[*idx] == '_'))
                        {
                            *state = STATE_PANIC;
                            (*idx)++;
                        }
                        else
                        {
                            *state = STATE_BEGIN;
                            return COLOR_NUMBER;
                        }
                }
                break;

            case STATE_NUM_LEAD0:
                switch (txt[*idx])
                {
                    case '0':
                    case '1':
                    case '2':
                    case '3':
                    case '4':
                    case '5':
                    case '6':
                    case '7':
                    case '8':
                    case '9':
                        (*idx)++;
                        break;

                    case '.':
                        *state = STATE_NUM_FRACT;
                        (*idx)++;
                        break;

                    case '\0':
                        *state = STATE_BEGIN;
                        return COLOR_ILLEGAL;

                    default:
                        *state = STATE_PANIC;
                }
                break;

            case STATE_NUM_FRACT:
                while (isdigit(txt[*idx])) (*idx)++;
                switch (txt[*idx])
                {
                    case 'E':
                    case 'e':
                        *state = STATE_FRACT_SUFF_E;
                        (*idx)++;
                        break;

                    case 'F':
                    case 'f':
                    case 'D':
                    case 'd':
                        *state = STATE_FRACT_SUFF;
                        (*idx)++;
                        break;

                    case '\0':
                        *state = STATE_BEGIN;
                        return COLOR_NUMBER;

                    default:
                        if (isalpha(txt[*idx]) || (txt[*idx] == '_') ||
                            (txt[*idx] == '.'))
                        {
                            *state = STATE_PANIC;
                            (*idx)++;
                        }
                        else
                        {
                            *state = STATE_BEGIN;
                            return COLOR_NUMBER;
                        }
                }
                break;

            case STATE_NUM_SUFF:
                switch (txt[*idx])
                {
                    case '\0':
                        *state = STATE_BEGIN;
                        return COLOR_NUMBER;

                    default:
                        if (isalnum(txt[*idx]) || (txt[*idx] == '.') ||
                            (txt[*idx] == '_'))
                        {
                            *state = STATE_PANIC;
                            (*idx)++;
                        }
                        else
                        {
                            *state = STATE_BEGIN;
                            return COLOR_NUMBER;
                        }
                }
                break;

            case STATE_FRACT_SUFF_E:
                if (isdigit(txt[*idx]))
                {
                    *state = STATE_FRACT_E_VAL;
                    (*idx)++;
                }
                else if (isalpha(txt[*idx]) || (txt[*idx] == '.') ||
                         (txt[*idx] == '_'))
                {
                    *state = STATE_PANIC;
                    (*idx)++;
                }
                else switch (txt[*idx])
                {
                    case '+':
                    case '-':
                        *state = STATE_FRACT_E_SIGN;
                        (*idx)++;
                        break;

                    case '\0':
                        *state = STATE_BEGIN;
                        return COLOR_ILLEGAL;

                    default:
                        *state = STATE_BEGIN;
                        return COLOR_ILLEGAL;
                }
                break;

            case STATE_FRACT_E_SIGN:
                if (isdigit(txt[*idx]))
                {
                    *state = STATE_FRACT_E_VAL;
                    (*idx)++;
                }
                else if (isalpha(txt[*idx]) || (txt[*idx] == '.') ||
                         (txt[*idx] == '_'))
                {
                    *state = STATE_PANIC;
                    (*idx)++;
                }
                else switch (txt[*idx])
                {
                    case '\0':
                        *state = STATE_BEGIN;
                        return COLOR_ILLEGAL;

                    default:
                        *state = STATE_BEGIN;
                        return COLOR_ILLEGAL;
                }
                break;

            case STATE_FRACT_E_VAL:
                while (isdigit(txt[*idx])) (*idx)++;
                switch (txt[*idx])
                {
                    case 'F':
                    case 'f':
                    case 'D':
                    case 'd':
                        *state = STATE_FRACT_SUFF;
                        (*idx)++;
                        break;

                    case '\0':
                        *state = STATE_BEGIN;
                        return COLOR_NUMBER;

                    default:
                        if (isalpha(txt[*idx]) || (txt[*idx] == '.') ||
                            (txt[*idx] == '_'))
                        {
                            *state = STATE_PANIC;
                            (*idx)++;
                        }
                        else
                        {
                            *state = STATE_BEGIN;
                            return COLOR_NUMBER;
                        }
                }
                break;

            case STATE_FRACT_SUFF:
                if (isalnum(txt[*idx]) || (txt[*idx] == '.') ||
                    (txt[*idx] == '_'))
                {
                    *state = STATE_PANIC;
                    (*idx)++;
                }
                else
                {
                    *state = STATE_BEGIN;
                    return COLOR_NUMBER;
                }
                break;

            case STATE_PANIC:
                while (isalnum(txt[*idx]) || (txt[*idx] == '_')) (*idx)++;

                *state = STATE_BEGIN;
                return COLOR_ILLEGAL;

            default:
                while (txt[*idx] != '\0') (*idx)++;
                return COLOR_DEBUG;
        }
    }

    return COLOR_IDENT;
}

/* The exported syntax highlighter function.  This function itself really only
 * exists to take care of a few loose ends.  In fact, these loose ends are
 * common to all languages, so this code should probably be moved to screen.c
 * instead of put here.  But that's not possible now because the perl and SGML
 * modules act differently.  As soon as I get all this stuff working the same,
 * I'll move stuff around to avoid duplicating code.
 */
int mode_highlight(buffer *buf, buf_line *ln, int lnum, int *idx, int *state)
{
    if (*state == -1)
    {
        int i, pos;
        char *c;

        while (buf->state_valid_num < lnum)
        {
            c = buf->state_valid->txt;
            pos = 0;
            *state = buf->state_valid->start_state;

            do get_color(c, &pos, state); while (c[pos] != '\0');

            buf->state_valid = buf->state_valid->next;
            buf->state_valid_num++;
            buf->state_valid->start_state = *state;
        }

        c = ln->txt;
        i = 0;
        *state = ln->start_state;
        while (i < *idx) get_color(c, &i, state);
    }

    return get_color(ln->txt, idx, state);
}
