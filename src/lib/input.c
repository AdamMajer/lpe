/* input.c
 *
 * Copyright (c) 1999 Chris Smith
 *
 * This file is distributed under the GPL, version 2 or at your option any
 * later version.  See COPYING for details.
 */

#include "options.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <sys/file.h>
#include <sys/wait.h>
#include <limits.h>
#include <fcntl.h>

#include "lpe.h"
#include "lpecomm.h"
#include "input.h"
#include "screen.h"
#include "minibuf.h"
#include "help.h"
#include "cfg.h"

#if !defined(HAVE_FLOCK) || defined(DUMMY_FLOCK)
#define flock(a,b)
#endif

#define VALID_STATE(x,y) {                  \
    if ((x) == NULL)                        \
    {                                       \
        buf->state_valid_num = 0;           \
        buf->state_valid = buf->text;       \
        buf->text->start_state = 0;         \
    }                                       \
    else if (buf->state_valid_num > (y))    \
    {                                       \
        buf->state_valid_num = (y);         \
        buf->state_valid = (x);             \
    }                                       \
}

/* A global variable to hold the current command repeater count and command
 * -- together, these values allow any command to be performed many times with
 * very few keystrokes.
 */
static int cmd_repeat_count = 0;
static int cmd_repeat_command = 0;
int public_repeat_count;

/* Global variables to implement the macro recorder.
 */
static int macro_keys[MAX_MACRO_KEYS];
static int macro_nkeys = 0;
static int macro_recording = 0;
static int macro_pos = -1;

static char macro_answers[MAX_MACRO_ANSWER_LEN];
static int macro_answer_pos = 0;
static int macro_answer_len = 0;

static int macro_reps[MAX_MACRO_KEYS];
static int macro_rep_save = -1;

/* A wrapper for calls to mbuf_ask; the purpose is that when the macro recorder
 * is enabled, answers to mbuf questions are recorded and played back.  When
 * the macro recorder is disabled, I can only hope the compiler is smart enough
 * to inline this function (which would essentially make it go away.
 */
static char *
wrap_mbuf_ask (buffer * buf, char *prompt, int compl)
{
    if (macro_pos != -1)
    {
	/* A macro is playing; don't ask, and return the previously recorded
	 * * answer.  Also, advance the answer position so the next mbuf
	 * question * returns the next answer.  If something is wrong and we
	 * are out of * recorded answers, fall through to ask interactively. */
	if (macro_answer_pos < macro_answer_len)
	{
	    char *ans = macro_answers + macro_answer_pos;
	    macro_answer_pos += strlen (ans) + 1;
	    return ans;
	}
    } else if (macro_recording)
    {
	/* A macro is recording; ask the question, but record the answer in * 
	 * mbuf_answers before returning it. */
	char *ans = mbuf_ask (buf, prompt, compl);
	if (ans == NULL)
	{
	    /* Record a '\0' -- it's all the same */
	    macro_answers[macro_answer_len++] = '\0';
	    return ans;
	} else
	{
	    if (macro_answer_len + strlen (ans) + 1 > MAX_MACRO_ANSWER_LEN)
	    {
		/* This is just as much an overflow as running out of keys */
		mbuf_tell (buf, _("Macro overflow"));
		SLtt_beep ();
		macro_recording = 0;
		macro_nkeys = 0;
	    }

	    strcpy (macro_answers + macro_answer_len, ans);
	    macro_answer_len += strlen (ans) + 1;

	    return ans;
	}
    } else
	return mbuf_ask (buf, prompt, compl);

    /* 
     * FIXME: is it right to return NULL here ?
     *  - Gergely Nagy
     */
    return NULL;
}

/* Respond to an unknown key sequence.  It's not really an option to beep or
 * otherwise notify the user, because S-Lang sends this for some pretty common
 * keys, like F11 and F12.  For now, actually, I am not doing anything.  If
 * this trend continues, I'll get rid of this function.
 */
static void
key_error (buffer * buf)
{
}

/* Move up a single line in the buffer */
static void
key_up (buffer * buf)
{
    is_killing = 0;

    if (buf->pos.line->prev == NULL)
	SLtt_beep ();
    else
    {
	buf->pos.line = buf->pos.line->prev;
	buf->linenum--;

	check_col (buf);
	if (check_scrolling (buf))
	    refresh_text = 1;
	refresh_banner = 1;
    }
}

/* Move down a line in the buffer */
static void
key_down (buffer * buf)
{
    is_killing = 0;

    if (buf->pos.line->next == NULL)
	SLtt_beep ();
    else
    {
	buf->pos.line = buf->pos.line->next;
	buf->linenum++;

	check_col (buf);
	if (check_scrolling (buf))
	    refresh_text = 1;
	refresh_banner = 1;
    }
}

/* Move left one character, wrapping to the previous line if the cursor is at
 * the beginning of the current line.
 */
static void
key_left (buffer * buf)
{
    is_killing = 0;

    if (buf->pos.col > 0)
    {
	buf->pos.col--;

	if (buf->pos.line->txt[buf->pos.col] == '\t')
	    set_scr_col (buf);
	else if (is_control (buf->pos.line->txt[buf->pos.col]))
	    buf->scr_col -= 2;
	else
	    buf->scr_col--;
    }

    else
    {
	if (buf->pos.line == buf->text)
	    SLtt_beep ();
	else
	{
	    buf->pos.line = buf->pos.line->prev;
	    buf->linenum--;
	    buf->pos.col = strlen (buf->pos.line->txt);
	    set_scr_col (buf);
	    refresh_banner = 1;
	}
    }

    buf->preferred_col = buf->scr_col;
    if (check_scrolling (buf))
	refresh_text = 1;
}

/* Move right one character, wrapping to the next line if the end of a line is
 * reached.
 */
static void
key_right (buffer * buf)
{
    is_killing = 0;

    if (buf->pos.col < LINELEN (buf->pos))
    {
	if (buf->pos.line->txt[buf->pos.col] == '\t')
	    buf->scr_col += next_mult (buf->scr_col,
				       cfg_get_global_int_with_default
				       ("hard_tab_width",
					DEF_HARD_TAB_WIDTH));
	else if (is_control (buf->pos.line->txt[buf->pos.col]))
	    buf->scr_col += 2;
	else
	    buf->scr_col++;

	buf->pos.col++;
    }

    else
    {
	if (buf->pos.line->next == NULL)
	    SLtt_beep ();
	else
	{
	    buf->pos.line = buf->pos.line->next;
	    buf->linenum++;
	    buf->pos.col = 0;
	    buf->scr_col = 0;
	    refresh_banner = 1;
	}
    }

    buf->preferred_col = buf->scr_col;
    if (check_scrolling (buf))
	refresh_text = 1;
}

/* Move up one page of text */
static void
key_pgup (buffer * buf)
{
    int i;

    is_killing = 0;

    if (buf->pos.line->prev == NULL)
	SLtt_beep ();
    else
    {
	for (i = 0;
	     (i < SLtt_Screen_Rows - 2) && buf->pos.line->prev != NULL;
	     i++)
	{
	    buf->pos.line = buf->pos.line->prev;
	    buf->linenum--;
	}

	check_col (buf);
	if (check_scrolling (buf))
	    refresh_text = 1;
	refresh_banner = 1;
    }
}

/* Move down one page of text */
static void
key_pgdn (buffer * buf)
{
    int i;

    is_killing = 0;

    if (buf->pos.line->next == NULL)
	SLtt_beep ();
    else
    {
	for (i = 0;
	     (i < SLtt_Screen_Rows - 2) && buf->pos.line->next != NULL;
	     i++)
	{
	    buf->pos.line = buf->pos.line->next;
	    buf->linenum++;
	}

	check_col (buf);

	if (check_scrolling (buf))
	    refresh_text = 1;
	refresh_banner = 1;
    }
}

/* Move to the beginning of the current line.  If automatic indents are on,
 * then do the nifty thing where home takes you to the first actual character
 * on the line.  Otherwise, assume that the indent means nothing and go to the
 * real beginning of the line.
 */
