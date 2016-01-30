/* screen.h
 *
 * Copyright (c) 1999 Chris Smith
 *
 * This file is distributed under the GPL, version 2 or at your option any
 * later version.  See COPYING for details.
 */

#ifndef LPE_SCREEN_H
#define LPE_SCREEN_H

#include <signal.h>
#include "buffer.h"

/* Flags indicating pieces of the screen to update.  These are used to group
 * screen updates after a series of operations on the buffer.  Since these are
 * designed to be set from anywhere, they need to be of type sig_atomic_t if
 * the target is a UNIX system.  Otherwise, autoconf defines them to be ints.
 */
extern volatile sig_atomic_t refresh_complete;
extern volatile sig_atomic_t refresh_text;
extern volatile sig_atomic_t refresh_banner;

/* A quick hack to make the minibuf work as expected */
int get_minibuf_col(void);

/* Prototypes for drawing functions */
void init_slang(void);
void init_slang_keys(void);
void cleanup_slang(void);
void draw_screen(buffer *buf);

#endif /* LPE_SCREEN_H */
