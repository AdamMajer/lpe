/* screen.c
 *
 * Copyright (c) 1999 Chris Smith
 *
 * This file is distributed under the GPL, version 2 or at your option any
 * later version.  See COPYING for details.
 */

#include "options.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include SLANG_H
#include <curses.h>
#include <term.h>

#include "lpe.h"
#include "lpecomm.h"
#include "screen.h"
#include "buffer.h"
#include "minibuf.h"
#include "input.h"
#include "help.h"
#include "cfg.h"

/* Definitions of screen update flags.  See screen.h for explanation.
 */
volatile sig_atomic_t refresh_complete;
volatile sig_atomic_t refresh_text;
volatile sig_atomic_t refresh_banner;

/* A global variable used to determine the starting column for the minibuf...
 * this is in turn returned by get_minibuf_col, which is used in minibuf.c to
 * decide when to start scrolling the minibuf for mbuf_ask().
 */
int minibuf_col = 0;

/* Initialize the S-Lang libraries for use in lpe.  This sets up the screen and
 * does other various cleanup needed for screen management.
 */
void
init_slang (void)
{
    SLtt_get_terminfo ();

#ifdef TIOCGWINSZ
    {
	struct winsize ws;

	/* Try to get a better screen size.  However, some terminals
	 * (Microsoft * Windows bundled telnet client, for example) will
	 * report 0x0 as the * size, so check for that first. */
	if (!ioctl (1, TIOCGWINSZ, &ws) && (ws.ws_row != 0)
	    && (ws.ws_col != 0))
	{
	    SLtt_Screen_Rows = ws.ws_row;
	    SLtt_Screen_Cols = ws.ws_col;
	}
    }
#endif				/* defined(TIOCGWINSZ) */

    SLang_init_tty (-1, 1, 0);
    SLang_set_abort_signal (NULL);
    SLsmg_init_smg ();

    refresh_text = 1;
    refresh_banner = 1;
    refresh_complete = 0;
}

/* Initialize key bindings for S-Lang.  This is separated because it need not
 * be done again when the screen is completely refreshed.  In fact, binding
 * these keys several times seems to result in some type of memory leak, so it
 * is really important that this only gets called once.
 */