static void
key_home (buffer * buf)
{
    is_killing = 0;

    if (buf->pos.col == 0)
    {
	int i = buf->pos.col;
	buf->scr_col = 0;

	while (buf->pos.line->txt[i] != '\0')
	{
	    if (buf->pos.line->txt[i] == '\t')
		buf->scr_col +=
		    cfg_get_global_int_with_default ("hard_tab_width",
						     DEF_HARD_TAB_WIDTH);
	    else if (buf->pos.line->txt[i] == ' ')
		buf->scr_col++;
	    else
		break;
	    i++;
	}

	buf->pos.col = i;
    } else
    {
	int i = 0;
	buf->scr_col = 0;

	/* Find the first non-space character of the line */
	while (i < buf->pos.col)
	{
	    if (buf->pos.line->txt[i] == '\t')
		buf->scr_col +=
		    cfg_get_global_int_with_default ("hard_tab_width",
						     DEF_HARD_TAB_WIDTH);
	    else if (buf->pos.line->txt[i] == ' ')
		buf->scr_col++;
	    else
		break;

	    i++;
	}

	if (i == buf->pos.col)
	    buf->pos.col = buf->scr_col = 0;
	else
	    buf->pos.col = i;
    }

    buf->preferred_col = buf->scr_col;
    if (check_scrolling (buf))
	refresh_text = 1;
}

/* Move to the end of the current line */
static void
key_end (buffer * buf)
{
    is_killing = 0;

    buf->pos.col = LINELEN (buf->pos);
    set_scr_col (buf);
    buf->preferred_col = buf->scr_col;

    if (check_scrolling (buf))
	refresh_text = 1;
}

static void
key_next_word (buffer * buf)
{
    /* skip the rest of this word and the whitespace after it. */
    while ((buf->pos.line->txt[buf->pos.col] != '\0') &&
	   !isspace (buf->pos.line->txt[buf->pos.col]))
    {
	buf->pos.col++;
    }
    while ((buf->pos.line->txt[buf->pos.col] == '\0') ||
	   isspace (buf->pos.line->txt[buf->pos.col]))
    {
	if (buf->pos.line->txt[buf->pos.col] == '\0')
	{
	    if (buf->pos.line->next == NULL)
		break;

	    buf->pos.line = buf->pos.line->next;
	    buf->linenum++;
	    buf->pos.col = 0;
	    refresh_banner = 1;
	} else
	    buf->pos.col++;
    }

    set_scr_col (buf);
    buf->preferred_col = buf->scr_col;

    if (check_scrolling (buf))
	refresh_text = 1;
}

static void
key_prev_word (buffer * buf)
{
    /* if we're at the beginning of this word, skip whitespace to get to the
     * * previous word.  Then go to the beginning of the word. */
    while ((buf->pos.col == 0)
	   || isspace (buf->pos.line->txt[buf->pos.col - 1]))
    {
	if (buf->pos.col == 0)
	{
	    if (buf->pos.line->prev == NULL)
		break;

	    buf->pos.line = buf->pos.line->prev;
	    buf->linenum--;
	    buf->pos.col = strlen (buf->pos.line->txt);
	    refresh_banner = 1;
	} else
	    buf->pos.col--;
    }

    while ((buf->pos.col != 0)
	   && !isspace (buf->pos.line->txt[buf->pos.col - 1]))
    {
	buf->pos.col--;
    }

    set_scr_col (buf);
    buf->preferred_col = buf->scr_col;

    if (check_scrolling (buf))
	refresh_text = 1;
}

/* Delete the character preceding the cursor */
void
key_backspace (buffer * buf)
{
    is_killing = 0;

    if (buf->pos.col == 0)
    {
	if (buf->pos.line->prev == NULL)
	    SLtt_beep ();

	else
	{
	    int reqlen, prevlen;
	    buf_line *old_line;

	    prevlen = strlen (buf->pos.line->prev->txt);
	    reqlen = buf->pos.line->txt_len + prevlen;

	    if (buf->pos.line->prev->txt_len < reqlen)
	    {
		char *nbuf;

		nbuf = (char *) realloc (buf->pos.line->prev->txt, reqlen);
		if (nbuf == NULL)
		{
		    mbuf_tell (buf, strerror (ENOMEM));
		    SLtt_beep ();
		    return;
		}

		buf->pos.line->prev->txt = nbuf;
		buf->pos.line->prev->txt_len = reqlen;
	    }

	    buf->pos.col = prevlen;
	    strcat (buf->pos.line->prev->txt + prevlen,
		    buf->pos.line->txt);

	    old_line = buf->pos.line;
	    buf->pos.line = buf->pos.line->prev;
	    buf->linenum--;
	    buf->pos.line->next = old_line->next;
	    if (old_line->next != NULL)
		old_line->next->prev = buf->pos.line;

	    if (buf->scrollpos == old_line)
	    {
		buf->scrollpos = buf->pos.line;
		buf->scrollnum--;
	    }

	    free (old_line->txt);
	    free (old_line);

	    set_scr_col (buf);
	    buf->modified = 1;
	    refresh_banner = 1;
	    refresh_text = 1;
	}
	buf->preferred_col = buf->scr_col;
    }

    else
    {
	del_char (buf);
	buf->modified = 1;
	refresh_text = 1;
	refresh_banner = 1;
    }

    check_scrolling (buf);
    VALID_STATE (buf->pos.line, buf->linenum);
}

/* Delete the character after the cursor */
static void
key_delete (buffer * buf)
{
    is_killing = 0;

    if (buf->pos.col == LINELEN (buf->pos))
    {
	if (buf->pos.line->next == NULL)
	    SLtt_beep ();

	else
	{
	    int curlen, reqlen;
	    buf_line *old_line;

	    curlen = LINELEN (buf->pos);
	    reqlen = curlen + buf->pos.line->next->txt_len;

	    if (buf->pos.line->txt_len < reqlen)
	    {
		char *nbuf;

		nbuf = (char *) realloc (buf->pos.line->txt, reqlen);
		if (nbuf == NULL)
		{
		    mbuf_tell (buf, strerror (ENOMEM));
		    SLtt_beep ();
		    return;
		}

		buf->pos.line->txt = nbuf;
		buf->pos.line->txt_len = reqlen;
	    }

	    strcat (buf->pos.line->txt + curlen, buf->pos.line->next->txt);

	    old_line = buf->pos.line->next;
	    buf->pos.line->next = old_line->next;
	    if (old_line->next != NULL)
		old_line->next->prev = buf->pos.line;

	    free (old_line->txt);
	    free (old_line);

	    buf->modified = 1;
	    refresh_text = 1;
	    refresh_banner = 1;
	}
    }

    else
    {
	char *p;
	p = buf->pos.line->txt + buf->pos.col;

	while (*p != '\0')
	{
	    *p = *(p + 1);
	    p++;
	}

	buf->modified = 1;
	refresh_text = 1;
	refresh_banner = 1;
    }

    VALID_STATE (buf->pos.line, buf->linenum);
}

/* Break the current line at the cursor position and insert a new line of text
 * below it... (that is, same thing enter generally does)
 */
static void
key_enter (buffer * buf)
{
    buf_line *line;

    is_killing = 0;

    line = (buf_line *) malloc (sizeof (buf_line));
    if (line == NULL)
    {
	mbuf_tell (buf, strerror (ENOMEM));
	SLtt_beep ();
	return;
    }

    line->txt_len = buf->pos.line->txt_len - buf->pos.col;
    line->txt = (char *) malloc (line->txt_len);
    if (line->txt == NULL)
    {
	free (line);
	mbuf_tell (buf, strerror (ENOMEM));
	SLtt_beep ();
	return;
    }

    line->next = buf->pos.line->next;
    line->prev = buf->pos.line;
    line->prev->next = line;
    if (line->next != NULL)
	line->next->prev = line;

    strcpy (line->txt, buf->pos.line->txt + buf->pos.col);
    buf->pos.line->txt[buf->pos.col] = '\0';

    buf->pos.line = line;
    buf->linenum++;

    buf->pos.col = 0;
    buf->scr_col = 0;
    buf->preferred_col = 0;

    check_scrolling (buf);

    if ((buf->autoindent == 1) && (!SLang_input_pending (0)))
    {
	if (buf->mode.indent)
	{
	    if ((*buf->mode.indent) (buf, '\n'))
	    {
		mbuf_tell (buf, "Error in indent");
		SLtt_beep ();
	    }
	} else
	{
	    def_indent (buf, '\n');
	}
    }

    buf->modified = 1;
    refresh_text = 1;
    refresh_banner = 1;

    VALID_STATE (buf->pos.line->prev, buf->linenum - 1);
}

