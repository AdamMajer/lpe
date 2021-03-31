/* help.c
 *
 * Copyright (c) 1999 Chris Smith
 *
 * This file is distributed under the GPL, version 2 or at your option any
 * later version.  See COPYING for details.
 */

#include "options.h"
#include "buffer.h"
#include "help.h"
#include "i18n.h"
#include "cfg.h"

#include <errno.h>
#include <stdlib.h>

buffer help_buf;
int help_initialized = 0;
int is_help = 0;

static buf_line *
read_string (char *p)
{
    buf_line *start;
    buf_pos pos;

    start = (buf_line *) malloc (sizeof (buf_line));
    if (start == NULL)
    {
	errno = ENOMEM;
	return NULL;
    }

    start->txt = (char *) malloc (1);
    if (start->txt == NULL)
    {
	free (start);
	errno = ENOMEM;
	return NULL;
    }

    start->txt[0] = '\0';
    start->txt_len = 1;

    start->prev = NULL;
    start->next = NULL;

    pos.line = start;
    pos.col = 0;

    while (*p != '\0')
    {
	int c;
	c = *(p++);

	if (c == '\n')
	{
	    buf_line *t;

	    pos.line->txt[pos.col] = '\0';
	    t = (buf_line *) malloc (sizeof (buf_line));
	    if (t == NULL)
	    {
		free_list (start);
		errno = ENOMEM;
		return NULL;
	    }

	    t->txt = (char *) malloc (1);
	    if (t->txt == NULL)
	    {
		free_list (start);
		free (t);
		errno = ENOMEM;
		return NULL;
	    }

	    pos.line->next = t;
	    pos.line->next->txt_len = 1;

	    pos.line->next->prev = pos.line;
	    pos.line->next->next = NULL;

	    pos.line = pos.line->next;
	    pos.col = 0;
	}

	else
	{
	    pos.line->txt[pos.col] = (char) c;
	    if (++pos.col == pos.line->txt_len)
	    {
		char *ntxt;

		ntxt = (char *) realloc (pos.line->txt,
					 pos.line->txt_len +
					 cfg_get_global_int_with_default
					 ("realloc_granularity",
					  DEF_REALLOC_GRAN));
		if (ntxt == NULL)
		{
		    free_list (start);
		    errno = ENOMEM;
		    return NULL;
		}

		pos.line->txt = ntxt;
		pos.line->txt_len +=
		    cfg_get_global_int_with_default ("realloc_granularity",
						     DEF_REALLOC_GRAN);
	    }
	}
    }

    return start;
}

static int
init_helpbuf (void)
{
    buffer *buf = &help_buf;
    char *help_text = _("Lpe: The Lightweight Programmer's Editor\n"
			"Help Screen\n"
			"\n"
			"Ctrl-Q           -- Move cursor to beginning of line (alternative to Home)\n"
			"Ctrl-W           -- Move cursor to end of line (alternative to End)\n"
			"Ctrl-R           -- Scroll one screen up (alternative to PgUp)\n"
			"Ctrl-T           -- Scroll one screen down (alternative to PgDn)\n"
			"Ctrl-O           -- Move to the next word\n"
			"Ctrl-P           -- Move to the previous word\n"
			"\n"
			"Ctrl-K           -- Kill the current line\n"
			"Ctrl-Y or Ctrl-U -- Insert the most recent block of killed lines\n"
			"Ctrl-S           -- Search for a specified string in the file\n"
			"Ctrl-A           -- Search again for the last search query\n"
			"\n"
			"Ctrl-F Ctrl-O    -- Open a new file to replace the current buffer\n"
			"Ctrl-F Ctrl-S    -- Save the buffer to disk\n"
			"Ctrl-F Ctrl-A    -- Save to disk with an alternate file name\n"
			"Ctrl-F Ctrl-R    -- Read a file and insert it at the current cursor position\n"
			"Ctrl-F Ctrl-E    -- Pretend that a buffer hasn't been modified\n"
			"Ctrl-F Ctrl-X    -- Next buffer\n"
			"Ctrl-F Ctrl-Z    -- Previous buffer\n"
			"Ctrl-F Ctrl-N    -- Open a new buffer from a file\n"
			"Ctrl-F Ctrl-L    -- Close current buffer\n"
			"\n"
			"Ctrl-B Ctrl-S    -- Set the mode of the current buffer\n"
			"Ctrl-B Ctrl-T    -- Toggle between hard and soft tabs for this buffer\n"
			"Ctrl-B Ctrl-A    -- Toggle automatic indentation of this buffer\n"
			"\n"
			"Ctrl-G Ctrl-A    -- Go to the first line of the buffer\n"
			"Ctrl-G Ctrl-S    -- Go to the last line of the buffer\n"
			"Ctrl-G Ctrl-G    -- Go to a specific line number of the buffer\n"
			"\n"
			"Ctrl-N Ctrl-R    -- Enter a value for the command repeater\n"
			"Ctrl-N Ctrl-T    -- Multiply the command repeater value by four\n"
			"Ctrl-N Ctrl-O    -- Start or stop recording a macro\n"
			"Ctrl-N Ctrl-P    -- Play back the last recorded macro\n"
			"\n"
			"Ctrl-V Ctrl-V    -- Pass the entire buffer through a shell command\n"
			"Ctrl-V Ctrl-A    -- Pass the entire buffer through an awk script\n"
			"Ctrl-V Ctrl-S    -- Pass the entire buffer through a sed script\n"
			"Ctrl-V Ctrl-B    -- Pass several lines of the buffer through a shell command\n"
			"Ctrl-V Ctrl-D    -- Pass several lines of the buffer through an awk script\n"
			"Ctrl-V Ctrl-F    -- Pass several lines of the buffer through a sed script\n"
			"\n"
			"Ctrl-D Ctrl-D    -- Perform an internal debug command\n"
			"Ctrl-D Ctrl-S    -- Execute a SLang command\n"
			"\n"
			"Ctrl-X           -- Write ALL buffers to disk and exit\n"
			"<interrupt>      -- Exit without writing to disk\n"
			"Ctrl-Z           -- Suspend the editor and escape to a prompt\n"
			"Ctrl-L           -- Erase and redraw the entire screen\n"
			"\n"
			"Notes:\n"
			"1. <interrupt> means your default terminal interrupt key, which is normally\n"
			"   set to Ctrl-C.\n"
			"\n"
			"2. Ctrl-Z is used to abort and suspend the editor.  This is completely\n"
			"   independent of the suspend key defined for your terminal.  Sorry about this,\n"
			"   but it appears to be a limitation of the screen management library (S-Lang)\n"
			"   that I use, which provides no option to preserve the suspend key, so I have\n"
			"   to emulate it with kill(0, SIGSTOP).\n");

    buf->text = read_string (help_text);
    buf->rdonly = 1;

    if (buf->text == NULL)
    {
	errno = ENOMEM;
	return -1;
    }

    buf->modified = 0;
    buf->scrollpos = buf->text;
    buf->scrollnum = 0;
    buf->scrollcol = 0;
    buf->pos.line = buf->text;
    buf->pos.col = 0;
    buf->scr_col = 0;
    buf->preferred_col = 0;
    buf->linenum = 0;

    buf->fname = NULL;
    buf->name = "?";
    buf->mode_name = NULL;

    return 0;
}

buffer *
get_helpbuf (void)
{
    if (!help_initialized)
	init_helpbuf ();
    return &help_buf;
}
