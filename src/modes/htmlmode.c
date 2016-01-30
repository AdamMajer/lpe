/* htmlmode.c
 *
 * Copyright (c) 1999 Chris Smith
 *
 * Author:
 *  Eckehard Berns  <eb@berns.i-s-o.net>
 *
 * This file is distributed under the GPL, version 2 or at your option any
 * later version.  See COPYING for details.
 */

#include <string.h>
#include <ctype.h>

#include "lpecomm.h"
#include "highlight.h"
#include "mode-utils.h"

enum _states
{
    STATE_BEGIN = 0,
    STATE_TAG,
    STATE_ARGUMENT,
    STATE_QUOTE,
    STATE_VALUE,
    STATE_COMMENT,
    STATE_SSI
};

#define COLOR_TEXT          70
#define COLOR_TAG           71
#define COLOR_SPECIAL       72
#define COLOR_ARGUMENT      73
#define COLOR_VALUE         74
#define COLOR_SSI           75

static int is_ssi = 0;

int mode_accept(buffer *buf)
{
    char *suffix;

    suffix = strrchr(buf->name, '.');
    if (suffix == NULL)
	return 0;

    return mode_util_accept_extensions ( suffix, 0, 5, ".html", ".htm", ".shtml", ".dhtml", ".gtml" );
}

void mode_init(buffer *buf)
{
    if (buf->mode_name == NULL)
    {
        buf->hardtab = mode_util_get_option_with_default ( "htmlmode", "hardtab", 1 );
	buf->autoindent = mode_util_get_option_with_default ( "htmlmode", "autoindent", 0 );
	buf->offerhelp = mode_util_get_option_with_default ( "htmlmode", "offerhelp", 1 );
	buf->highlight = mode_util_get_option_with_default ( "htmlmode", "highlight", 1 );
	buf->flashbrace = mode_util_get_option_with_default ( "htmlmode", "flashbrace", 1 );
    }

    buf->mode_name = "htmlmode";

    buf->state_valid = buf->text;
    buf->state_valid_num = 0;
    buf->text->start_state = STATE_BEGIN;
}

void mode_enter(buffer *buf)
{
    mode_util_set_slang_color("htmlmode", "text", COLOR_TEXT, "lightgray", "black" );
    mode_util_set_slang_color("htmlmode", "tag", COLOR_TAG, "cyan", "black" );
    mode_util_set_slang_color("htmlmode", "comment", COLOR_COMMENT, COLOR_COMMENT_FG, COLOR_COMMENT_BG );
    mode_util_set_slang_color("htmlmode", "argument", COLOR_ARGUMENT, "blue", "black" );
    mode_util_set_slang_color("htmlmode", "special", COLOR_SPECIAL, "brown", "black" );
    mode_util_set_slang_color("htmlmode", "value", COLOR_VALUE, "brightblue", "black" );
    mode_util_set_slang_color("htmlmode", "ssi", COLOR_SSI, "magenta", "black" );
    mode_util_set_slang_color("htmlmode", "symbol", COLOR_SYMBOL, COLOR_SYMBOL_FG, COLOR_SYMBOL_BG );
    mode_util_set_slang_color("htmlmode", "brace", COLOR_BRACE, COLOR_BRACE_FG, COLOR_BRACE_BG );
    mode_util_set_slang_color("htmlmode", "illegal", COLOR_ILLEGAL, COLOR_ILLEGAL_FG, COLOR_ILLEGAL_BG );
}

/* SGML implementation of brace flashing
 *
 * Rules:
 * 1. Braces are <>, and they cannot be nested
 * 2. Ignore anything inside quotation marks (double)
 */
int mode_flashbrace(buffer * buf)
{
    int found;
    char ch, quote;

    if (buf->pos.col == 0) return 0;
    ch = buf->pos.line->txt[buf->pos.col - 1];
    if (ch != '>') return 0;

    buf->pos.col--;
    quote = '\0';
    found = 0;

    do
    {
	while (buf->pos.col <= 0)
	{
	    if (buf->pos.line == buf->scrollpos)
		return 0;
	    buf->pos.line = buf->pos.line->prev;
	    buf->linenum--;
	    buf->pos.col = strlen(buf->pos.line->txt);
	}

	buf->pos.col--;
	ch = buf->pos.line->txt[buf->pos.col];

	if (quote != '\0')
	{
	    if (ch == quote)
		quote = '\0';
	    continue;
	}
	switch (ch)
	{
	case '<':
	    found = 1;
	    break;

	case '\"':
	    quote = ch;
	    break;
	}
    }
    while (!found);

    set_scr_col(buf);
    return 1;
}

#define IS_NAME(a)   ((isalnum(a)  || ((a) == '_') || \
                      ((a) == '%') || ((a) == '&') || \
                      ((a) == '.') || ((a) == '#')))

#define IS_WHITE(a)  (isspace(a))

#define STATE        (*state & 0x00ff)
#define EXCL         (*state & 0xff00)

