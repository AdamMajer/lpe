/* input.h
 *
 * Copyright (c) 1999 Chris Smith
 *
 * This file is distributed under the GPL, version 2 or at your option any
 * later version.  See COPYING for details.
 */

#ifndef LPE_INPUT_H
#define LPE_INPUT_H

#include SLANG_H
#include "buffer.h"

/* Constants to represent operations that can be bound to various keys */
#define LPE_ERR_KEY       SL_KEY_ERR
#define LPE_UP_KEY        SL_KEY_UP
#define LPE_DOWN_KEY      SL_KEY_DOWN
#define LPE_LEFT_KEY      SL_KEY_LEFT
#define LPE_RIGHT_KEY     SL_KEY_RIGHT
#define LPE_PGUP_KEY      SL_KEY_PPAGE
#define LPE_PGDN_KEY      SL_KEY_NPAGE
#define LPE_HOME_KEY      SL_KEY_HOME
#define LPE_END_KEY       SL_KEY_END
#define LPE_BACKSPACE_KEY SL_KEY_BACKSPACE
#define LPE_DELETE_KEY    SL_KEY_DELETE
#define LPE_ENTER_KEY     SL_KEY_ENTER
#define LPE_OPEN_KEY      0x1001
#define LPE_SAVE_KEY      0x1002
#define LPE_SAVE_ALT_KEY  0x1003
#define LPE_EXIT_KEY      0x1004
#define LPE_KILL_KEY      0x1005
#define LPE_YANK_KEY      0x1006
#define LPE_SEARCH_KEY    0x1007
#define LPE_READF_KEY     0x1008
#define LPE_GOTOLN_KEY    0x1009
#define LPE_CLEARMOD_KEY  0x100A
#define LPE_BUF_START_KEY 0x100B
#define LPE_BUF_END_KEY   0x100C
#define LPE_TAB_SWAP_KEY  0x100D
#define LPE_REFRESH_KEY   0x100E
#define LPE_SUSPEND_KEY   0x100F
#define LPE_FIND_NEXT_KEY 0x1010
#define LPE_REP_KEY       0x1011
#define LPE_REP_QUAD_KEY  0x1012
#define LPE_NEXT_WORD_KEY 0x1013
#define LPE_PREV_WORD_KEY 0x1014
#define LPE_SET_MODE_KEY  0x1018
#define LPE_AI_TOGGLE_KEY 0x1015
#define LPE_RECORDER_KEY  0x1016
#define LPE_PLAYBACK_KEY  0x1017
#define LPE_SHELL_KEY     0x1019
#define LPE_AWK_KEY       0x101A
#define LPE_SED_KEY       0x101B
#define LPE_SHELL_LN_KEY  0x101C
#define LPE_AWK_LN_KEY    0x101D
#define LPE_SED_LN_KEY    0x101E
#define LPE_HELP_KEY      0x101F
#define LPE_NEXTBUF_KEY   0x1020
#define LPE_PREVBUF_KEY   0x1021
#define LPE_OPENBUF_KEY   0x1022
#define LPE_CLOSEBUF_KEY  0x1023

#define LPE_SLANG_KEY     0x1024

#define LPE_DEBUG_KEY     0x1FFF

/* global variables holding the value of the command repeater */
extern int public_repeat_count;

/* utility functions of potential use to modularized functions */
int next_mult(int now, int spacing);
int check_scrolling(buffer *buf);
void check_col(buffer *buf);
void set_scr_col(buffer *buf);

/* prototypes for input handling functions */
void process_input(buffer *buf);

#endif /* LPE_INPUT_H */
