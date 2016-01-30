/* perlmode.c
 *
 * Copyright (c) 1999 Chris Smith
 *
 * Author:
 *  Eckehard Berns  <eb@berns.i-s-o.net>
 *
 * This file is distributed under the GPL, version 2 or at your option any
 * later version.  See COPYING for details.
 */

/* Known bugs:
 *
 *  The syntax highlighting doesn't do "if /foo/" or "if foo =~ /bar/" regular
 *  expression highlighting.  I have no idea how to catch regexps without the
 *  leading m without doing expensive syntax parsing.
 *
 *  This forces me to remove the other regexp highlighting from perlmode also.
 *  Because anything can happen in a "if /foo/" statement.  I'll reenable it
 *  when I found a way to highlight these regexps.
 */

#undef HIGHLIGHT_REGEXPS

#include <string.h>
#include <ctype.h>

#include "lpecomm.h"
#include "highlight.h"
#include "mode-utils.h"

/* If this is true nearly all keywords found in perlfunc(1p) get
 * highlighted. This is kind of overkill.
 */
#define EXTENDED_KEYWORDS 1

/* List of keywords.  The ord value of the first character of the string at
 * index 0 must contain the maximum length of the keywords defined in the
 * list.  All subsequent strings hold the list of keywords with the length
 * indicated by the array index.  So keywords[1] holds all keywords with the
 * length of 1, keywords[2] all with the length of 2, and so on.  NULL pointers
 * aren't allowed and will produce segfaults.
 */
static const char *keywords[] =
{
#if EXTENDED_KEYWORDS
    "\007",	/* maximum length is 7 */
    "",
    "dolcmynoucifneeqlegeltgtor",
    "chrhexoctordposabscosexpintlogoctsinpopmapdieeofvecsubreftie"
    "fornotand",
    "choppackrandsqrtpushgrepjoinsorteachkeysgetcreadseektellwarn"
    "globlinkopenstatdumpevalexitgotolastnextredoexecforkkillpipe"
    "waittiedbindrecvsendtimethenelse",
    "chompcryptindexsplitstudyatan2srandshiftcloseflockprintwrite"
    "chdirchmodchownfcntlioctllstatmkdirrmdirumaskutimelocalreset"
    "undefalarmsleeptimesblassuntiesemopelsifwhile",
    "lengthrindexsubstrspliceunpackdeleteexistsvaluesformatprintf"
    "selectchrootrenameunlinkcallerreturnimportscalarsystemaccept"
    "listensocketmsgctlmsggetmsgrcvmsgsndsemctlsemgetshmctlshmget"
    "gmtime",
    "lcfirstreversesprintfucfirstunshiftbinmodedbmopenreaddirseekdir"
    "syscallsysreadsysseektelldiropendirsymlinkgetpgrpgetppidsetpgrp"
    "connectshmreadforeach",
#else /* !EXTENDED_KEYWORDS */
    "\007",
    "",
    "ifneeqltgtlegedoor",
    "forsubdienotand",
    "nextlastelsewarn",
    "elsifwhile",
    "",
    "foreach",
#endif /* !EXTENDED_KEYWORDS */
};

static const char *preproc[] =
{
    "\007",
    "",
    "",
    "use",
    "",
    "",
    "import",
    "requirepackage",
};

/* I need these to save the character used in qq// style strings between
 * calls to mode_highlight.  The overall state is specified by the high byte.
 * The character needed to terminate this state is saved in the low byte as
 * an option to the state.
 */
#define STATE_MASK          0xff00
#define STATE_OPT_MASK      0x00ff

#define STATE_BEGIN         0x0000
#define STATE_IN_SQUOTE     0x0100
#define STATE_IN_DQUOTE     0x0200
#define STATE_IN_SHELL      0x0300
#define STATE_IN_REGEXP     0x0400
#define STATE_LAST_REGEXP   0x0500

#define COLOR_VARIABLE      70
#define COLOR_SHELL         71
#ifdef HIGHLIGHT_REGEXPS
#define COLOR_REGEXP        72
#endif /* defined(HIGHLIGHT_REGEXPS) */