void
init_slang_keys (void)
{
    char *term, *bp;

    SLkp_init ();

    /* dumb hacks to fix some terminal weirdness */
    term = getenv ("TERM");

    bp = 0;
    if( tgetent( bp, term ) == 1 ){
        if( key_backspace )
    		SLkp_define_keysym (key_backspace, LPE_BACKSPACE_KEY);
    	else
                SLkp_define_keysym ("\010", LPE_BACKSPACE_KEY);

	if( delete_character )
	        SLkp_define_keysym (delete_character, LPE_DELETE_KEY);
	else
                SLkp_define_keysym ("\177", LPE_DELETE_KEY);

        /* FIXME */
        SLkp_define_keysym ("\030", LPE_EXIT_KEY);	        /* Ctrl-X */
        SLkp_define_keysym ("\032", LPE_SUSPEND_KEY);	/* Ctrl-Z */
        SLkp_define_keysym ("\014", LPE_REFRESH_KEY);	/* Ctrl-L */
        SLkp_define_keysym ("\022", LPE_PGUP_KEY);	        /* Ctrl-R */
        SLkp_define_keysym ("\024", LPE_PGDN_KEY);	        /* Ctrl-T */
        SLkp_define_keysym ("\017", LPE_PREV_WORD_KEY);	/* Ctrl-O */
        SLkp_define_keysym ("\020", LPE_NEXT_WORD_KEY);	/* Ctrl-P */
        SLkp_define_keysym ("\013", LPE_KILL_KEY);	        /* Ctrl-K */
        SLkp_define_keysym ("\025", LPE_YANK_KEY);	        /* Ctrl-U */
        SLkp_define_keysym ("\031", LPE_YANK_KEY);          /* Ctrl-Y */
        SLkp_define_keysym ("\023", LPE_SEARCH_KEY);	/* Ctrl-S */
        SLkp_define_keysym ("\001", LPE_FIND_NEXT_KEY);	/* Ctrl-A */
        SLkp_define_keysym ("\006\017", LPE_OPEN_KEY);	/* Ctrl-F Ctrl-O */
        SLkp_define_keysym ("\006\023", LPE_SAVE_KEY);	/* Ctrl-F Ctrl-S */
        SLkp_define_keysym ("\006\001", LPE_SAVE_ALT_KEY);	/* Ctrl-F Ctrl-A */
        SLkp_define_keysym ("\006\022", LPE_READF_KEY);	/* Ctrl-F Ctrl-R */
        SLkp_define_keysym ("\006\005", LPE_CLEARMOD_KEY);	/* Ctrl-F Ctrl-E */
        SLkp_define_keysym ("\006\030", LPE_NEXTBUF_KEY);   /* Ctrl-F Ctrl-X */
        SLkp_define_keysym ("\006\032", LPE_PREVBUF_KEY);   /* Ctrl-F Ctrl-Z */
        SLkp_define_keysym ("\006\016", LPE_OPENBUF_KEY);   /* Ctrl-F Ctrl-N */
        SLkp_define_keysym ("\006\014", LPE_CLOSEBUF_KEY);  /* Ctrl-F Ctrl-L */
        SLkp_define_keysym ("\002\023", LPE_SET_MODE_KEY);	/* Ctrl-B Ctrl-S */
        SLkp_define_keysym ("\002\024", LPE_TAB_SWAP_KEY);	/* Ctrl-B Ctrl-T */
        SLkp_define_keysym ("\002\001", LPE_AI_TOGGLE_KEY);	/* Ctrl-B Ctrl-A */
        SLkp_define_keysym ("\007\021", LPE_HOME_KEY);	/* Ctrl-G Ctrl-Q */
        SLkp_define_keysym ("\007\027", LPE_END_KEY);	/* Ctrl-G Ctrl-W */
        SLkp_define_keysym ("\007\001", LPE_BUF_START_KEY);	/* Ctrl-G Ctrl-A */
        SLkp_define_keysym ("\007\023", LPE_BUF_END_KEY);	/* Ctrl-G Ctrl-S */
        SLkp_define_keysym ("\007\007", LPE_GOTOLN_KEY);	/* Ctrl-G Ctrl-G */
        SLkp_define_keysym ("\016\022", LPE_REP_KEY);	/* Ctrl-N Ctrl-R */
        SLkp_define_keysym ("\016\024", LPE_REP_QUAD_KEY);	/* Ctrl-N Ctrl-T */
        SLkp_define_keysym ("\016\017", LPE_RECORDER_KEY);	/* Ctrl-N Ctrl-O */
        SLkp_define_keysym ("\016\020", LPE_PLAYBACK_KEY);	/* Ctrl-N Ctrl-P */
        SLkp_define_keysym ("\026\026", LPE_SHELL_KEY);	/* Ctrl-V Ctrl-V */
        SLkp_define_keysym ("\026\001", LPE_AWK_KEY);	/* Ctrl-V Ctrl-A */
        SLkp_define_keysym ("\026\023", LPE_SED_KEY);	/* Ctrl-V Ctrl-S */
        SLkp_define_keysym ("\026\002", LPE_SHELL_LN_KEY);	/* Ctrl-V Ctrl-B */
        SLkp_define_keysym ("\026\004", LPE_AWK_LN_KEY);	/* Ctrl-V Ctrl-D */
        SLkp_define_keysym ("\026\006", LPE_SED_LN_KEY);	/* Ctrl-V Ctrl-F */
        SLkp_define_keysym ("\005", LPE_HELP_KEY);	        /* Ctrl-E */
        SLkp_define_keysym ("\004\004", LPE_DEBUG_KEY);	/* Ctrl-D Ctrl-D */
        SLkp_define_keysym ("\004\023", LPE_SLANG_KEY);	/* Ctrl-D Ctrl-S */
    }
    else if (!strcmp (term, "mach") || !strcmp (term, "mach-color"))
    {
        fprintf(stderr, "Tell me if you see this! <adamm@zombino.com>\n");
    	SLkp_define_keysym ("\177", LPE_BACKSPACE_KEY);
    }
    else {
        /* No terminfo - guess backspace and delete keys */
        fprintf(stderr, "No terminfo record found... File a bug against your terminal...");
        SLkp_define_keysym ("\010", LPE_BACKSPACE_KEY);
        SLkp_define_keysym ("\177", LPE_DELETE_KEY);

        SLkp_define_keysym ("\030", LPE_EXIT_KEY);	        /* Ctrl-X */
        SLkp_define_keysym ("\032", LPE_SUSPEND_KEY);	/* Ctrl-Z */
        SLkp_define_keysym ("\014", LPE_REFRESH_KEY);	/* Ctrl-L */
        SLkp_define_keysym ("\022", LPE_PGUP_KEY);	        /* Ctrl-R */
        SLkp_define_keysym ("\024", LPE_PGDN_KEY);	        /* Ctrl-T */
        SLkp_define_keysym ("\017", LPE_PREV_WORD_KEY);	/* Ctrl-O */
        SLkp_define_keysym ("\020", LPE_NEXT_WORD_KEY);	/* Ctrl-P */
        SLkp_define_keysym ("\013", LPE_KILL_KEY);	        /* Ctrl-K */
        SLkp_define_keysym ("\025", LPE_YANK_KEY);	        /* Ctrl-U */
        SLkp_define_keysym ("\031", LPE_YANK_KEY);          /* Ctrl-Y */
        SLkp_define_keysym ("\023", LPE_SEARCH_KEY);	/* Ctrl-S */
        SLkp_define_keysym ("\001", LPE_FIND_NEXT_KEY);	/* Ctrl-A */
        SLkp_define_keysym ("\006\017", LPE_OPEN_KEY);	/* Ctrl-F Ctrl-O */
        SLkp_define_keysym ("\006\023", LPE_SAVE_KEY);	/* Ctrl-F Ctrl-S */
        SLkp_define_keysym ("\006\001", LPE_SAVE_ALT_KEY);	/* Ctrl-F Ctrl-A */
        SLkp_define_keysym ("\006\022", LPE_READF_KEY);	/* Ctrl-F Ctrl-R */
        SLkp_define_keysym ("\006\005", LPE_CLEARMOD_KEY);	/* Ctrl-F Ctrl-E */
        SLkp_define_keysym ("\006\030", LPE_NEXTBUF_KEY);   /* Ctrl-F Ctrl-X */
        SLkp_define_keysym ("\006\032", LPE_PREVBUF_KEY);   /* Ctrl-F Ctrl-Z */
        SLkp_define_keysym ("\006\016", LPE_OPENBUF_KEY);   /* Ctrl-F Ctrl-N */
        SLkp_define_keysym ("\006\014", LPE_CLOSEBUF_KEY);  /* Ctrl-F Ctrl-L */
        SLkp_define_keysym ("\002\023", LPE_SET_MODE_KEY);	/* Ctrl-B Ctrl-S */
        SLkp_define_keysym ("\002\024", LPE_TAB_SWAP_KEY);	/* Ctrl-B Ctrl-T */
        SLkp_define_keysym ("\002\001", LPE_AI_TOGGLE_KEY);	/* Ctrl-B Ctrl-A */
        SLkp_define_keysym ("\007\021", LPE_HOME_KEY);	/* Ctrl-G Ctrl-Q */
        SLkp_define_keysym ("\007\027", LPE_END_KEY);	/* Ctrl-G Ctrl-W */
        SLkp_define_keysym ("\007\001", LPE_BUF_START_KEY);	/* Ctrl-G Ctrl-A */
        SLkp_define_keysym ("\007\023", LPE_BUF_END_KEY);	/* Ctrl-G Ctrl-S */
        SLkp_define_keysym ("\007\007", LPE_GOTOLN_KEY);	/* Ctrl-G Ctrl-G */
        SLkp_define_keysym ("\016\022", LPE_REP_KEY);	/* Ctrl-N Ctrl-R */
        SLkp_define_keysym ("\016\024", LPE_REP_QUAD_KEY);	/* Ctrl-N Ctrl-T */
        SLkp_define_keysym ("\016\017", LPE_RECORDER_KEY);	/* Ctrl-N Ctrl-O */
        SLkp_define_keysym ("\016\020", LPE_PLAYBACK_KEY);	/* Ctrl-N Ctrl-P */
        SLkp_define_keysym ("\026\026", LPE_SHELL_KEY);	/* Ctrl-V Ctrl-V */
        SLkp_define_keysym ("\026\001", LPE_AWK_KEY);	/* Ctrl-V Ctrl-A */
        SLkp_define_keysym ("\026\023", LPE_SED_KEY);	/* Ctrl-V Ctrl-S */
        SLkp_define_keysym ("\026\002", LPE_SHELL_LN_KEY);	/* Ctrl-V Ctrl-B */
        SLkp_define_keysym ("\026\004", LPE_AWK_LN_KEY);	/* Ctrl-V Ctrl-D */
        SLkp_define_keysym ("\026\006", LPE_SED_LN_KEY);	/* Ctrl-V Ctrl-F */
        SLkp_define_keysym ("\005", LPE_HELP_KEY);	        /* Ctrl-E */
        SLkp_define_keysym ("\004\004", LPE_DEBUG_KEY);	/* Ctrl-D Ctrl-D */
        SLkp_define_keysym ("\004\023", LPE_SLANG_KEY);	/* Ctrl-D Ctrl-S */
    }
}

