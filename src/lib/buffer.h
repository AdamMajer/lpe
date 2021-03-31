/* buffer.h
 *
 * Copyright (c) 1999 Chris Smith
 *
 * This file is distributed under the GPL, version 2 or at your option any
 * later version.  See COPYING for details.
 */

#ifndef LPE_BUFFER_H
#define LPE_BUFFER_H

#include <stdio.h>
#include "lpecomm.h"

/* Some global variables.  See buffer.c for an explanation of these. */
extern buffer *the_buf;
extern buf_line *killbuf;
extern int is_killing;

/* Prototypes for functions defined in buffer.c */
void set_buf_mode(buffer *buf, char *reqname);
void free_list(buf_line *list);
buf_line *read_stream(FILE *fp);
int write_stream(FILE *fp, buf_line *start);
int open_buffer(buffer *buf, char *fname, char *modename);
void close_buffer(buffer *buf);
int save_buffer(buffer *buf);

#endif /* LPE_BUFFER_H */
