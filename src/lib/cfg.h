/* cfg.h
 *
 * Copyright (c) 2000 Chris Smith
 *
 * Author:
 *  Gergely Nagy <algernon@debian.org>
 *
 * This file is distributed under the GPL, version 2 or at your option any
 * later version.  See COPYING for details.
 */

#ifndef LPE_CFG_H
#define LPE_CFG_H

/**
 * cfg_init: load initializing SLang scripts
 *
 * This needs to be called only once. It loads the SLang scripts needed by
 * cfg_get_option and friends.
 *
 * Returns nothing.
 **/
void cfg_init ( void );

char *cfg_get_option_string ( char *mode, char *section,
				    char *option );
char *cfg_get_option_string_with_default ( char *mode, char *secion,
					   char *option, char *def );

int cfg_get_option_int ( char *mode, char *section, char *option );

int cfg_get_option_int_with_default ( char *mode, char *section,
				      char *option, int def );

#define cfg_get_global_int(id) \
	cfg_get_option_int ( "global", "general", id )
#define cfg_get_global_int_with_default(id,def) \
	cfg_get_option_int_with_default ( "global", "general", id, def )
#define cfg_get_global_string(id) \
	cfg_get_option_string ( "global", "general", id )
#define cfg_get_global_string_with_default(id,def) \
	cfg_get_option_string_with_default ( "global", "general", id, def )

#endif /* ! LPE_CFG_H */