/* Get out of S-Lang's raw terminal mode */
void
cleanup_slang (void)
{
    SLsmg_gotorc (SLtt_Screen_Rows - 1, 0);
    SLsmg_refresh ();
    SLsmg_reset_smg ();
    SLang_reset_tty ();
}

/* Draw the banner at the bottom of the screen */
static void
draw_banner (buffer * buf)
{
    char banner[1024];
    char tmp[20], *ptmp;
    int pos, t;

    if (SLtt_Use_Ansi_Colors)
	SLsmg_set_color (0);

    memset (banner, '-', SLtt_Screen_Cols);
    memcpy (banner + 2, "lpe", 3);
    if (buf->rdonly)
	banner[7] = '%';
    if (buf->modified)
	banner[7] = '*';

    pos = 10;

    if (buf->offerhelp && !is_help)
    {
	ptmp = _("Ctrl-E for Help");
	t = strlen (ptmp);
	memcpy (banner + pos, ptmp, t);
	pos += t + 2;
    }

    t = strlen (buf->name);
    memcpy (banner + pos, buf->name, t);
    pos += t + 2;

    if (buf->mode_name != NULL)
    {
	t = strlen (buf->mode_name);
	memcpy (banner + pos, "<", 1);
	memcpy (banner + pos + 1, buf->mode_name, t);
	memcpy (banner + pos + t + 1, ">", 1);
	pos += t + 4;
    }

    sprintf (tmp, "L%u", buf->linenum + 1);
    t = strlen (tmp);
    memcpy (banner + pos, tmp, t);
    pos += t + 2;

    if (public_repeat_count > 1)
    {
	sprintf (tmp, "(%u)", public_repeat_count);
	t = strlen (tmp);
	memcpy (banner + pos, tmp, t);
	pos += t + 2;
    }

    minibuf_col = pos;

    if (the_mbuf.vis)
    {
	int i = 0;

	t = strlen (the_mbuf.label);
	memcpy (banner + pos, the_mbuf.label, t);
	pos += t;

	t = strlen (the_mbuf.text + the_mbuf.scroll);
	if (t > the_mbuf.width)
	    t = the_mbuf.width;
	else if (t < the_mbuf.width)
	    i = the_mbuf.width - t;

	memcpy (banner + pos, the_mbuf.text + the_mbuf.scroll, t);
	pos += t;

	while (i > 0)
	{
	    banner[pos++] = ' ';
	    i--;
	}
    }

    SLsmg_gotorc (SLtt_Screen_Rows - 1, 0);
    SLsmg_write_nchars (banner, SLtt_Screen_Cols);
}