/* Delete the current line of text, and add it to the killbuf to be inserted
 * elsewhere (or not).
 */
static void
key_kill (buffer * buf)
{
    buf_line **iter;

    if (!is_killing)
    {
	/* clear out the kill buffer */
	while (killbuf != NULL)
	{
	    buf_line *t;

	    t = killbuf;
	    killbuf = killbuf->next;

	    free (t->txt);
	    free (t);
	}
    }

    if (buf->pos.line->next == NULL)
    {
	if (LINELEN (buf->pos) == 0)
	{
	    SLtt_beep ();
	    return;
	}

	else
	{
	    /* Allocate a new, empty line to tack onto the end of the buffer.
	     * * This is done so that the kill can proceed as most users would
	     * * expect it to -- by deleting the current line and leaving it *
	     * blank. */
	    buf_line *nline;

	    nline = (buf_line *) malloc (sizeof (buf_line));
	    if (nline == NULL)
	    {
		mbuf_tell (buf, strerror (ENOMEM));
		SLtt_beep ();
		return;
	    }

	    nline->next = NULL;
	    nline->prev = buf->pos.line;
	    nline->txt_len = 1;

	    nline->txt = (char *) malloc (1);
	    if (nline->txt == NULL)
	    {
		free (nline);
		mbuf_tell (buf, strerror (ENOMEM));
		SLtt_beep ();
		return;
	    }

	    nline->txt[0] = '\0';
	    buf->pos.line->next = nline;
	}
    }

    iter = &killbuf;
    while (*iter != NULL)
	iter = &((*iter)->next);
    *iter = buf->pos.line;

    if (buf->pos.line->prev != NULL)
	buf->pos.line->prev->next = buf->pos.line->next;
    else
	buf->text = buf->pos.line->next;

    if (buf->scrollpos == buf->pos.line)
	buf->scrollpos = buf->pos.line->next;

    buf->pos.line->next->prev = buf->pos.line->prev;
    buf->pos.line = buf->pos.line->next;

    (*iter)->prev = NULL;
    (*iter)->next = NULL;

    check_col (buf);
    check_scrolling (buf);

    is_killing = 1;
    buf->modified = 1;
    refresh_text = 1;
    refresh_banner = 1;

    VALID_STATE (buf->pos.line->prev, buf->linenum - 1);
}

/* Insert the contents of the killbuf before the current line of text. */
static void
key_yank (buffer * buf)
{
    /* The only tricky part is that we need to build the block to insert
     * first, * and then stick it in there as a whole.  This ensures that if
     * lpe runs * out of memory in the middle, the buffer will still be
     * intact. */
    buf_line *srciter, **dstiter;
    buf_line *lines, *end;
    int numlines;

    is_killing = 0;

    lines = NULL;
    end = NULL;
    dstiter = &lines;
    numlines = 0;

    for (srciter = killbuf; srciter != NULL; srciter = srciter->next)
    {
	buf_line *nline;

	nline = (buf_line *) malloc (sizeof (buf_line));
	if (nline == NULL)
	{
	    buf_line *iter;
	    for (iter = lines; (iter != buf->pos.line) && (iter != NULL);)
	    {
		buf_line *t;
		t = iter->next;

		free (iter->txt);
		free (iter);
		iter = t;
	    }

	    mbuf_tell (buf, strerror (ENOMEM));
	    SLtt_beep ();
	    return;
	}

	nline->txt = (char *) malloc (srciter->txt_len);
	if (nline->txt == NULL)
	{
	    buf_line *iter;
	    for (iter = lines; (iter != buf->pos.line) && (iter != NULL);)
	    {
		buf_line *t;
		t = iter->next;

		free (iter->txt);
		free (iter);
		iter = t;
	    }

	    free (nline);

	    mbuf_tell (buf, strerror (ENOMEM));
	    SLtt_beep ();
	    return;
	}

	nline->txt_len = srciter->txt_len;
	strcpy (nline->txt, srciter->txt);
	nline->next = buf->pos.line;
	nline->prev = end;

	if (nline->prev != NULL)
	{
	    nline->prev->next = nline;
	}

	end = nline;
	*(dstiter) = nline;
	dstiter = &(nline->next);
	numlines++;
    }

    if (lines != NULL)
    {
	if (buf->state_valid_num >= buf->linenum)
	{
	    lines->start_state = buf->pos.line->start_state;
	    buf->state_valid = lines;
	    buf->state_valid_num = buf->linenum;
	}

	lines->prev = buf->pos.line->prev;

	if (buf->scrollpos == buf->pos.line)
	    buf->scrollpos = lines;

	if (lines->prev == NULL)
	    buf->text = lines;
	else
	    lines->prev->next = lines;

	buf->pos.line->prev = end;
	buf->linenum += numlines;

	check_scrolling (buf);
	check_col (buf);

	buf->modified = 1;
	refresh_text = 1;
	refresh_banner = 1;
    }

    else
	SLtt_beep ();
}

/* Save the buffer to an alternate file name.  This command differs from the
 * traditional Windows-style "save as" command because the buffer keeps its
 * current name, but is simply saved to the alternate file name.  Additionally,
 * if the previous buffer was modified it remains marked modified.  This is by
 * design, because an lpe process is supposed to remain associated with a
 * specific file.  If the user wants to edit the new file, they are welcome to
 * open the file after saving to it.
 */
static void
key_save_alt (buffer * buf)
{
    char *newname, *oldname;
    int mod_save;

    is_killing = 0;

    newname = wrap_mbuf_ask (buf, _("file: "), MBUF_FILE_COMPL);
    if ((newname == NULL) || (newname[0] == '\0'))
	return;

    oldname = buf->fname;
    mod_save = buf->modified;

    buf->fname = newname;

    if (save_buffer (buf) == -1)
    {
	mbuf_tell (buf, strerror (errno));
	SLtt_beep ();
    } else
	mbuf_tell (buf, _("File saved successfully"));

    buf->fname = oldname;
    buf->modified = mod_save;
}

/* Save the buffer to disk */
static void
key_save (buffer * buf)
{
    is_killing = 0;

    if (!buf->fname_valid)
    {
        key_save_alt(buf);
        return;
    }

    if (save_buffer (buf) == -1)
    {
	mbuf_tell (buf, strerror (errno));
	SLtt_beep ();
    }
    else mbuf_tell (buf, _("File saved successfully"));
}

/* Open a new file in the buffer, replacing the one that is there currently.
 * This command saves the killbuf, so that text can be cut from one file and
 * inserted into another with ease.  Note that the command will fail if the
 * current buffer is modified.  To open a file in this case, the user must
 * either save changes to the current buffer, or explicitly tell the editor to
 * forget them.  This is by design, in a conscious attempt to avoid a prompt
 * in the minibuffer asking whether to save changes.
 */
static void
key_open (buffer * buf)
{
    static char open_fname[100];

    is_killing = 0;

    if (buf->modified)
    {
	mbuf_tell (buf, _("Current buffer modified"));
	SLtt_beep ();
    }

    else
    {
	char *fname;
	buffer nbuf;

	fname = wrap_mbuf_ask (buf, _("file: "), MBUF_FILE_COMPL);
	if ((fname == NULL) || (fname[0] == '\0'))
	    return;

	strcpy (open_fname, fname);

	if (open_buffer (&nbuf, open_fname, NULL) == -1)
	{
	    mbuf_tell (buf, strerror (errno));
	    SLtt_beep ();
	    return;
	}

	if (buf->mode.leave)
	    (*buf->mode.leave) (buf);
	close_buffer (buf);

	/* Make the new buffer the open buffer */
	if (nbuf.mode.enter)
	    (*nbuf.mode.enter) (buf);
	nbuf.next = buf->next;
	nbuf.prev = buf->prev;
	memcpy (buf, &nbuf, sizeof (buffer));

	refresh_text = 1;
	refresh_banner = 1;
    }
}

