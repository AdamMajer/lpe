/* cfg.c
 *
 * Copyright (c) 2000 Chris Smith
 *
 * Author:
 *  Gergely Nagy <algernon@debian.org>
 *
 * This file is distributed under the GPL, version 2 or at your option any
 * later version.  See COPYING for details.
 */

#include <config.h>
#include "strfuncs.h"
#include "options.h"
#include "cfg-core.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include SLANG_H
#ifdef HAVE_STRING_H
#include <string.h>
#endif

int cfg_errno;

void
cfg_init (void)
{
    cfg_core_init ();

    SLadd_intrinsic_variable ("LPE_CONFIG_FILE", &LPE_CONFIG_FILE,
			      SLANG_STRING_TYPE, 1);

    if (access (DATADIR "/lpe/init.sl", R_OK) == 0)
    {
	if (SLang_load_file (DATADIR "/lpe/init.sl") == -1)
	{
	    SLang_restart (1);
	    SLang_set_error(0);
	}
    }
}

char *
cfg_get_option_string (char *mode, char *section, char *option)
{
    char *id;
    char *result;

    id =
	(char *) malloc (strlen (mode) + strlen (section) +
			 strlen (option) + 4);
    sprintf (id, "%s.%s.%s", mode, section, option);

    result = cfg_core_get_str (id);
    free (id);
    return result;
}

char *
cfg_get_option_string_with_default (char *mode, char *section,
				    char *option, char *def)
{
    char *val;

    val = cfg_get_option_string (mode, section, option);

    if ((!val && !cfg_errno) || (val && strlen (val) == 0))
    {
	val = cfg_get_option_string ("global", section, option);
	if ((!val && !cfg_errno) || (val && strlen (val) == 0))
	{
	    val = cfg_get_option_string ("global", section, "default");
	    if ((!val && !cfg_errno) || (val && strlen (val) == 0))
	    {
		if (def)
		    return g_strdup (def);
		else
		    return NULL;
	    }
	}
    }

    if (val && !strcmp (val, "NULL"))
	return NULL;
    else
	return val;

}

int
cfg_get_option_int (char *mode, char *section, char *option)
{
    char *id;
    int result;

    id =
	(char *) malloc (strlen (mode) + strlen (section) +
			 strlen (option) + 4);
    sprintf (id, "%s.%s.%s", mode, section, option);

    result = cfg_core_get_int (id);
    free (id);
    return result;
}

int
cfg_get_option_int_with_default (char *mode,
				 char *section, char *option, int def)
{
    int val;

    val = cfg_get_option_int (mode, section, option);

    if (val < 0
	&& (val = cfg_get_option_int ("global", section, option)) < 0)
    {
	val = cfg_get_option_int ("global", section, "default");
	if (val < 0)
	    return def;
	else
	    return val;
    } else
	return val;
}
