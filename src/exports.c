/* exports.c
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
#include "cfg-core.h"
#include "cfg.h"
#include "lpecomm.h"
#include "buffer.h"
#include "screen.h"
#include "mode-utils.h"

#include <unistd.h>

#include <slang.h>

static int
i_lpe_exists (char *filename)
{
    if (access (filename, R_OK) == 0)
	return 1;
    else
	return 0;
}

static void
i_lpe_reset (void)
{
    buffer *buff;

    buff = the_buf;

    if (buff->mode.leave)
	(*buff->mode.leave) (buff);
    if (buff->mode.enter)
	(*buff->mode.enter) (buff);

    if (buff->mode_name)
	mode_util_set_options (buff, buff->mode_name,
			       buff->hardtab || 0,
			       buff->autoindent || 1,
			       buff->offerhelp || 1,
			       buff->highlight || 0,
			       buff->flashbrace || 0);

    refresh_complete = 1;
}

static void
i_cfg_core_set_any (void)
{
    int rtype;
    char *string;
    char *name;
    int i;

    SLpop_string (&name);
    SLang_pop_integer (&rtype);

    switch (rtype)
    {
	case 1:
	case 2:
	    SLang_pop_integer (&i);
	    cfg_core_set_any (name, rtype, i);
	    break;
	case 0:
	    SLpop_string (&string);
	    cfg_core_set_any (name, rtype, string);
	    break;
    }
}

static SLang_Intrin_Fun_Type Exports[] = {
    MAKE_INTRINSIC_1 ("lpe_exists", i_lpe_exists, SLANG_INT_TYPE,
		      SLANG_STRING_TYPE),
    MAKE_INTRINSIC_0 ("lpe_cfg_reset", i_lpe_reset, SLANG_VOID_TYPE),
    MAKE_INTRINSIC_0 ("lpe_cfg_core_set_any", i_cfg_core_set_any,
		      SLANG_VOID_TYPE),
    MAKE_INTRINSIC_1 ("_lpe_cfg_core_get_str", cfg_core_get_str,
		      SLANG_STRING_TYPE,
		      SLANG_STRING_TYPE),
    SLANG_END_TABLE
};

void
export_all (void)
{
    SLang_init_all ();
    SLadd_intrin_fun_table (Exports, NULL);
    SLang_load_string ("define lpe_set_option ( option, value )"
		       "{"
		       "	if ( _typeof ( value ) == String_Type )"
		       "	{"
		       "		lpe_cfg_core_set_any ( value, 0, option );"
		       "	}"
		       "	else"
		       "	{"
		       "		lpe_cfg_core_set_any ( value, 1, option );"
		       "	}" "}");
}
