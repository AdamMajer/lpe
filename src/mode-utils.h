/* mode-utils.h
 *
 * Copyright (c) 1999 Chris Smith
 *
 * Author:
 *  Gergely Nagy <algernon@debian.org>
 *
 * This file is distributed under the GPL, version 2 or at your option any
 * later version.  See COPYING for details.
 */

#ifndef LPE_MODE_UTILS_H
#define LPE_MODE_UTILS_H

#include "cfg.h"
#include "lpecomm.h"

/**
 * mode_util_accept_extensions: accept only the specified extensions
 * @extension: the suffix (extension) of the filename
 * @case_sensitive: non-0 if the check should be case sensitive
 * @cnt: number of extensions specified as optional arguments
 * @optional_arguments: the extensions we want to accept
 *
 * This function makes it easy to check for filename suffixes, and if it is
 * in a list, then accept it.
 *
 * Returns 1 on success, 0 otherwise
 **/
int mode_util_accept_extensions ( char *extension, int case_sensitive, int cnt, ... );

/**
 * mode_util_accept_on_request: accept a file on request
 * @line: the line in which we perform the search
 * @case_sensitive: non-0 if the check should be case sensitive
 * @cnt: number of modes we specified as optional arguments
 * @optional_arguments: the list of modes we want to accept
 *
 * This function searches for -*- [mode] -*- and -*- Mode: [mode] -*- substrings
 * in a given line.
 *
 * Returns 1 on success, 0 otherwise
 **/
int mode_util_accept_on_request ( char *line, int case_sensitive, int cnt, ... );

void mode_util_set_slang_color ( char *mode, char *color_id,
				 int sl_color_id, char *def_fg,
				 char *def_bg );

void mode_util_set_options ( buffer *buf, char *mode, int hardtab,
			     int autoindent, int offerhelp, int highlight,
			     int flashbrace );

#define mode_util_get_option(mode,id) \
	cfg_get_option_int ( mode, "general", id )
#define mode_util_get_option_with_default(mode,id,def) \
	cfg_get_option_int_with_default ( mode, "general", id, def )

#endif /* ! LPE_MODE_UTILS_H */
