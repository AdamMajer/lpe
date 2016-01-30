/* lispmode.c
 *
 * Copyright (c) Chris Smith
 *
 * Author:
 *  Gergely Nagy <algernon@debian.org>
 *
 * This file is distributed under the GPL, version 2 or at your option any
 * later version.  See COPYING for details.
 */

#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "lpecomm.h"
#include "highlight.h"
#include "mode-utils.h"

enum _states {
	STATE_BEGIN = 0,
	STATE_TAG,
	STATE_QUOTE,
	STATE_COMMENT,
	STATE_SYMBOL,
	STATE_COMMAND_BEGIN,
	STATE_COMMAND_END,
	STATE_COMMAND
};

#define COLOR_TEXT		70
#define COLOR_COMMAND		71

int
mode_accept(buffer *buf)
{
	char *suffix;

	suffix = strrchr(buf->name, '.');

	/*
	 * FIXME: This list is probably incomplete. The only lisp I
	 * know is guile and rep (.scm,.jl,.rep), others are probably
	 * missing.
	 */
	if (suffix != NULL && 
	    mode_util_accept_extensions ( suffix, 0, 4, ".jl", ".rep", ".scm", ".el" ))
		return 1;
	/*
	 * FIXME: other 'modes' should be added if one findes them
	 */
	return mode_util_accept_on_request ( buf->text->txt, 0, 2, "lisp", "scheme", "emacs-lisp" );
}

void
mode_init(buffer *buf)
{
	if (buf->mode_name == NULL) {
		buf->hardtab = mode_util_get_option_with_default ( "lispmode", "hardtab", 0 );
		buf->autoindent = mode_util_get_option_with_default ( "lispmode", "autoindent", 1 );
		buf->offerhelp = mode_util_get_option_with_default ( "lispmode", "offerhelp", 1 );
		buf->highlight = mode_util_get_option_with_default ( "lispmode", "highlight", 1 );
		buf->flashbrace = mode_util_get_option_with_default ( "lispmode", "flashbrace", 1 );
	}

	buf->mode_name = "lispmode";
	buf->state_valid = buf->text;
	buf->state_valid_num = 0;
	buf->text->start_state = STATE_BEGIN;
}

void
mode_enter(buffer *buf)
{
	mode_util_set_slang_color ( "lispmode", "text", COLOR_TEXT, "lightgray", "black" );
	mode_util_set_slang_color ( "lispmode", "comment", COLOR_COMMENT, COLOR_COMMENT_FG, COLOR_COMMENT_BG );
	mode_util_set_slang_color ( "lispmode", "symbol", COLOR_SYMBOL, COLOR_SYMBOL_FG, COLOR_SYMBOL_BG );
	mode_util_set_slang_color ( "lispmode", "brace", COLOR_BRACE, COLOR_BRACE_FG, COLOR_BRACE_BG );
	mode_util_set_slang_color ( "lispmode", "command", COLOR_COMMAND, "cyan", "black" );
	mode_util_set_slang_color ( "lispmode", "string", COLOR_STRING, COLOR_STRING_FG, COLOR_STRING_BG );
}

/* LISP implementation of brace flashing
 *
 * Rules:
 * 1. Braces are () and they must be properly nested
 * 2. Ignore anything inside quotation marks (double only)
 * 3. Disregard rule 2 if the quote is escaped
 * 4. Ignore anything after a ';' (lisp comment)
 * 5. Print a silent warning in the minibuf for known mismatched braces
 */
/*
 * FIXME: brace mismatch detection does not work. don't know why...
 */
