/* genmode.c
 *
 * Copyright (c) 1999 Chris Smith
 *
 * This file is distributed under the GPL, version 2 or at your option any
 * later version.  See COPYING for details.
 */

#include "options.h"

#include "genmode.h"
#include "mode-utils.h"

void
gen_init (buffer * buf)
{
    if (buf->mode_name == NULL)
    {
	buf->hardtab =
	    mode_util_get_option_with_default ("genmode", "hardtab", 1);
	buf->autoindent =
	    mode_util_get_option_with_default ("genmode", "autoindent", 0);
	buf->offerhelp =
	    mode_util_get_option_with_default ("genmode", "offerhelp", 1);
	buf->highlight =
	    mode_util_get_option_with_default ("genmode", "highlight", 0);
	buf->flashbrace =
	    mode_util_get_option_with_default ("genmode", "flashbrace", 0);
    }

    buf->mode_name = "genmode";
}