/* Save and quit the editor in one keystroke. */
static void
key_exit (buffer * buf)
{
    int err = 0;
    buffer *node;
    is_killing = 0;

    node = the_buf;
    do
    {
    	if (!node)
		break;
        if (node->fname_valid)
        {
            if (save_buffer(node) == -1)
                err = 1;
        }
        node = node->next;
    } while (node != the_buf);

    if (err)
    {
	mbuf_tell (buf, strerror (errno));
	SLtt_beep ();
    }
    else
	quit = 1;
}

/* change buffers */
static void key_nextbuf()
{
    if (the_buf->mode.leave)
        (*the_buf->mode.leave)(the_buf);

    the_buf = the_buf->next;

    if (the_buf->mode.enter)
        (*the_buf->mode.enter)(the_buf);

    refresh_complete = 1;
    draw_screen(the_buf);
}

static void key_prevbuf()
{
    if (the_buf->mode.leave)
        (*the_buf->mode.leave)(the_buf);

    the_buf = the_buf->prev;

    if (the_buf->mode.enter)
        (*the_buf->mode.enter)(the_buf);

    refresh_complete = 1;
    draw_screen(the_buf);
}

/* load a file into a new buffer */
static void key_openbuf()
{
    char *fname;
    buffer *new = malloc(sizeof(buffer));
    if (new == NULL)
    {
        mbuf_tell(the_buf, strerror(ENOMEM));
        SLtt_beep();
        return;
    }
    memset(new, 0, sizeof(buffer));

    fname = wrap_mbuf_ask(the_buf, _("file: "), MBUF_FILE_COMPL);
    if ((fname == NULL) || (fname[0] == '\0'))
    {
        free(new);
        return;
    }

    if (open_buffer(new, fname, NULL) == -1)
    {
        mbuf_tell(the_buf, strerror(errno));
        SLtt_beep();
        free(new);
        return;
    }

    if (the_buf->mode.leave)
        (*the_buf->mode.leave)(the_buf);

    new->next = the_buf->next;
    new->prev = the_buf;
    the_buf->next->prev = new;
    the_buf->next = new;
    the_buf = the_buf->next;

    if (the_buf->mode.enter)
        (*the_buf->mode.enter)(the_buf);

    refresh_complete = 1;
    draw_screen(the_buf);
}

/* close and free the current buffer */
static void key_closebuf()
{
    buffer *node;

    if (the_buf->next == the_buf)
    {
        mbuf_tell(the_buf, "Cannot close last buffer");
        SLtt_beep();
        return;
    }

    if (the_buf->mode.leave)
        (*the_buf->mode.leave)(the_buf);

    close_buffer(the_buf);

    the_buf->next->prev = the_buf->prev;
    the_buf->prev->next = the_buf->next;

    node = the_buf;
    the_buf = the_buf->next;
    free(node);

    refresh_complete = 1;
    draw_screen(the_buf);
}

/* Search for the given string forward in the buffer.  If found, move the
 * cursor to the next occurrence of the string.
 */
static void
key_search (buffer * buf, int parent_key)
{
    static char str[50] = "";
    char *search_str;
    int col_start;
    buf_line *iter;
    int lines_forward;
    is_killing = 0;
    buf->preferred_col = buf->scr_col;

    search_str = str;

    if (parent_key == LPE_SEARCH_KEY || *search_str == '\0')
    {
	search_str =
	    wrap_mbuf_ask (buf, _("search term: "), MBUF_NO_COMPL);
	if ((search_str == NULL) || (*search_str == '\0'))
	    return;
	strcpy (str, search_str);
    }

    if (buf->pos.line->txt[buf->pos.col] == '\0')
    {
	iter = buf->pos.line->next;
	lines_forward = 1;
	col_start = 0;
    }

    else
    {
	iter = buf->pos.line;
	lines_forward = 0;
	col_start = buf->pos.col + 1;
    }

    while (iter != NULL)
    {
	char *result;

	result = strstr (iter->txt + col_start, search_str);
	if (result == NULL)
	{
	    iter = iter->next;
	    lines_forward++;
	    col_start = 0;
	} else
	{
	    buf->pos.line = iter;
	    buf->linenum += lines_forward;
	    buf->pos.col = result - iter->txt;
	    set_scr_col (buf);
	    buf->preferred_col = buf->scr_col;

	    mbuf_tell (buf, _("Found"));

	    if (check_scrolling (buf))
		refresh_text = 1;
	    refresh_banner = 1;

	    return;
	}
    }

    mbuf_tell (buf, _("Not found"));
    SLtt_beep ();
    refresh_banner = 1;
}

/* Move the cursor to the line number specified in the minibuf.
 */
static void
key_gotoln (buffer * buf)
{
    int i, ln, diff, back;
    char *ans, *end;
    buf_line *line;

    is_killing = 0;

    ans = wrap_mbuf_ask (buf, _("line: "), MBUF_NO_COMPL);
    if ((ans == NULL) || (ans[0] == '\0'))
	return;

    ln = strtol (ans, &end, 10) - 1;
    if (*end != '\0')
    {
	SLtt_beep ();
	return;
    }

    diff = ln - buf->linenum;
    back = 0;

    if (diff < 0)
    {
	diff = -diff;
	back = 1;
    }

    line = buf->pos.line;

    if (back)
    {
	for (i = 0; i < diff; i++)
	{
	    if (line->prev == NULL)
	    {
		SLtt_beep ();
		return;
	    } else
		line = line->prev;
	}
    } else
    {
	for (i = 0; i < diff; i++)
	{
	    if (line->next == NULL)
	    {
		SLtt_beep ();
		return;
	    } else
		line = line->next;
	}
    }

    buf->pos.line = line;
    buf->linenum += (back ? -diff : diff);

    check_col (buf);
    if (check_scrolling (buf))
	refresh_text = 1;
    refresh_banner = 1;
}

/* Read the contents of a file into the current buffer, inserting it at the
 * cursor position.
 */
static void
key_read_file (buffer * buf)
{
    FILE *fp;
    char *fname;
    buf_line *new_text, *iter;
    int err_save, nlines;
    int lneeded;
    int llen, i;

    is_killing = 0;

    fname = wrap_mbuf_ask (buf, _("file: "), MBUF_FILE_COMPL);
    if ((fname == NULL) || (fname[0] == '\0'))
	return;

    fp = fopen (fname, "r");
    if (fp == NULL)
    {
	mbuf_tell (buf, strerror (errno));
	SLtt_beep ();
	return;
    }

    flock (fileno (fp), LOCK_SH);
    new_text = read_stream (fp);
    err_save = errno;
    flock (fileno (fp), LOCK_UN);
    fclose (fp);

    if (new_text == NULL)
    {
	mbuf_tell (buf, strerror (err_save));
	SLtt_beep ();
	return;
    }

    /* At this point I'm sure enough it's gonna work to invalidate the DFA */
    VALID_STATE (buf->pos.line, buf->linenum);

    /* Count the number of lines in the new file.  This is needed to update * 
     * buf->linenum; it's also very ugly and inefficient compared to tracking
     * * as the file is read, so I should really change read_stream to return
     * * this information in an output parameter.  I wrote this so as to leave
     * * iter pointing to the last line; I'll use that in a sec. */
    nlines = 0;
    for (iter = new_text; iter->next != NULL; iter = iter->next)
	nlines++;

    /* break the current line at the cursor point and insert the text in
     * there. */
    llen = strlen (iter->txt);
    lneeded = buf->pos.line->txt_len - buf->pos.col + llen;
    if (iter->txt_len < lneeded)
    {
	char *ntxt;
	ntxt = (char *) realloc (iter->txt, lneeded);
	if (ntxt == NULL)
	{
	    mbuf_tell (buf, strerror (ENOMEM));
	    SLtt_beep ();
	    free_list (new_text);
	    return;
	}

	iter->txt = ntxt;
	iter->txt_len = lneeded;
    }

    /* This just barely works with aliasing by one-line files; we just *
     * increased iter->txt_len (in that case aka, new_text->txt_len) to the * 
     * required length after buf->pos.col, so adding buf->pos.col to that is
     * * the right thing to do. */
    lneeded = buf->pos.col + new_text->txt_len;
    if (buf->pos.line->txt_len < lneeded)
    {
	char *ntxt;
	ntxt = (char *) realloc (buf->pos.line->txt, lneeded);
	if (ntxt == NULL)
	{
	    mbuf_tell (buf, strerror (ENOMEM));
	    SLtt_beep ();
	    free_list (new_text);
	    return;
	}

	buf->pos.line->txt = ntxt;
	buf->pos.line->txt_len = lneeded;
    }

    strcpy (iter->txt + llen, buf->pos.line->txt + buf->pos.col);
    strcpy (buf->pos.line->txt + buf->pos.col, new_text->txt);

    iter->next = buf->pos.line->next;
    if (iter->next != NULL)
	iter->next->prev = iter;

    buf->pos.line->next = new_text->next;
    if (new_text->next != NULL)
	new_text->next->prev = buf->pos.line;

    for (i = 0; i < nlines; i++)
    {
	buf->pos.line = buf->pos.line->next;
	buf->pos.col = 0;
    }

    buf->linenum += nlines;
    buf->pos.col += llen;
    set_scr_col (buf);
    buf->preferred_col = buf->scr_col;

    buf->modified = 1;

    check_scrolling (buf);
    refresh_text = 1;
    refresh_banner = 1;
}

