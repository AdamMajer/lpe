/* minibuf.c
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
#include <ctype.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <signal.h>
#include LIMITS_H

#include "lpe.h"
#include "input.h"
#include "minibuf.h"
#include "screen.h"
#include "mode-utils.h"

/* The global minibuffer.  It is used both to ask the user for information in
 * commands that require it and to notify the user about various events.  There
 * is really a need for only one of these because there's only one status bar,
 * so it's stored in a global variable here.
 */
minibuf the_mbuf;

/* Set up the minibuf to ask the user for information using the specified
 * prompt.
 */
static void
setup_mbuf_ask (buffer * buf, char *prompt, int compl)
{
    the_mbuf.vis = 1;
    the_mbuf.focus = 1;
    the_mbuf.pos = 0;
    the_mbuf.scroll = 0;
    the_mbuf.compl = compl;

    strcpy (the_mbuf.label, prompt);
    the_mbuf.text[0] = '\0';

    the_mbuf.width = SLtt_Screen_Cols - get_minibuf_col () -
	strlen (the_mbuf.label) - 3;
	/* given a sufficiently large filename, we may have no room at all */
	if(the_mbuf.width < 0)
		the_mbuf.width = 0;
}

/* Set up the minibuf to print a notification message for the user.
 */
static void
setup_mbuf_tell (buffer * buf, char *msg)
{
    the_mbuf.vis = 1;
    the_mbuf.focus = 0;
    the_mbuf.scroll = 0;

    strcpy (the_mbuf.text, msg);
    the_mbuf.label[0] = '\0';

    the_mbuf.width = strlen (the_mbuf.text);
}

/* Make the minibuf invisible after asking a question with it */
static void
cleanup_mbuf (void)
{
    the_mbuf.compl = MBUF_NO_COMPL;
    the_mbuf.vis = 0;
    the_mbuf.focus = 0;
}

/* Check to see if the minibuf needs to be scrolled sideways.  This is called
 * after cursor movement to see that the buffer scrolls as it would be expected
 * to do.
 */
static int
mbuf_check_scrolling (void)
{
    int ret = 0;

    if (the_mbuf.pos < the_mbuf.scroll)
    {
	the_mbuf.scroll = the_mbuf.pos;
	ret = 1;
    }

    if (the_mbuf.pos >= the_mbuf.scroll + the_mbuf.width)
    {
	the_mbuf.scroll = the_mbuf.pos - the_mbuf.width + 1;
	ret = 1;
    }

    return ret;
}

/* qsort compare function for alphabetical sorting */
static int
mbuf_alpha_sort (const void *a, const void *b)
{
    int i = 0;

    const char *pa = ((const char **) (a))[0];
    const char *pb = ((const char **) b)[0];

    while (*(pa + i) && *(pb + i))
	if (*(pa + i) != *(pb + i))
	    return *(pa + i) < *(pb + i) ? -1 : 1;
	else
	    ++i;
    return 0;
}

/* Find available modes.  The only guarantee that libtool gives on library
 * names is that they'll be accompanied by a libfoo.la file, so I search for
 * that -- and the module name then becomes foo.  For now, I assume there's a
 * symlink from foo.so to the appropriate actual library name on the host
 * system; these symlinks are created at install time to make it easier to
 * find an appropriate module for a mode.
 *
 * FIXME: the above description is outdated
 *	- Gergely Nagy
 */
static int
mbuf_modes_avail (char ***modes)
{
    DIR *dir;
    struct dirent *de;

    char **am;
    char pn[PATH_MAX];
    char *path, *nc, *mn;

    int nl;
    int mc = 1;

    am = NULL;

    path = getenv ("LPE_MODULE_PATH");
    if (path == NULL)
	path =
	    cfg_get_global_string_with_default ("module_path",
						DEF_MODULE_PATH);

    if ((am = (char **) malloc ((mc + 1) * sizeof (char *))) == NULL)
	;			/* XXX */
    am[0] = "genmode";

    do
    {
	if (*path == '~')
	{
	    char *home = getenv ("HOME");
	    strcpy (pn, (home != NULL ? home : "/"));
	    path++;
	} else
	{
	    *pn = '\0';
	}

	nc = strchr (path, ':');

	if (nc != NULL)
	{
	    strncat (pn, path, nc - path);
	    path += nc - path + 1;
	} else
	{
	    strcat (pn, path);
	}

	if ((dir = opendir (pn)) != NULL)
	{
	    while ((de = readdir (dir)) != 0)
	    {
		if (
		    (am =
		     (char **) realloc (am,
					(mc + 1) * sizeof (char *))) ==
		    NULL)
		    ;		/* XXX */

		if ((mn = strstr (de->d_name, ".la")) != NULL)
		    if ((nl = strlen (de->d_name)) == mn - de->d_name + 3)
			/* if (strncmp(de->d_name, "lib", 3) == 0) { *mn
			 * ='\0'; if ((am[mc] = (char *)malloc(nl + 1)) ==
			 * NULL) ; strcpy(am[mc], de->d_name + 3); ++mc; } */
		    {
			*mn = '\0';
			if (
			    (am[mc] =
			     (char *) malloc (strlen (de->d_name) + 1)) ==
			    NULL)
			    ;	/* XXX */
			strcpy (am[mc], de->d_name);
			++mc;
		    }

	    }
	    closedir (dir);
	}
    }
    while (nc != 0);

    *modes = am;
    return mc;
}

