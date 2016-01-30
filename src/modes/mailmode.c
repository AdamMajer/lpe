/* mailmode.c
 *
 * Copyright (c) 1999 Chris Smith
 *
 * Author:
 *  Eckehard Berns  <eb@berns.i-s-o.net>
 * Mod by:
 *  Michal Safranek <wayne@linuxfreak.com>
 *   Brief description of my patch:
 *   - accepts any files starting with 'From ' text
 *   - it colorizes any header (detected by occurence 'From ') in the file 
 *     (needs to be rewritten - really ugly)
 *   - any changes are commented by p-w string
 * Mod #2 by:
 *  Gergely Nagy <algernon@debian.org>
 *   - turned flashbrace off
 *   - tried to fix header highlighting. It works for me :)
 *
 * This file is distributed under the GPL, version 2 or at your option any
 * later version.  See COPYING for details.
 */

#include <stdlib.h>
#include <string.h>

#include "lpecomm.h"
#include "highlight.h"
#include "mode-utils.h"

enum _state {
	STATE_BEGIN = 0,
	STATE_HEADER,
	STATE_BODY,
	STATE_SIG
};

#define COLOR_TEXT          0
#define COLOR_TEXT_FG       "lightgray"
#define COLOR_TEXT_BG       "black"

#define COLOR_HEADER        1
#define COLOR_HEADER_FG     "green"
#define COLOR_HEADER_BG     "black"

#define COLOR_QUOTE         2
#define COLOR_QUOTE_FG      "cyan"
#define COLOR_QUOTE_BG      "black"

#define COLOR_QUOTE2        3
#define COLOR_QUOTE2_FG     "brightblue"
#define COLOR_QUOTE2_BG     "black"

#define COLOR_SIG           4
#define COLOR_SIG_FG        "red"
#define COLOR_SIG_BG        "black"

int mode_accept(buffer *buf)
{
    if (strncmp(buf->name, "mutt-", 5) == 0)
        return 1;
    if ((strncmp(buf->name, "pico.", 5) == 0) && (atoi(buf->name + 5)))
        return 1;
    if ((strncmp(buf->text->txt, "From ", 5) == 0))
    {
        return 1;
    }
    return 0;
}

void mode_init(buffer *buf)
{
    if (buf->mode_name == NULL)
    {
        buf->hardtab = mode_util_get_option_with_default ( "mailmode", "hardtab", 1 );
	buf->autoindent = mode_util_get_option_with_default ( "mailmode", "autoindent", 0 );
	buf->offerhelp = mode_util_get_option_with_default ( "mailmode", "offerhelp", 1 );
	buf->highlight = mode_util_get_option_with_default ( "mailmode", "highlight", 1 );
	buf->flashbrace = mode_util_get_option_with_default ( "mailmode", "flashbrace", 0 );
    }

    buf->mode_name = "mailmode";

    buf->state_valid = buf->text;
    buf->state_valid_num = 0;
    buf->text->start_state = 0;
}

void mode_enter(buffer *buf)
{
    SLtt_set_color(COLOR_TEXT, NULL, COLOR_TEXT_FG, COLOR_TEXT_BG);
    SLtt_set_color(COLOR_HEADER, NULL, COLOR_HEADER_FG, COLOR_HEADER_BG);
    SLtt_set_color(COLOR_QUOTE, NULL, COLOR_QUOTE_FG, COLOR_QUOTE_BG);
    SLtt_set_color(COLOR_QUOTE2, NULL, COLOR_QUOTE2_FG, COLOR_QUOTE2_BG);
    SLtt_set_color(COLOR_SIG, NULL, COLOR_SIG_FG, COLOR_SIG_BG);
}

int mode_highlight(buffer * buf, buf_line * ln, int lnum, int *idx, int *state)
{
	int quote, i;
	char *p;

	if (*state == -1) {
		*state = buf->state_valid->start_state;
		while (buf->state_valid_num < lnum) {
			i = 0;
			mode_highlight(buf, buf->state_valid, buf->state_valid_num,
				       &i, state);
			buf->state_valid = buf->state_valid->next;
			buf->state_valid_num++;
			buf->state_valid->start_state = *state;
		}
		*state = ln->start_state;
	}

	if (*state == STATE_BEGIN)
		*state = STATE_HEADER;
	if(strncmp(ln->txt, "From ", 5)==0) *state=STATE_HEADER;/* p-w */
	if (ln->txt[*idx] == 0 /**/&& *state!=STATE_SIG/**/) {/* p-w */
		*state = STATE_BODY;
		return COLOR_TEXT;
	}
	if (*idx > 0) {
		*idx = strlen(ln->txt);
		return COLOR_TEXT;
	}
	*idx = strlen(ln->txt);

	if (*state == STATE_SIG)
		return COLOR_SIG;
	if (/*(lnum == 0) && */(0 == strncmp("From ", ln->txt, 5))){/* p-w */
		*state=STATE_HEADER;/* p-w */
		return COLOR_HEADER;
	}

	/*
	 * This did not work for me
	 *  - Gergely Nagy
	 */
	/*
	if (*state == STATE_HEADER) {
		if (0 != (i = strcspn(ln->txt, ": "))) {
			if (ln->txt[i] == ':')
				return COLOR_HEADER;
		}
		if ( 0 != strncmp("\n", ln->txt, 1))
			*state = STATE_BODY;
	}
	*/
	if ( *state == STATE_HEADER ) {
		return COLOR_HEADER;
	}

	if (0 == strncmp("--", ln->txt, 2)) {
		p = ln->txt + 2;
		while ((*p == ' ') || (*p == '\t'))
			p++;
		if (*p == 0) {
			*state = STATE_SIG;
			return COLOR_SIG;
		}
	}

	quote = 0;
	p = ln->txt;
	if (*p != ' ') {
		while ((strchr(" >:|", *p)) && (*p)) {
			if (*p != ' ')
				quote++;
			p++;
		}
		if (quote)
			return (quote % 2)?COLOR_QUOTE:COLOR_QUOTE2;
	}

	return COLOR_TEXT;
}