int mode_highlight(buffer * buf, buf_line * ln, int lnum, int *idx, int *state)
{
    int color;
    int i, ch;
    char *p;

    if (*state == -1)
    {
	*state = buf->state_valid->start_state;
	while (buf->state_valid_num < lnum)
	{
	    i = 0;
	    while (buf->state_valid->txt[i])
		mode_highlight(buf, buf->state_valid,
			       buf->state_valid_num,
			       &i, state);
	    buf->state_valid = buf->state_valid->next;
	    buf->state_valid_num++;
	    buf->state_valid->start_state = *state;
	}
	i = 0;
	color = -1;
	*state = ln->start_state;
	while (i < *idx)
	    color = mode_highlight(buf, ln, lnum, &i, state);
	if ((i > *idx) && (color != -1))
	{
	    *idx = i;
	    return color;
	}
    }

    ch = ln->txt[*idx];
    if (ch == 0)
	return COLOR_TEXT;

    if (ch == '>')
    {
	(*idx)++;
	if (STATE == STATE_BEGIN)
	    return COLOR_ILLEGAL;
	*state = STATE_BEGIN | EXCL;
	return COLOR_TAG;
    }
    if ((*state == (STATE_TAG | 0x100)) &&
        (ch == '-') && (ln->txt[*idx + 1] == '-') && (ln->txt[*idx + 2] == '#'))
    {
        *state = STATE_SSI | EXCL;
        (*idx) += 3;
        is_ssi = 1;
    }
    if ((*state == (STATE_TAG | 0x100)) &&
        (ch == '-') && (ln->txt[*idx + 1] == '-'))
    {
        *state = STATE_COMMENT | EXCL;
        (*idx) += 2;
    }
    if ((STATE == STATE_TAG) && (IS_WHITE(ch)))
    {
	do
	{
	    (*idx)++;
	}
	while (IS_WHITE(ln->txt[*idx]));
	return COLOR_TAG;
    }
    if (((STATE == STATE_TAG) || (STATE == STATE_ARGUMENT)) &&
        (strchr("-;|+*?,", ch)))
    {
        (*idx)++;
        *state = STATE_TAG | EXCL;
        return COLOR_SYMBOL;
    }
    if (((STATE == STATE_TAG) || (STATE == STATE_ARGUMENT)) &&
        (strchr("()[]", ch)))
    {
        (*idx)++;
        *state = STATE_TAG | EXCL;
        return COLOR_BRACE;
    }
    if ((STATE == STATE_TAG) && (IS_NAME(ch)))
    {
	do
	{
	    (*idx)++;
	}
	while (IS_NAME(ln->txt[*idx]));
	*state = STATE_ARGUMENT | EXCL;
	return COLOR_ARGUMENT;
    }
    if ((STATE == STATE_TAG) && (ch == '"'))
    {
	(*idx)++;
	*state = STATE_QUOTE | EXCL;
    }
    if (STATE == STATE_TAG)
    {
	(*idx)++;
	return COLOR_ILLEGAL;
    }
    if ((STATE == STATE_ARGUMENT) && (ch == '='))
    {
	(*idx)++;
	if (ln->txt[*idx] == '"')
	{
	    *state = STATE_TAG | EXCL;
	}
	else
	{
	    *state = STATE_VALUE | EXCL;
	}
	return COLOR_TAG;
    }
    if (STATE == STATE_ARGUMENT)
    {
	if (IS_WHITE(ch))
	{
	    (*idx)++;
	    *state = STATE_TAG | EXCL;
	    return COLOR_TAG;
	}
	(*idx)++;
	return COLOR_ILLEGAL;
    }
    if (STATE == STATE_VALUE)
    {
	while ((!IS_WHITE(ln->txt[*idx])) && (ln->txt[*idx]) &&
	       (ln->txt[*idx] != '>'))
	    (*idx)++;
	*state = STATE_TAG | EXCL;
	return COLOR_VALUE;
    }
    if (STATE == STATE_QUOTE)
    {
	while ((ln->txt[*idx]) && (ln->txt[*idx] != '"'))
	    (*idx)++;
	if (ln->txt[*idx] == '"')
	{
	    (*idx)++;
	    *state = STATE_TAG | EXCL;
	}
	return COLOR_VALUE;
    }
    if ((STATE == STATE_COMMENT) || (STATE == STATE_SSI))
    {
	p = strstr(&(ln->txt[*idx]), "--");
	if (p == NULL)
	{
	    *idx = strlen(ln->txt);
	}
	else
	{
	    *state = STATE_TAG | EXCL;
	    *idx = p + 2 - ln->txt;
	}
	if (is_ssi)
	{
	    is_ssi = 0;
	    return COLOR_SSI;
	}
	else
	    return COLOR_COMMENT;
    }
    if (ch == '<')
    {
	*state = STATE_TAG;
	(*idx)++;
        if (ln->txt[*idx] == '!')
        {
            *state = STATE_TAG | 0x100;
            (*idx)++;
        }
	else if (ln->txt[*idx] == '/')
        {
	    (*idx)++;
        }
	ch = ln->txt[*idx];
	while (IS_NAME(ch))
	{
	    (*idx)++;
	    ch = ln->txt[*idx];
	}
	return COLOR_TAG;
    }
    if (ch == '&')
    {
	do
	{
	    (*idx)++;
	    ch = ln->txt[*idx];
	}
	while ((IS_NAME(ch)) || (ch == '#'));
	if (ch == ';')
	{
	    (*idx)++;
	    return COLOR_SPECIAL;
	}
	return COLOR_ILLEGAL;
    }

    *idx += strcspn(&(ln->txt[*idx]), "<&");
    return COLOR_TEXT;
}