/* Try to <tab> complete a mode: */
static void
mbuf_mode_compl (void)
{
    char **am;
    int ml;
    int found = -1;
    int i = 0;
    int amn = 0;

    if ((amn = mbuf_modes_avail (&am)) == 0)
    {
	SLtt_beep ();
	return;
    }
    qsort (am, amn, sizeof (char *), mbuf_alpha_sort);

    if (strlen (the_mbuf.text) > 0)
    {
	if ((the_mbuf.compl & MBUF_PEND_COMPL) != MBUF_PEND_COMPL)
	{
	    do
	    {
		if (strstr (am[i], the_mbuf.text) == am[i])
		{
		    found = i;
		    break;
		}
	    }
	    while (++i < amn);
	} else
	{
	    do
	    {
		if (strcmp (the_mbuf.text, am[i]) == 0)
		{
		    found = (i + 1 >= amn) ? 0 : (i + 1);
		    break;
		}
	    }
	    while (++i < amn);
	}
    } else
    {
	found = 0;
    }

    if (found > -1)
    {
	if ((ml = strlen (am[found])) > 99)
	    ml = 99;
	strncpy (the_mbuf.text, am[found], ml);
	the_mbuf.text[ml] = '\0';
	the_mbuf.pos = ml;
	the_mbuf.compl |= MBUF_PEND_COMPL;
    } else
    {
	SLtt_beep ();
    }

    while (amn-- > 1)
    {
	if (strcmp (am[amn], "genmode"))
	    free (am[amn]);
    }

    free (am);
}

/* Find available files.
 */
static int
mbuf_files_avail (char ***files)
{
    DIR *dir;
    struct dirent *de;

    char **am;
    int mc = 0;

    am = NULL;

    if ((am = (char **) malloc ((mc + 1) * sizeof (char *))) == NULL)
	;			/* XXX */

    if ((dir = opendir (".")) != NULL)
    {
	while ((de = readdir (dir)) != 0)
	{
	    if ((am = (char **) realloc (am, (mc + 1) * sizeof (char *)))
		== NULL)
		;		/* XXX */

	    if (de->d_name[0] != '.')
	    {
		if ((am[mc] = (char *) malloc (strlen (de->d_name) + 1)) ==
		    NULL)
		    ;		/* XXX */
		strcpy (am[mc], de->d_name);
		++mc;
	    }
	}

	closedir (dir);
    }

    *files = am;
    return mc;
}

/* Try to <tab> complete a file: */
static void
mbuf_file_compl (void)
{
    char **am;
    int ml;
    int found = -1;
    int i = 0;
    int amn = 0;

    if ((amn = mbuf_files_avail (&am)) == 0)
    {
	SLtt_beep ();
	return;
    }
    qsort (am, amn, sizeof (char *), mbuf_alpha_sort);

    if (strlen (the_mbuf.text) > 0)
    {
	if ((the_mbuf.compl & MBUF_PEND_COMPL) != MBUF_PEND_COMPL)
	{
	    do
	    {
		if (strstr (am[i], the_mbuf.text) == am[i])
		{
		    found = i;
		    break;
		}
	    }
	    while (++i < amn);
	} else
	{
	    do
	    {
		if (strcmp (the_mbuf.text, am[i]) == 0)
		{
		    found = (i + 1 >= amn) ? 0 : (i + 1);
		    break;
		}
	    }
	    while (++i < amn);
	}
    } else
    {
	found = 0;
    }

    if (found > -1)
    {
	if ((ml = strlen (am[found])) > 99)
	    ml = 99;
	strncpy (the_mbuf.text, am[found], ml);
	the_mbuf.text[ml] = '\0';
	the_mbuf.pos = ml;
	the_mbuf.compl |= MBUF_PEND_COMPL;
    } else
    {
	SLtt_beep ();
    }

    while (amn-- > 1)
    {
	free (am[amn]);
    }

    free (am);
}

