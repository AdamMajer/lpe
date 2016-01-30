/* lpecomm.h
 *
 * Copyright (c) 1999 Chris Smith
 *
 * This file is distributed under the GPL, version 2 or at your option any
 * later version.  See COPYING for details.
 */

#ifndef LPE_LPECOMM_H
#define LPE_LPECOMM_H

#include "options.h"
#include <string.h>
#include <slang.h>

/* forward decls */
struct _buffer;
struct _buf_line;

/* A mode for a buffer.  Modes are used to define behaviors that are specific
 * to particular programming languages or situations.  Modes consist of
 * function pointers, which can point to code to do a specific task, or can be
 * NULL to indicate that a task should not be performed.
 *
 * init: initializes the buffer according to conventions of the mode
 */
typedef struct _buf_mode
{
    void *handle;
    void *data;

    void (*init)(struct _buffer *);
    void (*uninit)(struct _buffer *);
    void (*enter)(struct _buffer *);
    void (*leave)(struct _buffer *);
    void (*extkey)(struct _buffer *, int ch);
    int (*flashbrace)(struct _buffer *);
    int (*highlight)(struct _buffer *, struct _buf_line *, int, int *, int *);
    int (*indent)(struct _buffer *, char ch);
} buf_mode;

/* A line in the file, represented as a doubly linked list node.
 *
 * txt_len: length of allocated space for the line's text
 * txt: pointer to a string representing the text of this line
 * next: pointer to the next line
 * prev: pointer to the previous line
 */
typedef struct _buf_line
{
    unsigned int txt_len;
    char *txt;
    struct _buf_line *next;
    struct _buf_line *prev;
    unsigned int start_state;
} buf_line;

/* A position in the buffer
 *
 * line: a pointer to the line node for the position
 * col: a numerical index into the text of that line
 */
typedef struct _buf_pos
{
    buf_line *line;
    int col;
} buf_pos;

/* The buffer itself, with all relevant info
 *
 * text: the head of the linked list of lines
 * scrollpos: pointer to the first visible line on the screen
 * scrollnum: line number of the first visible line
 * scrollcol: column number (on the screen) of the first visible column
 * pos: the current cursor position in the text of the buffer
 * scr_col: the cursor column on the screen itself
 * linenum: the line number of the current line
 * preferred_col: the index on the screen of the "preferred" column
 * fname: full filename of the file being edited
 * name: short name (aka, basename) of the file being edited
 * rdonly: flag indicating if the buffer is read-only
 * modified: flag indicating if the buffer has been modified since last saved
 * hardtab: flag indicating whether to insert tabs or simulate them w/ spaces
 * mode: in case of dynamic modes, the active buffer mode
 */
typedef struct _buffer
{
    buf_line *text;
    buf_line *scrollpos;
    int scrollnum;
    int scrollcol;
    buf_pos pos;
    int scr_col;
    int linenum;
    int preferred_col;
    char *fname, *name;
    int fname_valid;
    int rdonly;
    int modified;
    char *mode_name;
    buf_mode mode;
    buf_line *state_valid;
    int state_valid_num;
    
    int hardtab;
    int autoindent;
    int offerhelp;
    int highlight;
    int flashbrace;
    int usecrlf;
    
    int break_hard_links; /* 1 if save will break hard links. 0 is default */
    
    struct _buffer *next;
    struct _buffer *prev;
} buffer;

/* A macro to calculate the length of the line referred to by a buf_pos.  This
 * is just a little more efficient for long lines, since it takes into account
 * that the line is known to be at least as long as the current column.
 */
#define LINELEN(pos) (strlen((pos).line->txt + pos.col) + pos.col)

int is_control(char c);
int next_mult(int now, int spacing);
int check_scrolling(buffer *buf);
void check_col(buffer *buf);
void set_scr_col(buffer *buf);
int add_char(int c, buffer *buf);
void del_char(buffer *buf);
void def_indent(buffer *buf, char ch);

#endif /* LPE_LPECOMM_H */