/* Clear the buffer modified flag.  This represents a particular design
 * strategy for lpe.  Instead of confirming an operation that would discard a
 * modified buffer, the operation should simply fail.  If the user wants to
 * do so anyway, he/she simply tells the editor to forget changes and then
 * retries.
 */
static void
key_clearmod (buffer * buf)
{
    is_killing = 0;

    if (buf->modified)
    {
	buf->modified = 0;
	refresh_banner = 1;
    }
}

/* Move to the beginning of the entire buffer */
static void
key_buf_start (buffer * buf)
{
    is_killing = 0;

    buf->scrollpos = buf->text;
    buf->scrollnum = 0;
    buf->scrollcol = 0;
    buf->pos.line = buf->text;
    buf->pos.col = 0;
    buf->scr_col = 0;
    buf->linenum = 0;
    buf->preferred_col = 0;

    refresh_text = 1;
    refresh_banner = 1;
}

/* Move to the end of the entire buffer */
static void
key_buf_end (buffer * buf)
{
    is_killing = 0;

    while (buf->pos.line->next != NULL)
    {
	buf->pos.line = buf->pos.line->next;
	buf->linenum++;
    }

    buf->pos.col = strlen (buf->pos.line->txt);
    set_scr_col (buf);
    buf->preferred_col = buf->scr_col;

    if (check_scrolling (buf))
	refresh_text = 1;
    refresh_banner = 1;
}

/* Switch tab handling behaviors between hard and soft tabs */
static void
key_tab_swap (buffer * buf)
{
    is_killing = 0;

    if (buf->hardtab)
    {
	buf->hardtab = 0;
	mbuf_tell (buf, _("Using soft tabs"));
    } else
    {
	buf->hardtab = 1;
	mbuf_tell (buf, _("Using hard tabs"));
    }
}

/* Respond to a keystroke by inserting that key in the buffer */
static void
key_typed (int c, buffer * buf)
{
    is_killing = 0;

    if (add_char (c, buf))
    {
	mbuf_tell (buf, strerror (ENOMEM));
	SLtt_beep ();
	return;
    }

    buf->modified = 1;

    if ((!SLang_input_pending (0)) && (buf->autoindent == 1))
    {
	if (buf->mode.indent)
	{
	    if ((*buf->mode.indent) (buf, c))
	    {
		mbuf_tell (buf, "Error in indent");
		SLtt_beep ();
	    }
	} else
	{
	    def_indent (buf, c);
	}
    }

    check_scrolling (buf);
    refresh_text = 1;
    refresh_banner = 1;

    VALID_STATE (buf->pos.line, buf->linenum);

    if ((buf->mode.flashbrace != NULL) && !SLang_input_pending (0) &&
	buf->flashbrace)
    {
	int col_save;
	int scr_col_save;
	buf_line *line_save;
	int linenum_save;
	int ret;

	col_save = buf->pos.col;
	scr_col_save = buf->scr_col;
	line_save = buf->pos.line;
	linenum_save = buf->linenum;

	ret = (*buf->mode.flashbrace) (buf);
	if (ret == 1)
	{
	    if ((buf->scr_col >= buf->scrollcol) &&
		(buf->scr_col < buf->scrollcol + SLtt_Screen_Cols))
	    {
		draw_screen (buf);
		SLang_input_pending (cfg_get_global_int_with_default
				     ("flash_time", DEF_FLASH_TIME));
	    }
	}

	buf->pos.col = col_save;
	buf->scr_col = scr_col_save;
	buf->pos.line = line_save;
	buf->linenum = linenum_save;

	if (ret == -1)
	    mbuf_tell (buf, _("Mismatched braces"));
    }
}

/* Insert spaces for a soft tab */
static void
key_soft_tab (buffer * buf)
{
    int nblanks, i;

    is_killing = 0;

    nblanks = next_mult (buf->scr_col, DEF_SOFT_TAB_WIDTH);
    for (i = 0; i < nblanks; i++)
	add_char (' ', buf);

    check_scrolling (buf);
    buf->modified = 1;
    refresh_text = 1;
}

/* Suspend the editor manually.  This is required because S-Lang apparently has
 * decided to disable the Ctrl-Z key with no option to circumvent it.  So lpe
 * must simulate this action by sending itself the SIGSTOP signal.
 */
static void
key_suspend (buffer * buf)
{
    cleanup_slang ();
    raise (SIGTSTP);
    init_slang ();
}

/* Toggle autoindent mode. */
static void
key_ai_toggle (buffer * buf)
{
    is_killing = 0;

    if (buf->autoindent)
    {
	buf->autoindent = 0;
	mbuf_tell (buf, _("Autoindent off"));
    } else
    {
	buf->autoindent = 1;
	mbuf_tell (buf, _("Autoindent on"));
    }
}

/* Get a number of commands for the command repeater */
static void
key_rep (buffer * buf)
{
    char *p;

    p = wrap_mbuf_ask (buf, _("count: "), MBUF_NO_COMPL);
    if ((p != NULL) && (*p != '\0'))
	cmd_repeat_count = atoi (p);
    cmd_repeat_command = 0;

    refresh_banner = 1;
}

/* Quickly quadruple the number of commands in the command repeater.  This
 * allows the user to quickly enter a large number by hitting a key a few
 * times.  For example, if the command repeater is clear, then hitting this key
 * will put 4 into it.  Hitting the key again makes that 16.  Again makes it
 * 64.  A couple more times is 1024.  This number grows exponentially, so it
 * should be easy to reach the general proximity of the desired value of
 * repetitions.
 */
static void
key_rep_quad (buffer * buf)
{
    if (cmd_repeat_count == 0)
	cmd_repeat_count = 4;
    else
	cmd_repeat_count *= 4;
    cmd_repeat_command = 0;

    refresh_banner = 1;
}

/* Start or stop the macro recorder. */
static void
key_recorder (buffer * buf)
{
    if (macro_recording)
    {
	mbuf_tell (buf, _("Macro recorder off"));

	macro_recording = 0;
	return;
    }

    mbuf_tell (buf, _("Macro recorder on"));

    macro_recording = 1;
    macro_nkeys = 0;
    macro_answer_len = 0;
}

/* Replay the keystrokes recorded by the macro recorder.  If this command is
 * invoked while the recorder is recording, it stops the recorder before
 * replaying the sequence.
 */
static void
key_playback (buffer * buf)
{
    macro_recording = 0;

    if (macro_nkeys == 0)
    {
	SLtt_beep ();
	return;
    }

    else
    {
	macro_pos = 0;
	macro_answer_pos = 0;
    }
}