/* Handle the tab key according to the compl flag in minibuf */
static void
mbuf_key_tab (buffer * buf)
{
    if ((the_mbuf.compl & MBUF_FILE_COMPL) == MBUF_FILE_COMPL)
    {
	mbuf_file_compl ();
	mbuf_check_scrolling ();
	refresh_banner = 1;
    } else if ((the_mbuf.compl & MBUF_MODE_COMPL) == MBUF_MODE_COMPL)
    {
	mbuf_mode_compl ();
	mbuf_check_scrolling ();
	refresh_banner = 1;
    } else
    {
	SLtt_beep ();
    }
}

/* Move one character to the left while typing in the minibuf */
static void
mbuf_key_left (buffer * buf)
{
    if (the_mbuf.pos == 0)
	SLtt_beep ();
    else
    {
	the_mbuf.pos--;
	if (mbuf_check_scrolling ())
	    refresh_banner = 1;
    }
}

/* Move one character to the right while typing in the minibuf */
static void
mbuf_key_right (buffer * buf)
{
    if (the_mbuf.text[the_mbuf.pos] == '\0')
	SLtt_beep ();
    else
    {
	the_mbuf.pos++;
	if (mbuf_check_scrolling ())
	    refresh_banner = 1;
    }
}

/* Move the cursor to the beginning of the minibuf */
static void
mbuf_key_home (buffer * buf)
{
    if (the_mbuf.pos == 0)
	SLtt_beep ();
    else
    {
	the_mbuf.pos = 0;
	if (mbuf_check_scrolling ())
	    refresh_banner = 1;
    }
}

/* Move the cursor to the end of the minibuf. */
static void
mbuf_key_end (buffer * buf)
{
    if (the_mbuf.text[the_mbuf.pos] == '\0')
	SLtt_beep ();
    else
    {
	the_mbuf.pos = strlen (the_mbuf.text);
	if (mbuf_check_scrolling ())
	    refresh_banner = 1;
    }
}

static void
mbuf_key_next_word (buffer * buf)
{
    /* skip the rest of this word and the whitespace after it. */
    while ((the_mbuf.text[the_mbuf.pos] != '\0') &&
	   !isspace (the_mbuf.text[the_mbuf.pos]))
    {
	the_mbuf.pos++;
    }
    while (isspace (the_mbuf.text[the_mbuf.pos]))
    {
	the_mbuf.pos++;
    }

    if (mbuf_check_scrolling ())
	refresh_banner = 1;
}

static void
mbuf_key_prev_word (buffer * buf)
{
    /* if we're at the beginning of this word, skip whitespace to get to the
     * * previous word.  Then go to the beginning of the word. */
    while ((the_mbuf.pos != 0)
	   && isspace (the_mbuf.text[the_mbuf.pos - 1]))
    {
	the_mbuf.pos--;
    }
    while ((the_mbuf.pos != 0)
	   && !isspace (the_mbuf.text[the_mbuf.pos - 1]))
    {
	the_mbuf.pos--;
    }

    if (mbuf_check_scrolling ())
	refresh_banner = 1;
}

/* Delete the character in the minibuf just before the cursor */
static void
mbuf_key_backspace (buffer * buf)
{
    if (the_mbuf.pos == 0)
	SLtt_beep ();
    else
    {
	char *p;

	the_mbuf.pos--;
	p = the_mbuf.text + the_mbuf.pos;
	do
	{
	    *p = *(p + 1);
	    ++p;
	}
	while (*p != '\0');

	mbuf_check_scrolling ();
	refresh_banner = 1;
    }
}

/* Delete the character in the minibuf just after the cursor position */
static void
mbuf_key_delete (buffer * buf)
{
    if (the_mbuf.pos == strlen (the_mbuf.text))
	SLtt_beep ();
    else
    {
	char *p;
	p = the_mbuf.text + the_mbuf.pos;

	while (*p != '\0')
	{
	    *p = *(p + 1);
	    ++p;
	}

	refresh_banner = 1;
    }
}

