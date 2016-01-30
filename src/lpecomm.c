/* lpecomm.c
 *
 * Copyright (c) 1999 Chris Smith
 *
 * This file is distributed under the GPL, version 2 or at your option any
 * later version.  See COPYING for details.
 */

#include "options.h"
#include SLANG_H
#include <stdlib.h>
#include "lpecomm.h"
#include "cfg.h"

/* Calculate the next tab stop position.  Returns the number of spaces needed
 * to reach that position.  This is separated into a separate function just to
 * make code look nicer.
 */
int
next_mult (int now, int spacing)
{
    return spacing - (now - ((now / spacing) * spacing));
}

/* Make sure that the current cursor position is visible, scrolling the screen
 * as necessary is both the horizontal and vertical directions.  Returns
 * non-zero if scrolling positions were updated (and hence the screen needs to
 * be redrawn) or zero if the current scrolling was okay.
 */
int
check_scrolling (buffer * buf)
{
    int ret = 0;

    while (buf->linenum < buf->scrollnum)
    {
	int i;
	for (i = 0;
	     i <= cfg_get_global_int_with_default ("scroll_granularity",
						   DEF_SCROLL_GRAN); i++)
	{
	    if (buf->scrollpos->prev != NULL)
	    {
		buf->scrollpos = buf->scrollpos->prev;
		buf->scrollnum--;
	    }
	}

	ret = 1;
    }
    while (buf->linenum >= buf->scrollnum + SLtt_Screen_Rows - 1)
    {
	int i;
	for (i = 0;
	     i <= cfg_get_global_int_with_default ("scroll_granularity",
						   DEF_SCROLL_GRAN); i++)
	{
	    if (buf->scrollpos->next != NULL)
	    {
		buf->scrollpos = buf->scrollpos->next;
		buf->scrollnum++;
	    }
	}

	ret = 1;
    }

    if (buf->scr_col < buf->scrollcol)
    {
	buf->scrollcol =
	    buf->scr_col -
	    cfg_get_global_int_with_default ("scroll_granularity",
					     DEF_SCROLL_GRAN);
	if (buf->scrollcol < 0)
	    buf->scrollcol = 0;
	ret = 1;
    }
    if (buf->scr_col >= buf->scrollcol + SLtt_Screen_Cols)
    {
	buf->scrollcol =
	    buf->scr_col - SLtt_Screen_Cols + 1 +
	    cfg_get_global_int_with_default ("scroll_granularity",
					     DEF_SCROLL_GRAN);
	if (buf->scrollcol >= LINELEN (buf->pos))
	    buf->scrollcol = LINELEN (buf->pos) - 1;
	ret = 1;
    }

    return ret;
}

/* Adjust the column after switching to a new line.  Tries to move the cursor
 * to the preferred column.  If that fails, moves it to the last column in the
 * current line.  This provides the sticky column behavior that makes editors
 * a lot nicer to use.
 */
void
check_col (buffer * buf)
{
    int txt_i = 0, scr_i = 0;

    while ((scr_i < buf->preferred_col)
	   && (buf->pos.line->txt[txt_i] != '\0'))
    {
	if (buf->pos.line->txt[txt_i] == '\t')
	    scr_i += next_mult (scr_i,
				cfg_get_global_int_with_default
				("hard_tab_width", DEF_HARD_TAB_WIDTH));
//                      DEF_HARD_TAB_WIDTH );
	else if (is_control (buf->pos.line->txt[txt_i]))
	    scr_i += 2;
	else
	    scr_i++;

	txt_i++;
    }

    buf->pos.col = txt_i;
    buf->scr_col = scr_i;
}

/* Recalculate the screen column from the current cursor position.  Needed
 * because of the possibility of multi-column characters in the text.
 */
void
set_scr_col (buffer * buf)
{
    int i;

    buf->scr_col = 0;
    for (i = 0; i < buf->pos.col; i++)
    {
	if (buf->pos.line->txt[i] == '\t')
	    buf->scr_col += next_mult (buf->scr_col,
				       cfg_get_global_int_with_default
				       ("hard_tab_width",
					DEF_HARD_TAB_WIDTH));
//              DEF_HARD_TAB_WIDTH );
	else if (is_control (buf->pos.line->txt[i]))
	    buf->scr_col += 2;
	else
	    buf->scr_col++;
    }
}

int
is_control (char c)
{
    unsigned char ch = c;

    if (ch < 9)
	return 1;
    if ((ch > 10) && (ch < 32))
	return 1;
    if ((ch > 126) && (ch < 160))
	return 1;
    return 0;
}

/* Insert a character in the buffer.  Returns 0 on success, or -1 if it runs
 * out of memory.
 */
int
add_char (int c, buffer * buf)
{
    int ch, insc;
    char *p;

    if (buf->pos.line->txt_len == LINELEN (buf->pos) + 1)
    {
	/* need to allocate more space for the insertion */
	char *nbuf;

	nbuf = (char *) realloc (buf->pos.line->txt,
				 buf->pos.line->txt_len +
				 cfg_get_global_int_with_default
				 ("realloc_granularity",
				  DEF_REALLOC_GRAN));
	if (nbuf == NULL)
	    return -1;

	buf->pos.line->txt = nbuf;
	buf->pos.line->txt_len +=
	    cfg_get_global_int_with_default ("realloc_granularity",
					     DEF_REALLOC_GRAN);
    }

    p = buf->pos.line->txt + buf->pos.col;
    insc = c;
    while (*p != '\0')
    {
	ch = *p;
	*p = insc;
	insc = ch;
	p++;
    }

    *p = insc;
    *(p + 1) = '\0';

    buf->pos.col++;

    if (c == '\t')
	buf->scr_col += next_mult (buf->scr_col,
				   cfg_get_global_int_with_default
				   ("hard_tab_width", DEF_HARD_TAB_WIDTH));
//              DEF_HARD_TAB_WIDTH );
    else if (is_control (c))
	buf->scr_col += 2;
    else
	buf->scr_col++;

    buf->preferred_col = buf->scr_col;

    return 0;
}

/* Delete the character preceding the cursor */
void
del_char (buffer * buf)
{
    char *p;
    buf->pos.col--;

    if (buf->pos.line->txt[buf->pos.col] == '\t')
	set_scr_col (buf);
    else if (is_control (buf->pos.line->txt[buf->pos.col]))
	buf->scr_col -= 2;
    else
	buf->scr_col--;

    p = buf->pos.line->txt + buf->pos.col;
    do
    {
	*p = *(p + 1);
	p++;
    }
    while (*p != '\0');

    buf->preferred_col = buf->scr_col;
}

/* The default implementation of auto-indentation.  It just adds spaces to the
 * new line to match the indent of the previous line.  If hard tabs are enabled
 * for the current buffer, it fills the indent with tabs as much as possible;
 * if not, it uses spaces.  This implementation of auto-indent is used when
 * dynamic modes are disabled, if no mode is found, or if the mode does not
 * implement a replacement for auto-indentation.
 */
void
def_indent (buffer * buf, char ch)
{
    char *p;

    if (ch != '\n')
	return;

    for (p = buf->pos.line->prev->txt; (*p == ' ') || (*p == '\t'); p++)
	add_char (*p, buf);

    /* kill the contents of the previous line if there were whitespaces *
     * only.  This way, autoindent doesn't leave a bunch of random whitespace
     * * when it's not necessary. */
    if (*p == '\0')
	*(buf->pos.line->prev->txt) = 0;
}