int
mode_flashbrace(buffer *buf)
{
	char *brace_stack;
	int sp;
	char ch, quote;

	if (buf->pos.col == 0)
		return 0;
	ch = buf->pos.line->txt[buf->pos.col - 1];
	if (ch != ')')
		return 0;

	if ( (strchr(buf->pos.line->txt, ';')) &&
	     (buf->pos.col > strchr(buf->pos.line->txt, ';') - buf->pos.line->txt) )
		return 0;

	brace_stack = (char *)malloc ( 1024 );

	brace_stack[0] = ch;
	sp = 1;
	buf->pos.col--;

	quote = '\0';

	do
	{
		char last_ch;

		while (buf->pos.col <= 0) {

		    if (buf->pos.line == buf->scrollpos)
		    {
		        free ( brace_stack );
			return 0;
		    }

		    buf->pos.line = buf->pos.line->prev;
		    buf->linenum--;
		    buf->pos.col = strlen(buf->pos.line->txt);

		    if ( strchr(buf->pos.line->txt, ';') )
			buf->pos.col = strchr(buf->pos.line->txt, ';') - buf->pos.line->txt;
		}

		buf->pos.col--;
		last_ch = ch;
		ch = buf->pos.line->txt[buf->pos.col];

		if (quote != '\0') {
		    if (ch == quote)
			quote = '\0';
		    else if ((last_ch == quote) && (ch == '\\'))
			quote = 0;

		    continue;
		}

		switch (ch) {
			case ')':
			    if (sp == sizeof ( brace_stack ) )
				brace_stack = (char *) realloc ( brace_stack, sizeof ( brace_stack ) + 1024 );
			    brace_stack[sp++] = ch;
			    break;

			case '(':
			    if (brace_stack[--sp] != ')')
			    {
			        free ( brace_stack );
				return -1;
			    }
			    break;

			case '\"':
			    quote = ch;
			    break;

			case '\\':
			    if ((last_ch == '\'') || (last_ch == '\"'))
				quote = last_ch;
			    break;
		}
	} while (sp != 0);

	free ( brace_stack );

	set_scr_col(buf);
	return 1;
}

#define STATE        (*state & 0x00ff)
#define EXCL         (*state & 0xff00)

int
mode_highlight(buffer *buf, buf_line *ln, int lnum, int *idx, int *state)
{
	int color;
	int i, ch;

	if (*state == -1) {
		*state = buf->state_valid->start_state;
		while (buf->state_valid_num < lnum) {
			i = 0;
			while (buf->state_valid->txt[i])
				mode_highlight(buf, buf->state_valid, 
					       buf->state_valid_num, &i, state);

			buf->state_valid = buf->state_valid->next;
			buf->state_valid_num++;
			buf->state_valid->start_state = *state;
		}

		i = 0;
		color = -1;
		*state = ln->start_state;
		while (i < *idx)
			color = mode_highlight(buf, ln, lnum, &i, state);

		if ((i > *idx) && (color != -1)) {
			*idx = i;
			return color;
		}
	}

	ch = ln->txt[*idx];
	if (ch == 0)
		return COLOR_TEXT;

	if (STATE == STATE_SYMBOL) {
		if (isalnum(ch) || strchr("_-", ch)) {
			(*idx)++;
			return COLOR_SYMBOL;
		} else
			*state = STATE_TAG | EXCL;
	}

	if (STATE == STATE_COMMAND_BEGIN || STATE == STATE_COMMAND || 
	    STATE == STATE_COMMAND_END ) {
		if (isalnum(ch) || strchr("_-?!*", ch)) {
			(*idx)++;
			*state = STATE_COMMAND_END | EXCL;
			return COLOR_COMMAND;
		} else {
			if ( !isspace(ch) || STATE == STATE_COMMAND_END )
				*state = STATE_TAG | EXCL;
			else
				*state = STATE_COMMAND | EXCL;
		}
	}

	if (ln->txt[*idx] == ';') {
		*idx = strlen(ln->txt);
		return COLOR_COMMENT;
	}

	if (strchr("'", ch)) {
		(*idx)++;
		*state = STATE_SYMBOL | EXCL;
		return COLOR_SYMBOL;
	}

	if (strchr("()", ch)) {
		(*idx)++;
		if ( ch == '(' )
			*state = STATE_COMMAND_BEGIN | EXCL;
		else
			*state = STATE_TAG | EXCL;
		return COLOR_BRACE;
	}

	if (ch == '"') {
		(*idx)++;
		*state = STATE_QUOTE | EXCL;
	}

	if (STATE == STATE_QUOTE) {
		while ((ln->txt[*idx]) && (ln->txt[*idx] != '"'))
			(*idx)++;

		if (ln->txt[*idx] == '"') {
			(*idx)++;
			*state = STATE_TAG | EXCL;
		}
		return COLOR_STRING;
	}

	(*idx)++;
	return COLOR_TEXT;
}