/* Move the cursor to the place it should be on the screen.  This should be
 * called between any drawing activity and a refresh, because drawing changes
 * the cursor position.
 */
static void
position_cursor (buffer * buf)
{
    if (the_mbuf.focus)
    {
	SLsmg_gotorc (SLtt_Screen_Rows - 1,
		      minibuf_col + strlen (the_mbuf.label)
		      + the_mbuf.pos - the_mbuf.scroll);
    }

    else
    {
	SLsmg_gotorc (buf->linenum - buf->scrollnum,
		      buf->scr_col - buf->scrollcol);
    }
}

/* Writes a representation for this control character into the buffer to print,
 * but backwards so as to coincide with my nifty little write_seg's strange
 * behavior.
 */
static void
control_rep (char c, char *rep)
{
    unsigned char ch = c;

    rep[2] = '\0';

    if (ch < 32)
    {
	rep[1] = '^';
	rep[0] = '@' + ch;
    } else if (ch == '\177')
    {
	rep[1] = '^';
	rep[0] = '?';
    } else if ((ch > 127) && (ch < 160))
    {
	rep[1] = '~';
	rep[0] = '@' + (ch - 128);
    } else
    {
	/* this shouldn't be happening -- so I print something obviously *
	 * wrong so that people will notice if it does! :) */
	rep[1] = '^';
	rep[0] = '!';
    }
}

/* Write a segment of text to the screen, starting at start_col within the
 * line, and writing at most width characters.  Also takes into account tabs
 * in the line.
 */
