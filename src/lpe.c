/* lpe.c
 *
 * Copyright (c) 1999 Chris Smith
 *
 * This file is distributed under the GPL, version 2 or at your option any
 * later version.  See COPYING for details.
 */

#include "options.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <libintl.h>
#include <stdlib.h>

#include "lpe.h"
#include "buffer.h"
#include "input.h"
#include "screen.h"
#include "cfg.h"
#include "strfuncs.h"
#include "exports.h"

/* A flag indicating a desire to quit the editor.  This is set whenever a
 * command should cause an exit.
 */
int quit;

/* Structure used to hold options read from the command line.  This is only
 * used to pass to the command line parser and read the results, so there's no
 * specific need to make the struct definition accessible outside of lpe.c
 *
 * fname: name of the file to open
 * mode: name of the preferred mode -- this exists even when dynamic modes are
 *       turned off, to make getting parameters right a little easier
 */
struct lpe_opts {
    char *fname;
    char *mode;
    int line;
};

/* free all buffers */
void free_bufs ()
{
    buffer *node;

    if (!the_buf)
        return;

    while (the_buf->next != the_buf && the_buf->next != NULL)
    {
        if (node)
	{
            node = the_buf->next;
	    if ( the_buf->next )
	    {
                the_buf->next = the_buf->next->next;
                the_buf->next->prev = the_buf;
            }
            free(node);
	}
    }
    free(the_buf);
}

/* Kill the editor with the specified error message.  This function does not
 * provide the user with any opportunity to save the buffer, and any changes
 * since the last save will be lost.
 */
void
die (char *s, ...)
{
    char buf[160];
    va_list args;

    cleanup_slang ();
    free_bufs ();

    sprintf (buf, "lpe: %s\n", s);

    va_start (args, s);
    vfprintf (stderr, buf, args);
    va_end (args);

    exit (1);
}

/* A function to handle refreshing the screen in certain situations, such as
 * when the user has just resumed lpe from a suspended session or the terminal
 * has just been resized.
 */
#ifndef __STRICT_ANSI__
static void
sig_handler (int signum)
{
    refresh_complete = 1;
}
#endif				/* ! __STRICT_ANSI__ */

/* Print a version message and terminate */
static void
version (void)
{
    printf (_("%s version %s\n"), PACKAGE, VERSION);
    exit (0);
}

/* Print the dedication and terminate */
static void
dedication (void)
{
    printf (_("The lpe text editor is dedicated to all those who died at\n"
	      "Columbine High School in Littleton, CO, USA on April 20, 1999.\n"
	      "We will always remember you.\n"));
    exit (0);
}

/* Print a usage message to standard error and terminate.  The hint parameter
 * provides a pointer to the argument that prompted this error, or is NULL if
 * the message was requested (eg, --help).  It's ignored for now, but most
 * likely won't be later.
 */
static void
usage (char *hint)
{
    if (hint != NULL)
	fprintf (stderr, _("error in option: %s\n"), hint);
    fprintf (stderr,
	     _
	     ("usage:\t'lpe [--mode mode] [--] file'\topens file for editing\n"
	      "      \t'lpe --version'              \tprints version number\n"
	      "      \t'lpe --help'                 \tprints this help message\n"));
    if (hint == NULL)
	exit (0);
    else
	exit (1);
}

