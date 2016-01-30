/* mode-utils.c
 *
 * Copyright (c) 1999 Chris Smith
 *
 * Author:
 *  Gergely Nagy <algernon@debian.org>
 *
 * This file is distributed under the GPL, version 2 or at your option any
 * later version.  See COPYING for details.
 */

#include <config.h>
#include "mode-utils.h"
#include "strfuncs.h"

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include <slang.h>

static int _mode_util_accept_one (char *line, char *mode);

int
mode_util_accept_extensions (char *extension, int case_sensitive, int cnt,
			     ...)
{
    va_list el;
    char *ext;
    int i;

    va_start (el, cnt);
    for (i = 0; i < cnt; i++)
    {
	ext = va_arg (el, char *);
	if (case_sensitive)
	{
	    if (!strcmp (extension, ext))
	    {
		va_end (el);
		return 1;
	    }
	} else
	{
	    if (!g_strcasecmp (extension, ext))
	    {
		va_end (el);
		return 1;
	    }
	}
    }
    va_end (el);
    return 0;
}

int
mode_util_accept_on_request (char *line, int case_sensitive, int cnt, ...)
{
    va_list modelist;
    char *c_mode;
    char *cs_line;
    int i;

    if (!line || (line && strlen (line) == 0))
	return 0;

    cs_line = g_strdup (line);
    if (!case_sensitive)
	g_strdown (cs_line);

    va_start (modelist, cnt);
    for (i = 0; i < cnt; i++)
    {
	c_mode = va_arg (modelist, char *);
	if (_mode_util_accept_one (cs_line, c_mode))
	{
	    free (cs_line);
	    va_end (modelist);
	    return 1;
	}
    }
    va_end (modelist);
    free (cs_line);
    return 0;
}

void
mode_util_set_slang_color (char *mode, char *color_id,
			   int sl_color_id, char *def_fg, char *def_bg)
{
    char *fg, *bg;

    fg =
	cfg_get_option_string_with_default (mode, "color", color_id,
					    def_fg);
    bg =
	cfg_get_option_string_with_default (mode, "background", color_id,
					    def_bg);

    if (fg && !strcmp (fg, "transparent"))
	fg = NULL;
    if (bg && !strcmp (bg, "transparent"))
	bg = NULL;

    SLtt_set_color (sl_color_id, NULL, fg, bg);
}

void
mode_util_set_options (buffer * buf, char *mode, int hardtab,
		       int autoindent, int offerhelp, int highlight,
		       int flashbrace)
{
    buf->hardtab =
	mode_util_get_option_with_default (mode, "hardtab", hardtab);
    buf->autoindent =
	mode_util_get_option_with_default (mode, "autoindent", autoindent);
    buf->offerhelp =
	mode_util_get_option_with_default (mode, "offerhelp", offerhelp);
    buf->highlight =
	mode_util_get_option_with_default (mode, "highlight", highlight);
    buf->flashbrace =
	mode_util_get_option_with_default (mode, "flashbrace", flashbrace);
}

static int
_mode_util_accept_one (char *line, char *mode)
{
    char *substring;
    char *tmp = NULL;

    if ((tmp = strstr (line, "-*-")) == NULL)
	return 0;

    substring = g_strdup (tmp + 3);
    if (!substring)
	return 0;
    if (!strstr(substring, "-*-"))
    	return 0;

    substring[strlen (substring) - strlen (strstr (substring, "-*-"))] = '\0';

    if ((tmp = strstr (substring, mode)) != NULL)
    {
	/* 
	 * Check if it's not a false alarm...
	 * If the chare before mode is one of ":;- " AND
	 * the one after it is one of ";- ", then we've
	 * found it.
	 */
	tmp--;
	if (strchr (": ;-", tmp[0])
	    && strchr ("; -", tmp[strlen (mode) + 1]))
	{
	    free (substring);
	    return 1;
	}
    }
    free (substring);
    return 0;
}