static int
write_seg (char *txt, int chars, int max_col, int start_col, int vis_col)
{
    /* This is ugly, but at least it works */
    char *p = txt;
    char *pending;
    int npending;
    int col = start_col;
    int htw =
	cfg_get_global_int_with_default ("hard_tab_width",
					 DEF_HARD_TAB_WIDTH);
//      int htw= DEF_HARD_TAB_WIDTH;
    char c;

    npending = 0;
    pending = (char *) malloc (htw + 1);

    while (((p - txt) < chars) || (npending > 0))
    {
	if (npending > 0)
	{
	    c = pending[--npending];
	} else if (*p == '\t')
	{
	    int i, tab_spcs;

	    tab_spcs = htw - (col - ((col / htw) * htw)) - 1;

	    for (i = 0; i < tab_spcs; i++)
		pending[i] = ' ';
	    npending = i;
	    c = ' ';
	    p++;
	} else if (is_control (*p))
	{
	    control_rep (*p, pending);
	    c = pending[1];
	    npending = 1;
	    p++;
	} else
	{
	    c = *p;
	    p++;
	}

	if ((col >= vis_col) && (col < max_col))
	    SLsmg_write_char (c);
	col++;
    }

    free (pending);

    return col;
}

static void
write_line (buffer * buf, buf_line * line, int lnum, int vis_col)
{
    char *txt;
    int i = 0;

    txt = line->txt;

    if ((SLtt_Use_Ansi_Colors) && (buf->highlight) &&
	(buf->mode.highlight != NULL))
    {
	int idx, start, state, len;

	idx = 0;
	state = -1;
	i = 0;

	len = strlen (txt);

	do
	{
	    int color;

	    start = idx;
	    color = (*buf->mode.highlight) (buf, line, lnum, &idx, &state);
	    SLsmg_set_color (color);
	    i = write_seg (txt + start, idx - start,
			   SLtt_Screen_Cols + vis_col - 1, i, vis_col);
	}
	while (idx < len);

	if (line->next != NULL)
	{
	    line->next->start_state = state;
	    buf->state_valid = line->next;
	    buf->state_valid_num = lnum + 1;
	}

	/* back to a normal color for extra space and/or $ */
	SLsmg_set_color (0);
    } else
    {
	int txtlen;

	txtlen = strlen (txt);
	i =
	    write_seg (txt, txtlen, SLtt_Screen_Cols + vis_col - 1, 0,
		       vis_col);
    }

    if (i - vis_col >= SLtt_Screen_Cols)
    {
	SLsmg_write_char ('$');
    }

    else
	while (i - vis_col < SLtt_Screen_Cols)
	{
	    if (i >= vis_col)
		SLsmg_write_char (' ');
	    i++;
	}
}

/* Draw the current screenful of text to the screen */
static void
draw_text (buffer * buf)
{
    int i;
    buf_line *curline;

    curline = buf->scrollpos;
    for (i = 0; (i < SLtt_Screen_Rows - 1) && (curline != NULL); i++)
    {
	SLsmg_gotorc (i, 0);
	write_line (buf, curline, buf->scrollnum + i, buf->scrollcol);
	curline = curline->next;
    }
    while (i < SLtt_Screen_Rows - 1)
    {
	SLsmg_gotorc (i, 0);

	SLsmg_write_nstring (cfg_get_global_string_with_default
			     ("eof_fill", DEF_EOF_FILL), SLtt_Screen_Cols);
	i++;
    }
}

/* Completely refresh the screen, re-initializing S-Lang and everything.  Used
 * in case something gets screwed up, or when (as with Ctrl-Z) the editor gets
 * suspended and then resumed, and needs to be restored.
 */
static void
refresh_screen (buffer * buf)
{
    SLsmg_reset_smg ();
    SLang_reset_tty ();
    init_slang ();
    draw_text (buf);
    draw_banner (buf);
    position_cursor (buf);
    SLsmg_refresh ();
}

void
draw_screen (buffer * buf)
{
    if (refresh_complete)
    {
	refresh_screen (buf);

	refresh_complete = 0;
	refresh_text = 0;
	refresh_banner = 0;
	return;
    }

    if (refresh_text || refresh_banner)
    {
	if (refresh_text)
	{
	    draw_text (buf);
	    refresh_text = 0;
	}

	if (refresh_banner)
	{
	    draw_banner (buf);
	    refresh_banner = 0;
	}
    }

    position_cursor (buf);
    SLsmg_refresh ();
}

int
get_minibuf_col (void)
{
    return minibuf_col;
}