int mode_accept(buffer *buf)
{
    char *suffix;

    suffix = strrchr(buf->name, '.');
    if ( ( suffix != NULL && mode_util_accept_extensions ( suffix, 0, 2, ".pl", ".pm" ) ) ||
    	 mode_util_accept_on_request ( buf->text->txt, 0, 1, "perl" ) )
	return 1;

    /* Check the first line of the file for the string "perl".
     * This isn't perfect, but should be a good guess. */

    if (buf->text->txt[0] != '#') return 0;
    if (strstr(buf->text->txt, "perl") != NULL) return 1;
    return 0;
}

void mode_init(buffer *buf)
{
    if (buf->mode_name == NULL)
    {
        buf->hardtab = mode_util_get_option_with_default ( "perlmode", "hardtab", 1 );
	buf->autoindent = mode_util_get_option_with_default ( "perlmode", "autoindent", 1 );
	buf->offerhelp = mode_util_get_option_with_default ( "perlmode", "offerhelp", 1 );
	buf->highlight = mode_util_get_option_with_default ( "perlmode", "highlight", 1 );
	buf->flashbrace = mode_util_get_option_with_default ( "perlmode", "flashbrace", 1 );
    }

    buf->mode_name = "perlmode";

    buf->state_valid = buf->text;
    buf->state_valid_num = 0;
    buf->text->start_state = STATE_BEGIN;
}

void mode_enter(buffer *buf)
{
    mode_util_set_slang_color ( "cmode", "ident", COLOR_IDENT, COLOR_IDENT_FG, COLOR_IDENT_BG );
    mode_util_set_slang_color ( "cmode", "symbol", COLOR_SYMBOL, COLOR_SYMBOL_FG, COLOR_SYMBOL_BG );
    mode_util_set_slang_color ( "cmode", "brace", COLOR_BRACE, COLOR_BRACE_FG, COLOR_BRACE_BG );
    mode_util_set_slang_color ( "cmode", "comment", COLOR_COMMENT, COLOR_COMMENT_FG, COLOR_COMMENT_BG );
    mode_util_set_slang_color ( "cmode", "keyword", COLOR_KEYWORD, COLOR_KEYWORD_FG, COLOR_KEYWORD_BG );
    mode_util_set_slang_color ( "cmode", "preproc", COLOR_PREPROC, COLOR_PREPROC_FG, COLOR_PREPROC_BG );
    mode_util_set_slang_color ( "cmode", "string", COLOR_STRING, COLOR_STRING_FG, COLOR_STRING_BG );
    mode_util_set_slang_color ( "cmode", "number", COLOR_NUMBER, COLOR_NUMBER_FG, COLOR_NUMBER_BG );
    mode_util_set_slang_color ( "cmode", "stringe", COLOR_STRINGE, COLOR_STRINGE_FG, COLOR_STRINGE_BG );
    mode_util_set_slang_color ( "cmode", "illegal", COLOR_ILLEGAL, COLOR_ILLEGAL_FG, COLOR_ILLEGAL_BG );
    mode_util_set_slang_color ( "cmode", "debug", COLOR_DEBUG, COLOR_DEBUG_FG, COLOR_DEBUG_BG );
    mode_util_set_slang_color ( "perlmode", "variable", COLOR_VARIABLE, "brightcyan", "black" );
    mode_util_set_slang_color ( "perlmode", "shell", COLOR_SHELL, "magneta", "black" );
#ifdef HIGHLIGHT_REGEXPS
    mode_util_set_slang_color ( "perlmode", "regexp", COLOR_REGEXP, "magneta", "black" );
#endif /* HIGHLIGHT_REGEXPS */
}

/* Perl implementation of brace flashing
 *
 * Rules:
 * 1. Braces are (), [], and {}, and they must be properly nested
 * 2. Ignore anything inside quotation marks (single or double)
 * 3. Disregard rule 2 if the quote is escaped
 * 4. Ignore anything after a hash (perl comment)
 * 5. Print a silent warning in the minibuf for known mismatched braces
 */