/* Handle a key type in the minibuf by inserting it into the text. */
static void
mbuf_key_typed (buffer * buf, int c)
{
    if (strlen (the_mbuf.text) == 99)
	SLtt_beep ();
    else
    {
	char *p;
	char ch;

	p = the_mbuf.text + the_mbuf.pos;
	while (*p != '\0')
	{
	    ch = *p;
	    *p = c;
	    c = ch;
	    p++;
	}

	*p = c;
	*(p + 1) = '\0';

	the_mbuf.pos++;

	mbuf_check_scrolling ();
	refresh_banner = 1;
    }
}

/* Elegantly suspend the editor while the minibuf has focus.  This replaces the
 * default mechanism because S-Lang seems to provide no option to preserve the
 * stop key (traditionally Ctrl-Z).  So I am reduced to simulating it in lpe by
 * sending SIGSTOP to the current process manually.
 */
static void
mbuf_key_suspend (buffer * buf)
{
    cleanup_slang ();
    raise (SIGTSTP);
    init_slang ();
}

/* The main input loop for the minibuf.  Gets keys from the keyboard and puts
 * dispatches them as appropriate.
 */
static int
mbuf_process_input (buffer * buf)
{
    int c;

    if (!SLang_input_pending (0))
	draw_screen (buf);
    c = SLkp_getkey ();
    if (SLKeyBoard_Quit)
	return 1;

    if (c != '\t')
	the_mbuf.compl &= ~MBUF_PEND_COMPL;

    switch (c)
    {
	case LPE_LEFT_KEY:
	    mbuf_key_left (buf);
	    break;

	case LPE_RIGHT_KEY:
	    mbuf_key_right (buf);
	    break;

	case LPE_HOME_KEY:
	    mbuf_key_home (buf);
	    break;

	case LPE_END_KEY:
	    mbuf_key_end (buf);
	    break;

	case LPE_NEXT_WORD_KEY:
	    mbuf_key_next_word (buf);
	    break;

	case LPE_PREV_WORD_KEY:
	    mbuf_key_prev_word (buf);
	    break;

	case LPE_BACKSPACE_KEY:
	    mbuf_key_backspace (buf);
	    break;

	case LPE_DELETE_KEY:
	    mbuf_key_delete (buf);
	    break;

	case '\n':
	case '\r':
	case LPE_ENTER_KEY:
	    return 1;

	case LPE_REFRESH_KEY:
	    refresh_complete = 1;
	    break;

	case LPE_SUSPEND_KEY:
	    mbuf_key_suspend (buf);
	    break;

	case '\t':
	    mbuf_key_tab (buf);
	    break;

	case LPE_UP_KEY:
	case LPE_DOWN_KEY:
	case LPE_PGUP_KEY:
	case LPE_PGDN_KEY:
	case LPE_BUF_START_KEY:
	case LPE_BUF_END_KEY:
	case LPE_OPEN_KEY:
	case LPE_SAVE_KEY:
	case LPE_SAVE_ALT_KEY:
	case LPE_EXIT_KEY:
	case LPE_KILL_KEY:
	case LPE_YANK_KEY:
	case LPE_SEARCH_KEY:
	case LPE_FIND_NEXT_KEY:
	case LPE_GOTOLN_KEY:
	case LPE_READF_KEY:
	case LPE_CLEARMOD_KEY:
	case LPE_TAB_SWAP_KEY:
	case LPE_REP_KEY:
	case LPE_REP_QUAD_KEY:
	case LPE_SET_MODE_KEY:
	case LPE_AI_TOGGLE_KEY:
	case LPE_RECORDER_KEY:
	case LPE_PLAYBACK_KEY:
	case LPE_DEBUG_KEY:
	    SLtt_beep ();
	    break;

	default:
	    /* ignore control characters */
	    if (c >= 27)
		mbuf_key_typed (buf, c);
	    break;
    }

    return 0;
}

/* Ask a question using the minibuf.  This function forms half of the external
 * interface to the minibuf.  It is called when a command needs extra
 * information to prompt for and read that information.
 */
char *
mbuf_ask (buffer * buf, char *prompt, int compl)
{
    setup_mbuf_ask (buf, prompt, compl);

    refresh_banner = 1;
    while (!mbuf_process_input (buf)) ;

    cleanup_mbuf ();

    refresh_banner = 1;
    return SLKeyBoard_Quit ? NULL : the_mbuf.text;
}

/* Tell the user something using the minibuf.  This is the other half of the
 * external interface to the minibuffer.  It is called whenever a command needs
 * to print an error (without dying) or a notification that it succeeded.
 */
void
mbuf_tell (buffer * buf, char *msg)
{
    setup_mbuf_tell (buf, msg);
    refresh_banner = 1;
}