/* Parse command line arguments to lpe */
static void
parse_args (int argc, char *argv[], struct lpe_opts *opts)
{
    int i;

    /* Initialize the options so they don't have leftover values */
    opts->fname = NULL;
    opts->mode = NULL;

    for (i = 1; i < argc; i++)
    {
	if (argv[i][0] == '-' || argv[i][0] == '+')
	{
	    if (strcmp (argv[i], "--version") == 0 ||
		strcmp (argv[i], "-V") == 0)
		version ();
	    if (strcmp (argv[i], "--dedication") == 0)
		dedication ();
	    if (strcmp (argv[i], "--help") == 0 ||
		strcmp (argv[i], "-h") == 0)
		usage (NULL);
	    if (strcmp (argv[i], "--rcfile") == 0 ||
		strcmp (argv[i], "-F") == 0)
	    {
		if (argc == i + 1)
		    usage (argv[i]);
		LPE_CONFIG_FILE = argv[++i];
		continue;
	    }
	    if (strcmp (argv[i], "--mode") == 0 ||
		strcmp (argv[i], "-m") == 0)
	    {
		if (argc == i + 1)
		    usage (argv[i]);
		opts->mode = argv[++i];
		continue;
	    }
	    
	    if (argv[i][0] ==  '+')
	    {
		if(!argv[i][1])
		opts->line = strtol(argv[++i],NULL,0);
		else
		opts->line = strtol(argv[i]+1,NULL,0);
		opts->line--;
		continue;
	    }

	    if (strcmp (argv[i], "--") == 0)
		break;
	    usage (argv[i]);
	} else
	{
	    if (i + 1 != argc)
	    {
		usage (argv[i + 1]);
	    } else
		opts->fname = argv[i];
	}
    }

    i++;

    while (i < argc)
    {
	/* The user indicated a desire to treat future arguments as files, so
	 * * don't parse any options from here on out. */
	if (i + 1 != argc)
	{
	    usage (argv[i + 1]);
	} else
	    opts->fname = argv[i];

	i++;
    }
}

/* Application entry point */
int
main (int argc, char *argv[])
{
    struct lpe_opts opts;
    buffer *node;

#ifndef __STRICT_ANSI__
    struct sigaction sa;
    memset (&sa, 0, sizeof (sa));

    sa.sa_handler = sig_handler;
    sigaction (SIGCONT, &sa, NULL);
    sigaction (SIGWINCH, &sa, NULL);
#endif				/* ! __STRICT_ANSI__ */

    setlocale (LC_ALL, "");
    bindtextdomain (PACKAGE, LOCALEDIR);
    textdomain (PACKAGE);

    if (getenv ("HOME"))
    {
	LPE_CONFIG_FILE =
	    (char *) malloc (strlen (getenv ("HOME")) +
			     strlen ("/.lpe/custom") + 1); /* space for trailing null */
	LPE_CONFIG_FILE[0] = '\0';
	strcat (LPE_CONFIG_FILE, getenv ("HOME"));
	strcat (LPE_CONFIG_FILE, "/.lpe/custom");
    } else
    {
	LPE_CONFIG_FILE = g_strdup (".lperc");
    }

    memset (&opts, 0, sizeof (struct lpe_opts));
    parse_args (argc, argv, &opts);

    if ((opts.fname == NULL) && (isatty(0))) usage(NULL);

    init_slang ();
    init_slang_keys ();
    export_all ();

    cfg_init ();

    the_buf = malloc(sizeof(buffer));
    if (the_buf == NULL) die("Out of memory");
    memset(the_buf, 0, sizeof(buffer));

    if (open_buffer (the_buf, opts.fname, opts.mode) == -1)
	die (strerror (errno));

    the_buf->prev = the_buf->next = the_buf;

    killbuf = NULL;
    is_killing = 0;

    if (the_buf->mode.enter)
	(*the_buf->mode.enter) (the_buf);

    if(opts.line > 0) {
        int line_number = opts.line;
        /* move cursor down */
        while(opts.line && the_buf->pos.line->next) {
            the_buf->pos.line = the_buf->pos.line->next;
            the_buf->linenum++;
            opts.line--;
        }
        check_scrolling(the_buf);
        if(!opts.line) {
            /* attempt to center the selected line */
            int centerint = SLtt_Screen_Rows >> 1;
            buf_line *centerline = the_buf->pos.line;
            if(centerint < line_number) {
                if(line_number < SLtt_Screen_Rows)
                    centerint -= (SLtt_Screen_Rows - line_number);
                while(centerint-- && centerline && the_buf->scrollpos->next) {
                    centerline = centerline->next;
                    the_buf->scrollpos = the_buf->scrollpos->next;
                    the_buf->scrollnum++;
                }
            }
	}
	refresh_text = 1;
	refresh_banner = 1;
    }

    while (!quit)
	process_input (the_buf);
    if (the_buf->mode.leave)
	(*the_buf->mode.leave) (the_buf);

    node = the_buf;

    do {
        close_buffer(node);
	if (node)
            node = node->next;
	else
	    break;
    } while (node != the_buf);

    free_bufs();
    cleanup_slang ();
    return 0;
}