int mode_flashbrace(buffer * buf)
{
    char brace_stack[1024];
    int sp;
    char ch, quote;

    if (buf->pos.col == 0)
	return 0;
    ch = buf->pos.line->txt[buf->pos.col - 1];
    if ((ch != ')') && (ch != ']') && (ch != '}'))
	return 0;
    if ((strchr(buf->pos.line->txt, '#')) &&
	(buf->pos.col > strchr(buf->pos.line->txt, '#') - buf->pos.line->txt))
	return 0;		/* we're in a comment */

    brace_stack[0] = ch;
    sp = 1;
    buf->pos.col--;

    quote = '\0';

    do
    {
	char last_ch;

	while (buf->pos.col <= 0)
	{
	    if (buf->pos.line == buf->scrollpos) return 0;
	    buf->pos.line = buf->pos.line->prev;
	    buf->linenum--;
	    buf->pos.col = strlen(buf->pos.line->txt);
	    if (strchr(buf->pos.line->txt, '#'))
	    {
		buf->pos.col = strchr(buf->pos.line->txt,
				      '#') -
		    buf->pos.line->txt;
	    }
	}

	buf->pos.col--;
	last_ch = ch;
	ch = buf->pos.line->txt[buf->pos.col];

	if (quote != '\0')
	{
	    if (ch == quote)
		quote = '\0';
	    else if ((last_ch == quote) && (ch == '\\'))
		quote = 0;

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
	    if ((last_ch == '\'') || (last_ch == '\"'))
		quote = last_ch;
	    break;
	}
    }
    while (sp != 0);

    set_scr_col(buf);
    return 1;
}

/* These two macros check if a character can be part of a valid variable or
 * function name
 */
#define IS_VAR(a) (isalnum(a) || ((a) == '_'))
#define IS_VAR1(a) (isalpha(a) || ((a) == '#'))

/* This function checks if the word found at *idx within the line ln is in
 * the list of *words[].  If the word isn't found in the list,
 * 0 is returned.  Otherwise the length of the found word is returned.
 * maxlen is used as input and output.  If the contents of *maxlen is empty,
 * the length will be computed and stored in that variable.  This should
 * prevent duplicate length calculation.
 */
static int check_words(buf_line * ln, int *idx, const char *words[],
		       int *maxlen)
{
    int i, len;
    const char *c, *p, *w;

    if (!IS_VAR1(ln->txt[*idx]))
	return 0;	/* paranoia */

    if ((maxlen) && (*maxlen))
	len = *maxlen;
    else
	len = 0;
    if (len == 0)
    {
	for (len = 1; IS_VAR(ln->txt[*idx + len]); len++) ;
	if (maxlen)
	    *maxlen = len;
    }
    if (len > words[0][0])
	return 0;

    for (w = words[len]; *w; w += len)
    {
	p = &(ln->txt[*idx]);
	for (i = 0, c = w; i < len; i++, c++, p++)
	    if (*c != *p)
		break;
	if (i == len)
	    return len;
    }
    return 0;
}

/* The exported highlighting function.
 */
