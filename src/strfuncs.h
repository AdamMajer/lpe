/* strfuncs.c
 *
 * Copyright (c) 1999 Chris Smith
 *
 * Author:
 *  The GLib team, copied from glib 1.2.6 sourcetree
 *
 * This file is distributed under the GPL, version 2 or at your option any
 * later version.  See COPYING for details.
 */

#ifndef LPE_STRFUNCS_H
#define LPE_STRFUNCS_H

int g_strcasecmp ( const char *s1, const char *s2 );
int g_strncasecmp ( const char *s1, const char *s2, unsigned int n );
void g_strdown ( char *string );
void g_strup ( char *string );
char *g_strdup ( const char *string );
char *g_strndup ( const char *string, unsigned int n );

#endif /* ! LPE_STRFUNCS_H */
