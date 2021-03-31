/* minibuf.h
 *
 * Copyright (c) 1999 Chris Smith
 *
 * This file is distributed under the GPL, version 2 or at your option any
 * later version.  See COPYING for details.
 */

#ifndef LPE_MINIBUF_H
#define LPE_MINIBUF_H

#include "buffer.h"

/* A structure to represent the minibuf
 *
 * vis: flag indicating if the minibuf is visible
 * focus: flag indicating of the minibuf has keyboard focus
 * pos: the cursor position in the minibuf
 * scroll: the position of the leftmost visible character in the minibuf
 * startcol: the screen column at which the minibuf should start displaying
 * width: number of columns reserved for just the text part of the minibuf
 * compl: filename completion flags
 * label: the prompt when asking a question in the minibuf
 * text: the primary text -- either that typed or displayed
 */
typedef struct
{
    int vis;
    int focus;
    int pos;
    int scroll;
    int width;
    int compl;
    char label[50];
    char text[150];
} minibuf;

/* The global minibuf.  See minibuf.c for details */
extern minibuf the_mbuf;

/* Prototypes for the external interface to the minibuf */
char *mbuf_ask(buffer *buf, char *prompt, int compl);
void mbuf_tell(buffer *buf, char *msg);

/* Completion flags */

#define MBUF_NO_COMPL   0x0000
#define MBUF_FILE_COMPL 0x0004
#define MBUF_MODE_COMPL 0x0008

#define MBUF_PEND_COMPL 0x0001  /* Completion pending - last key was a '\t' */

#endif /* LPE_MINIBUF_H */