static void
key_set_mode (buffer * buf)
{
    char *reqname;

    reqname = wrap_mbuf_ask (buf, _("mode: "), MBUF_MODE_COMPL);
    if ((reqname == NULL) || (reqname[0] == '\0'))
	return;

    if (buf->mode.leave != NULL)
	(*buf->mode.leave) (buf);
    if (buf->mode.uninit != NULL)
	(*buf->mode.uninit) (buf);
    set_buf_mode (buf, reqname);
    if (buf->mode.enter != NULL)
	(*buf->mode.enter) (buf);

    refresh_text = 1;
    refresh_banner = 1;
}

/* To avoid a nasty deadlock, we actually need to fork twice: once for the
 * writing process and again for the stream manipulation command itself.
 */
static buf_line *
stream_manip (buf_line * txt, char *cmd, char *args[])
{
    int pipes[2][2], status;
    FILE *fp;
    pid_t child, writer;
    buf_line *new_text;

    pipe (pipes[0]);
    pipe (pipes[1]);

    if ((writer = fork ()) == 0)
    {
	FILE *fp;

	close (pipes[0][0]);
	close (pipes[1][0]);
	close (pipes[1][1]);

	fp = fdopen (pipes[0][1], "w");
	if (fp == NULL)
	{
	    close (pipes[0][1]);
	    exit (1);
	}

	write_stream (fp, txt);
	fclose (fp);

	exit (0);
    }

    else if (writer < 0)
    {
	close (pipes[0][0]);
	close (pipes[0][1]);
	close (pipes[1][0]);
	close (pipes[1][1]);

	return NULL;
    }

    if ((child = fork ()) == 0)
    {
	int fderr;

	close (pipes[0][1]);
	close (pipes[1][0]);

	dup2 (pipes[0][0], 0);
	dup2 (pipes[1][1], 1);

	/* Redirect standard error to /dev/null */
	fderr = open ("/dev/null", O_WRONLY);
	dup2 (fderr, 2);

	execvp (cmd, args);
	exit (1);
    } else if (child < 0)
    {
	kill (writer, SIGKILL);
	waitpid (writer, NULL, 0);

	close (pipes[0][0]);
	close (pipes[0][1]);
	close (pipes[1][0]);
	close (pipes[1][1]);

	return NULL;
    }

    close (pipes[0][0]);
    close (pipes[0][1]);
    close (pipes[1][1]);

    fp = fdopen (pipes[1][0], "r");

    if (fp == NULL)
    {
	kill (writer, SIGKILL);
	waitpid (writer, NULL, 0);

	kill (child, SIGKILL);
	waitpid (child, NULL, 0);

	close (pipes[1][0]);

	return NULL;
    }

    new_text = read_stream (fp);
    fclose (fp);

    if (new_text == NULL)
    {
	kill (writer, SIGKILL);
	waitpid (writer, NULL, 0);

	kill (child, SIGKILL);
	waitpid (child, NULL, 0);

	return NULL;
    }

    waitpid (writer, &status, 0);

    if (!WIFEXITED (status) || (WEXITSTATUS (status) != 0))
    {
	kill (child, SIGKILL);
	waitpid (child, NULL, 0);

	free_list (new_text);
	return NULL;
    }

    waitpid (child, &status, 0);

    if (!WIFEXITED (status) || (WEXITSTATUS (status) != 0))
    {
	free_list (new_text);
	return NULL;
    }

    return new_text;
}

/* Pass the current buffer through the specified shell command as a stream, and
 * replace the buffer with the output if the shell command succeeds.
 */
static void
key_shell (buffer * buf)
{
    char *args[4];
    buf_line *new_text;

    args[0] = cfg_get_global_string_with_default ("shell_command", CMD_SH);
    args[1] = "-c";
    args[2] = wrap_mbuf_ask (buf, _("shell: "), MBUF_NO_COMPL);
    args[3] = NULL;

    if ((args[2] == NULL) || (args[2][0] == '\0'))
	return;

    new_text = stream_manip (buf->text, args[0], args);

    if (new_text == NULL)
    {
	mbuf_tell (buf, _("Command failed"));
	SLtt_beep ();
	return;
    }

    free_list (buf->text);
    buf->text = new_text;

    buf->scrollpos = buf->pos.line = buf->text;
    buf->scrollnum = buf->linenum = 0;
    buf->scrollcol = buf->scr_col = buf->preferred_col = buf->pos.col = 0;

    buf->text->start_state = 0;
    buf->state_valid = buf->text;
    buf->state_valid_num = 0;
    buf->modified = 1;
    refresh_text = 1;
    refresh_banner = 1;
}

/* Pass the current buffer through an awk script entered from the minibuf,
 * and replace the buffer with the output if awk succeeds.
 */
static void
key_awk (buffer * buf)
{
    char *args[3];
    buf_line *new_text;

    args[0] = cfg_get_global_string_with_default ("awk_command", CMD_AWK);
    args[1] = wrap_mbuf_ask (buf, _("awk: "), MBUF_NO_COMPL);
    args[2] = NULL;

    if ((args[1] == NULL) || (args[1][0] == '\0'))
	return;

    new_text = stream_manip (buf->text, args[0], args);

    if (new_text == NULL)
    {
	mbuf_tell (buf, _("Command failed"));
	SLtt_beep ();
	return;
    }

    free_list (buf->text);
    buf->text = new_text;

    buf->scrollpos = buf->pos.line = buf->text;
    buf->scrollnum = buf->linenum = 0;
    buf->scrollcol = buf->scr_col = buf->preferred_col = buf->pos.col = 0;

    buf->text->start_state = 0;
    buf->state_valid = buf->text;
    buf->state_valid_num = 0;

    buf->modified = 1;
    refresh_text = 1;
    refresh_banner = 1;
}

/* Pass the current buffer through a sed script entered from the minibuf,
 * and replace the buffer with the output if sed succeeds.
 */
static void
key_sed (buffer * buf)
{
    char *args[3];
    buf_line *new_text;

    args[0] = cfg_get_global_string_with_default ("sed_command", CMD_SED);
    args[1] = wrap_mbuf_ask (buf, _("sed: "), MBUF_NO_COMPL);
    args[2] = NULL;

    if ((args[1] == NULL) || (args[1][0] == '\0'))
	return;

    new_text = stream_manip (buf->text, args[0], args);

    if (new_text == NULL)
    {
	mbuf_tell (buf, _("Command failed"));
	SLtt_beep ();
	return;
    }

    free_list (buf->text);
    buf->text = new_text;

    buf->scrollpos = buf->pos.line = buf->text;
    buf->scrollnum = buf->linenum = 0;
    buf->scrollcol = buf->scr_col = buf->preferred_col = buf->pos.col = 0;

    buf->text->start_state = 0;
    buf->state_valid = buf->text;
    buf->state_valid_num = 0;

    buf->modified = 1;
    refresh_text = 1;
    refresh_banner = 1;
}

/* Pass a range of lines of the current buffer through the specified shell
 * command as a stream.  The range begins at the current line and ends n lines
 * forward, where n is the number in the command repeater.  This is blatant
 * abuse of the repeater, but it's the most intuitive way I can find to design
 * the interface.
 */
