/* cfg-core.h
 *
 * Copyright (c) 2000 Chris Smith
 *
 * Author:
 *  Gergely Nagy <algernon@debian.org>
 *
 * This file is distributed under the GPL, version 2 or at your option any
 * later version.  See COPYING for details.
 */

#ifndef LPE_CFG_CORE_H
#define LPE_CFG_CORE_H

void cfg_core_init ( void );
void cfg_core_destroy ( void );

void cfg_core_set_int ( char *name, long value );
long cfg_core_get_int ( char *name );

void cfg_core_set_str ( char *name, char *value );
char* cfg_core_get_str ( char *name );

void cfg_core_set_bool ( char *name, int value );
int cfg_core_get_bool ( char *name );

void cfg_core_set_any ( char *name, int type, ... );

#endif /* ! LPE_CFG_CORE_H */