int mode_highlight(buffer * buf, buf_line * ln, int lnum, int *idx, int *state)
{
    int color, ch, i;
    int tmp;

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
    if (ln->txt[*idx] == 0)
	return COLOR_IDENT;

    /* single quotes */
    if ((*state & STATE_MASK) == STATE_IN_SQUOTE)
    {
	while (ln->txt[*idx])
	{
	    if ((((*idx) && (ln->txt[*idx - 1] != '\\')) ||
		 (*idx == 0)) &&
		(ln->txt[*idx] == (*state & STATE_OPT_MASK)))
	    {
		*state = STATE_BEGIN;
		(*idx)++;
		return COLOR_STRING;
	    }
	    (*idx)++;
	}
	return COLOR_STRING;
    }
    /* variables */
    if (strchr("$%@&", ln->txt[*idx]))
    {
	(*idx)++;
	if (ln->txt[*idx] == '{')
	{
	    for ((*idx)++;
		 (ln->txt[*idx]) && (ln->txt[*idx] != '}');
		 (*idx)++) ;
	    if (ln->txt[*idx])
	    {
		(*idx)++;
		return COLOR_VARIABLE;
	    }
	    return COLOR_ILLEGAL;
	}
	if (ln->txt[*idx] == '#')
	    (*idx)++;
	for (; (ln->txt[*idx]) && (IS_VAR(ln->txt[*idx])); (*idx)++) ;
	return COLOR_VARIABLE;
    }
    /* escaped characters */
    if (ln->txt[*idx] == '\\')
    {
	(*idx)++;
	if (ln->txt[*idx] == 0)
	{
	    return COLOR_SYMBOL;
	}
	if (ln->txt[*idx] == 'x')
	{
	    for ((*idx)++, tmp = 0;
		 (ln->txt[*idx]) && (tmp < 2) &&
		 (strchr("0123456789abcdefABCDEF", ln->txt[*idx]));
		 (*idx)++, tmp++) ;
	    if (tmp == 0)
		return COLOR_ILLEGAL;
	    return COLOR_STRINGE;
	}
	if (strchr("01234567", ln->txt[*idx]))
	{
	    for ((*idx)++, tmp = 0;
		 (ln->txt[*idx]) && (strchr("01234567", ln->txt[*idx]));
		 tmp++, (*idx)++) ;
	    if (tmp > 2)
		return COLOR_ILLEGAL;
	    return COLOR_STRINGE;
	}
	(*idx)++;
	return COLOR_STRINGE;
    }
#ifdef HIGHLIGHT_REGEXPS
    /* regexps (first part of ///) */
    if ((*state & STATE_MASK) == STATE_IN_REGEXP)
    {
	while ((ch = ln->txt[*idx]) != 0)
	{
	    if ((((*idx) && (ln->txt[*idx - 1] != '\\')) ||
		 (*idx == 0)) && (ch == (*state & STATE_OPT_MASK)))
	    {
		*state = (*state & STATE_OPT_MASK) |
		    STATE_LAST_REGEXP;
		(*idx)++;
		break;
	    }
	    (*idx)++;
	}
	if ((*state & STATE_MASK) != STATE_LAST_REGEXP)
	    return COLOR_REGEXP;
    }
    /* regexps (second part of /// or just //) */
    if ((*state & STATE_MASK) == STATE_LAST_REGEXP)
    {
	while ((ch = ln->txt[*idx]) != 0)
	{
	    if ((((*idx) && (ln->txt[*idx - 1] != '\\')) ||
		 (*idx == 0)) && (ch == (*state & STATE_OPT_MASK)))
	    {
		*state = STATE_BEGIN;
		(*idx)++;
		return COLOR_REGEXP;
	    }
	    (*idx)++;
	}
	return COLOR_REGEXP;
    }
#endif /* defined(HIGHLIGHT_REGEXPS) */
    /* double quotes */
    if ((*state & STATE_MASK) == STATE_IN_DQUOTE)
    {
	while ((ch = ln->txt[*idx]) != 0)
	{
	    if ((((*idx) && (ln->txt[*idx - 1] != '\\')) ||
		 (*idx == 0)) && (ch == (*state & STATE_OPT_MASK)))
	    {
		*state = STATE_BEGIN;
		(*idx)++;
		return COLOR_STRING;
	    }
	    if ((ch == '\\') || (ch == '$'))
		return COLOR_STRING;
	    (*idx)++;
	}
	return COLOR_STRING;
    }
    /* `cmd` style shell commands */
    if (*state == STATE_IN_SHELL)
    {
	while ((ch = ln->txt[*idx]) != 0)
	{
	    if ((((*idx) && (ln->txt[*idx - 1] != '\\')) ||
		 (*idx == 0)) && (ch == '`'))
	    {
		*state = STATE_BEGIN;
		(*idx)++;
		return COLOR_SHELL;
	    }
	    if ((ch == '\\') || (ch == '$'))
		return COLOR_SHELL;
	    (*idx)++;
	}
	return COLOR_SHELL;
    }
    /* comments */
    if (ln->txt[*idx] == '#')
    {
	*idx = strlen(ln->txt);
	return COLOR_COMMENT;
    }
#ifdef HIGHLIGHT_REGEXPS
    /* start of regexps: m//, s///, tr///, etc */
    if ((ln->txt[*idx + 1]) && (!IS_VAR(ln->txt[*idx + 1])) &&
	(!strchr("([{", ln->txt[*idx + 1])) &&
	(((*idx > 0) && (ln->txt[*idx - 1] != '/')) || (*idx == 0)))
    {
	if (ln->txt[*idx] == 'm')
	    *state = STATE_LAST_REGEXP |
		(ln->txt[*idx + 1] & STATE_OPT_MASK);
	else if (ln->txt[*idx] == 's')
	    *state = STATE_IN_REGEXP |
		(ln->txt[*idx + 1] & STATE_OPT_MASK);
	if (*state != STATE_BEGIN)
	{
	    (*idx) += 2;
	    return COLOR_REGEXP;
	}
    }
    if ((IS_VAR(ln->txt[*idx + 1])) &&
	(!strchr("([{", ln->txt[*idx + 2])) &&
	(ln->txt[*idx + 2]) && (!IS_VAR(ln->txt[*idx + 2])))
    {
	if ((ln->txt[*idx] == 't') && (ln->txt[*idx + 1] == 'r'))
	    *state = STATE_IN_REGEXP |
		(ln->txt[*idx + 2] & STATE_OPT_MASK);
	if (*state != STATE_BEGIN)
	{
	    (*idx) += 3;
	    return COLOR_REGEXP;
	}
    }
#endif /* defined(HIGHLIGHT_REGEXPS) */
    /* start of qq// and q// */
    if (ln->txt[*idx] == 'q')
    {
	if ((ln->txt[*idx + 1]) && (!IS_VAR(ln->txt[*idx + 1])))
	{
	    if (ln->txt[*idx + 1] == '(')
		*state = STATE_IN_SQUOTE | ')';
	    else if (ln->txt[*idx + 1] == '[')
		*state = STATE_IN_SQUOTE | ']';
	    else if (ln->txt[*idx + 1] == '{')
		*state = STATE_IN_SQUOTE | '}';
	    else
		*state = STATE_IN_SQUOTE |
		    (ln->txt[*idx + 1] & STATE_OPT_MASK);
	    (*idx) += 2;
	    return COLOR_STRING;
	}
	if ((ln->txt[*idx + 1]) && (ln->txt[*idx + 2]) &&
	    (ln->txt[*idx + 1] == 'q') && (!IS_VAR(ln->txt[*idx + 2])))
	{
	    if (ln->txt[*idx + 2] == '(')
		*state = STATE_IN_DQUOTE | ')';
	    else if (ln->txt[*idx + 2] == '[')
		*state = STATE_IN_DQUOTE | ']';
	    else if (ln->txt[*idx + 2] == '{')
		*state = STATE_IN_DQUOTE | '}';
	    else
		*state = STATE_IN_DQUOTE |
		    (ln->txt[*idx + 2] & STATE_OPT_MASK);
	    (*idx) += 3;
	    return COLOR_STRING;
	}
    }
    /* keywords etc. */
    if (IS_VAR1(ln->txt[*idx]))
    {
	i = 0;
	if (check_words(ln, idx, keywords, &i))
	{
	    (*idx) += i;
	    return COLOR_KEYWORD;
	}
	if (check_words(ln, idx, preproc, &i))
	{
	    (*idx) += i;
	    return COLOR_PREPROC;
	}
	(*idx) += i;
	return COLOR_IDENT;
    }
    /* the rest */
    if (ln->txt[*idx] == '\'')
    {
	*state = STATE_IN_SQUOTE | '\'';
	color = COLOR_STRING;
    }
    else if (ln->txt[*idx] == '"')
    {
	*state = STATE_IN_DQUOTE | '"';
	color = COLOR_STRING;
    }
    else if (ln->txt[*idx] == '`')
    {
	*state = STATE_IN_SHELL;
	color = COLOR_SHELL;
    }
    else if (strchr("-+,.?=~!&/;*<>|", ln->txt[*idx]))
	color = COLOR_SYMBOL;
    else if ((ln->txt[*idx] >= '0') && (ln->txt[*idx] <= '9'))
	color = COLOR_NUMBER;
    else if (strchr(" \n\v\t\r", ln->txt[*idx]))
	color = COLOR_IDENT;
    else if (strchr("()[]{}", ln->txt[*idx]))
	color = COLOR_BRACE;
    else
	color = COLOR_IDENT;
    (*idx)++;
    return color;
}