static void
key_shell_ln (buffer * buf)
{
    char *args[4];
    buf_line *new_text;
    buf_line *last_before, *first_after, *txt;

    /* unlink the lines from the buffer so that stream_manip doesn't exceed * 
     * the desired range. */
    last_before = buf->pos.line->prev;
    txt = buf->pos.line;
    txt->prev = NULL;

    if (cmd_repeat_count == 0)
	cmd_repeat_count = 1;
    first_after = txt;
    while ((first_after) && (cmd_repeat_count > 0))
    {
	first_after = first_after->next;
	cmd_repeat_count--;
    }

    if (first_after)
	first_after->prev->next = NULL;

    /* do the command */
    args[0] = cfg_get_global_string_with_default ("shell_command", CMD_SH);
    args[1] = "-c";
    args[2] = wrap_mbuf_ask (buf, _("shell: "), MBUF_NO_COMPL);
    args[3] = NULL;

    if ((args[2] == NULL) || (args[2][0] == '\0'))
	return;

    new_text = stream_manip (txt, args[0], args);

    if (new_text == NULL)
    {
	mbuf_tell (buf, _("Command failed"));
	SLtt_beep ();
	return;
    }

    new_text->start_state = buf->pos.line->start_state;
    VALID_STATE (new_text, buf->linenum);

    /* relink the lines back into the buffer */
    if (buf->scrollpos == buf->pos.line)
	buf->scrollpos = new_text;
    buf->pos.line = new_text;
    if (last_before)
    {
	last_before->next = new_text;
	new_text->prev = last_before;
    } else
    {
	buf->text = new_text;
	new_text->prev = NULL;
    }

    while (new_text->next != NULL)
    {
	buf->linenum++;
	new_text = new_text->next;
    }

    new_text->next = first_after;
    if (first_after)
    {
	buf->linenum++;
	first_after->prev = new_text;
	buf->pos.line = first_after;
	buf->pos.col = buf->scr_col = buf->preferred_col = 0;
    } else
    {
	buf->pos.line = new_text;
	buf->pos.col = strlen (new_text->txt);
	set_scr_col (buf);
	buf->preferred_col = buf->scr_col;
    }

    free_list (txt);

    check_scrolling (buf);
    buf->modified = 1;

    refresh_text = 1;
    refresh_banner = 1;
}

static void
key_awk_ln (buffer * buf)
{
    char *args[3];
    buf_line *new_text;
    buf_line *last_before, *first_after, *txt;

    /* unlink the lines from the buffer so that stream_manip doesn't exceed * 
     * the desired range. */
    last_before = buf->pos.line->prev;
    txt = buf->pos.line;
    txt->prev = NULL;

    if (cmd_repeat_count == 0)
	cmd_repeat_count = 1;
    first_after = txt;
    while ((first_after) && (cmd_repeat_count > 0))
    {
	first_after = first_after->next;
	cmd_repeat_count--;
    }

    if (first_after)
	first_after->prev->next = NULL;

    /* do the command */
    args[0] = cfg_get_global_string_with_default ("awk_command", CMD_AWK);
    args[1] = wrap_mbuf_ask (buf, _("awk: "), MBUF_NO_COMPL);
    args[2] = NULL;

    if ((args[1] == NULL) || (args[1][0] == '\0'))
	return;

    new_text = stream_manip (txt, args[0], args);

    if (new_text == NULL)
    {
	mbuf_tell (buf, _("Command failed"));
	SLtt_beep ();
	return;
    }

    new_text->start_state = buf->pos.line->start_state;
    VALID_STATE (new_text, buf->linenum);

    /* relink the lines back into the buffer */
    if (buf->scrollpos == buf->pos.line)
	buf->scrollpos = new_text;
    buf->pos.line = new_text;
    if (last_before)
    {
	last_before->next = new_text;
	new_text->prev = last_before;
    } else
    {
	buf->text = new_text;
	new_text->prev = NULL;
    }

    while (new_text->next != NULL)
    {
	buf->linenum++;
	new_text = new_text->next;
    }

    new_text->next = first_after;
    if (first_after)
    {
	buf->linenum++;
	first_after->prev = new_text;
	buf->pos.line = first_after;
	buf->pos.col = buf->scr_col = buf->preferred_col = 0;
    } else
    {
	buf->pos.line = new_text;
	buf->pos.col = strlen (new_text->txt);
	set_scr_col (buf);
	buf->preferred_col = buf->scr_col;
    }

    free_list (txt);

    check_scrolling (buf);
    buf->modified = 1;

    refresh_text = 1;
    refresh_banner = 1;
}

static void
key_sed_ln (buffer * buf)
{
    char *args[3];
    buf_line *new_text;
    buf_line *last_before, *first_after, *txt;

    /* unlink the lines from the buffer so that stream_manip doesn't exceed * 
     * the desired range. */
    last_before = buf->pos.line->prev;
    txt = buf->pos.line;
    txt->prev = NULL;

    if (cmd_repeat_count == 0)
	cmd_repeat_count = 1;
    first_after = txt;
    while ((first_after) && (cmd_repeat_count > 0))
    {
	first_after = first_after->next;
	cmd_repeat_count--;
    }

    if (first_after)
	first_after->prev->next = NULL;

    /* do the command */
    args[0] = cfg_get_global_string_with_default ("sed_command", CMD_SED);
    args[1] = wrap_mbuf_ask (buf, _("sed: "), MBUF_NO_COMPL);
    args[2] = NULL;

    if ((args[1] == NULL) || (args[1][0] == '\0'))
	return;

    new_text = stream_manip (txt, args[0], args);

    if (new_text == NULL)
    {
	mbuf_tell (buf, _("Command failed"));
	SLtt_beep ();
	return;
    }

    new_text->start_state = buf->pos.line->start_state;
    VALID_STATE (new_text, buf->linenum);

    /* relink the lines back into the buffer */
    if (buf->scrollpos == buf->pos.line)
	buf->scrollpos = new_text;
    buf->pos.line = new_text;
    if (last_before)
    {
	last_before->next = new_text;
	new_text->prev = last_before;
    } else
    {
	buf->text = new_text;
	new_text->prev = NULL;
    }

    while (new_text->next != NULL)
    {
	buf->linenum++;
	new_text = new_text->next;
    }

    new_text->next = first_after;

    if (first_after)
    {
	buf->linenum++;
	first_after->prev = new_text;
	buf->pos.line = first_after;
	buf->pos.col = buf->scr_col = buf->preferred_col = 0;
    } else
    {
	buf->pos.line = new_text;
	buf->pos.col = strlen (new_text->txt);
	set_scr_col (buf);
	buf->preferred_col = buf->scr_col;
    }

    free_list (txt);

    check_scrolling (buf);
    buf->modified = 1;

    refresh_text = 1;
    refresh_banner = 1;
}

static void
key_help (buffer * buf)
{
    static buffer buf_save;
    buffer *buf_help;

    buf_help = get_helpbuf ();
    if (buf_help == NULL)
    {
	SLtt_beep ();
	return;
    }

    if (!is_help)
    {
	memcpy (&buf_save, buf, sizeof (buffer));
	memcpy (buf, buf_help, sizeof (buffer));
	refresh_text = refresh_banner = 1;
	is_help = 1;
    }

    else
    {
	memcpy (buf, &buf_save, sizeof (buffer));
	refresh_text = refresh_banner = 1;
	is_help = 0;
    }
}

/* Perform some debug operation.  This is a defined hook for useful debugging
 * commands and diagnostic information.  It is meant to be used when debugging
 * the editor itself.
 */
static void
key_debug (buffer * buf)
{
    char *cmd;

    is_killing = 0;

    cmd = wrap_mbuf_ask (buf, _("debug: "), MBUF_NO_COMPL);
    if ((cmd == NULL) || (cmd[0] == '\0'))
	return;

    /* Eat all available memory, so that the next command that tries to *
     * allocate memory will fail.  This is used to test response to out of *
     * memory problems.  This is not recommended in normal practice. :-) */
    if (strcmp (cmd, "memtest") == 0)
    {
	while (malloc (1) != NULL) ;
	refresh_banner = 1;
    }

    else
    {
	mbuf_tell (buf, _("Unknown debug request"));
	SLtt_beep ();
    }
}

static void
key_slang (buffer * buf)
{
    char *cmd;

    is_killing = 0;

    cmd = wrap_mbuf_ask (buf, _("command: "), MBUF_NO_COMPL);
    if ((cmd == NULL) || (cmd[0] == '\0'))
	return;

    SLang_load_string (cmd);
    /* to avoid slang stack overflows... */
    SLang_restart (1);
    SLang_set_error(0);

    /* 
     * This is not the nicest thing to do, but certain slang commnds,
     * such as printf ( "\033];A New XTerm title\007\n" ); leave trash
     * behind, that must be cleared.
     *  - Gergely Nagy
     */
    refresh_complete = 1;
}

/* The main input processing loop.  Reads keys and dispatches them to the
 * appropriate handler function described above.
 */
