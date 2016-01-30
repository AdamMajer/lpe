/* lpe.h
 *
 * Copyright (c) 1999 Chris Smith
 *
 * This file is distributed under the GPL, version 2 or at your option any
 * later version.  See COPYING for details.
 */

#ifndef LPE_H
#define LPE_H

#include "i18n.h"

/*
 * This is bad. very bad. but I need PATH_MAX for -pedantic -ansi.
 * It is defined in linux/limits.h, but that's Linux only, and I
 * don't have access to other systems, so can't detect it in
 * configure...
 *  - Gergely Nagy
 */
#ifndef PATH_MAX
#define PATH_MAX 255
#endif

/* Flag to quit the application */
extern int quit;

/* Prototypes for functions in lpe.c */
void die(char *s, ...);

#endif /* LPE_H */
