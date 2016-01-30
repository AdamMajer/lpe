/* options.h
 *
 * Copyright (c) 1999 Chris Smith
 *
 * This file is distributed under the GPL, version 2 or at your option any
 * later version.  See COPYING for details.
 */

#ifndef LPE_OPTIONS_H
#define LPE_OPTIONS_H

#include "config.h"

#ifdef HAVE_LIMITS_H
#define LIMITS_H "limits.h"
#elif defined(HAVE_SYS_SYSLIMITS_H)
#define LIMITS_H "sys/syslimits.h"
#endif

#define MAX_MACRO_KEYS       1024
#define MAX_MACRO_ANSWER_LEN 1024

/*
 * These are just defaults, they can be overridden from
 * configuration files.
 */
#define CMD_SH  "sh"
#define CMD_AWK "awk"
#define CMD_SED "sed"
#define DEF_MODULE_PATH "~/.lpe:" PLUGINDIR "/lpe"
#define DEF_HARD_TAB_WIDTH 8
#define DEF_SOFT_TAB_WIDTH 4
#define DEF_FLASH_TIME 3
#define DEF_SCROLL_GRAN 0
#define DEF_REALLOC_GRAN 10
#define DEF_EOF_FILL NULL

/*
 * Other things that are used in some places...
 */
char *LPE_CONFIG_FILE;

#endif /* LPE_OPTIONS_H */