void
process_input (buffer * buf)
{
    int c;

    /* Read a key.  This is complicated for two reasons: first, the command * 
     * repeater.  And second, the macro recorder.  Both might be sources of * 
     * characters other than the keyboard.  And they interact with each other
     * * in interesting ways. * * To keep things sane, the macro recorder does 
     * NOT record command repeater * commands.  Instead, it actually records
     * the result from the command * repeater as a series of keystrokes.  On
     * the other hand, the repeater * will repeat playback commands in full.
     * It won't just repeat the first * keystroke.  This is necessary for the
     * macro recorder to be of any real * use. :-) */
    if (macro_rep_save != -1)
    {
	cmd_repeat_count = macro_rep_save;
	macro_rep_save = -1;
    }

    if (macro_pos >= macro_nkeys)
	macro_pos = -1;
    if (macro_pos == -1)
    {
	if ((cmd_repeat_count == 0) || (cmd_repeat_command == 0))
	{
	    public_repeat_count = cmd_repeat_count;
	    if (!SLang_input_pending (0))
		draw_screen (buf);

	    if (the_mbuf.vis)
	    {
		the_mbuf.vis = 0;
		refresh_banner = 1;
	    }

	    c = SLkp_getkey ();

	    if (cmd_repeat_count > 0)
	    {
		if ((c != LPE_REP_KEY) && (c != LPE_REP_QUAD_KEY) &&
		    (c != LPE_SHELL_LN_KEY) && (c != LPE_AWK_LN_KEY) &&
		    (c != LPE_SED_LN_KEY))
		{
		    cmd_repeat_command = c;
		    cmd_repeat_count--;
		}
	    }
	} else
	{
	    c = cmd_repeat_command;
	    cmd_repeat_count--;
	}
    }

    else
    {
	c = macro_keys[macro_pos];
	macro_rep_save = cmd_repeat_count;
	cmd_repeat_count = macro_reps[macro_pos];
	macro_pos++;
    }

    if ((macro_recording) && (c != LPE_PLAYBACK_KEY) &&
	(c != LPE_RECORDER_KEY) && (c != LPE_REP_KEY) &&
	(c != LPE_REP_QUAD_KEY))
    {
	if (macro_nkeys == MAX_MACRO_KEYS)
	{
	    mbuf_tell (buf, _("Macro overflow"));
	    SLtt_beep ();
	    macro_recording = 0;
	    macro_nkeys = 0;
	} else
	{
	    macro_keys[macro_nkeys] = c;
	    macro_reps[macro_nkeys] = cmd_repeat_count;
	    macro_nkeys++;
	}
    }

    if (SLKeyBoard_Quit)
	quit = 1;
    else
	switch (c)
	{
	    case LPE_ERR_KEY:
		key_error (buf);
		break;

	    case LPE_UP_KEY:
		key_up (buf);
		break;

	    case LPE_DOWN_KEY:
		key_down (buf);
		break;

	    case LPE_LEFT_KEY:
		key_left (buf);
		break;

	    case LPE_RIGHT_KEY:
		key_right (buf);
		break;

	    case LPE_PGUP_KEY:
		key_pgup (buf);
		break;

	    case LPE_PGDN_KEY:
		key_pgdn (buf);
		break;

	    case LPE_HOME_KEY:
		key_home (buf);
		break;

	    case LPE_END_KEY:
		key_end (buf);
		break;

	    case LPE_NEXT_WORD_KEY:
		key_next_word (buf);
		break;

	    case LPE_PREV_WORD_KEY:
		key_prev_word (buf);
		break;

	    case LPE_BACKSPACE_KEY:
		if (!buf->rdonly)
		    key_backspace (buf);
		else
		    SLtt_beep ();
		break;

	    case LPE_DELETE_KEY:
		if (!buf->rdonly)
		    key_delete (buf);
		else
		    SLtt_beep ();
		break;

	    case '\t':
		if (!buf->rdonly)
		{
		    if (buf->hardtab)
			key_typed ('\t', buf);
		    else
			key_soft_tab (buf);
		} else
		    SLtt_beep ();
		break;

	    case '\n':
	    case '\r':
	    case LPE_ENTER_KEY:
		if (!buf->rdonly)
		    key_enter (buf);
		else
		    SLtt_beep ();
		break;

	    case LPE_OPEN_KEY:
		key_open (buf);
		break;

	    case LPE_SAVE_KEY:
		key_save (buf);
		break;

	    case LPE_SAVE_ALT_KEY:
		key_save_alt (buf);
		break;

	    case LPE_EXIT_KEY:
		key_exit (buf);
		break;

	    case LPE_KILL_KEY:
		if (!buf->rdonly)
		    key_kill (buf);
		else
		    SLtt_beep ();
		break;

	    case LPE_YANK_KEY:
		if (!buf->rdonly)
		    key_yank (buf);
		else
		    SLtt_beep ();
		break;

	    case LPE_SEARCH_KEY:
		key_search (buf, LPE_SEARCH_KEY);
		break;

	    case LPE_FIND_NEXT_KEY:
		key_search (buf, LPE_FIND_NEXT_KEY);
		break;

	    case LPE_GOTOLN_KEY:
		key_gotoln (buf);
		break;

	    case LPE_READF_KEY:
		if (!buf->rdonly)
		    key_read_file (buf);
		else
		    SLtt_beep ();
		break;

	    case LPE_CLEARMOD_KEY:
		key_clearmod (buf);
		break;

	    case LPE_BUF_START_KEY:
		key_buf_start (buf);
		break;

	    case LPE_BUF_END_KEY:
		key_buf_end (buf);
		break;

	    case LPE_TAB_SWAP_KEY:
		key_tab_swap (buf);
		break;

	    case LPE_REFRESH_KEY:
		refresh_complete = 1;
		break;

	    case LPE_SUSPEND_KEY:
		key_suspend (buf);
		break;

	    case LPE_AI_TOGGLE_KEY:
		key_ai_toggle (buf);
		break;

	    case LPE_REP_KEY:
		key_rep (buf);
		break;

	    case LPE_REP_QUAD_KEY:
		key_rep_quad (buf);
		break;

	    case LPE_RECORDER_KEY:
		key_recorder (buf);
		break;

	    case LPE_PLAYBACK_KEY:
		key_playback (buf);
		break;

	    case LPE_SET_MODE_KEY:
		key_set_mode (buf);
		break;

	    case LPE_SHELL_KEY:
		if (!buf->rdonly)
		    key_shell (buf);
		else
		    SLtt_beep ();
		break;

	    case LPE_AWK_KEY:
		if (!buf->rdonly)
		    key_awk (buf);
		else
		    SLtt_beep ();
		break;

	    case LPE_SED_KEY:
		if (!buf->rdonly)
		    key_sed (buf);
		else
		    SLtt_beep ();
		break;

	    case LPE_SHELL_LN_KEY:
		if (!buf->rdonly)
		    key_shell_ln (buf);
		else
		    SLtt_beep ();
		break;

	    case LPE_AWK_LN_KEY:
		if (!buf->rdonly)
		    key_awk_ln (buf);
		else
		    SLtt_beep ();
		break;

	    case LPE_SED_LN_KEY:
		if (!buf->rdonly)
		    key_sed_ln (buf);
		else
		    SLtt_beep ();
		break;

	    case LPE_HELP_KEY:
		key_help (buf);
                break;

            case LPE_NEXTBUF_KEY:
               if(is_help) key_help(buf);
               else key_nextbuf();
                break;

            case LPE_PREVBUF_KEY:
               if(is_help) key_help(buf);
                else key_prevbuf();
                break;

            case LPE_OPENBUF_KEY:
               if(is_help) key_help(buf);
                key_openbuf();
                break;

            case LPE_CLOSEBUF_KEY:
                if(is_help) key_help(buf);
                else key_closebuf();
		break;

	    case LPE_DEBUG_KEY:
		key_debug (buf);
		break;

	    case LPE_SLANG_KEY:
		key_slang (buf);
		break;

	    default:
		/* If it's a special key, then the mode must have registered
		 * it, * so pass it along.  If not, then type it. */
		if (c > 0x2000)
		{
		    if (buf->mode.extkey != NULL)
			(*buf->mode.extkey) (buf, c);
		} else if (!buf->rdonly)
		    key_typed (c, buf);
		else
		    SLtt_beep ();
		break;
	}
}
