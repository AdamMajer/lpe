/* highlight.h
 *
 * Copyright (c) 1999 Chris Smith
 *
 * This file is distributed under the GPL, version 2 or at your option any
 * later version.  See COPYING for details.
 */

#ifndef LPE_HIGHLIGHT_H
#define LPE_HIGHLIGHT_H

#include "options.h"

/* These are some standard colors that should be used when appropriate by
 * syntax highlighters for various modules for programming languages.  Modes
 * may, of course, define their own custom colors -- but these should be used
 * if they apply for the benefit of users attempting to customize coloring.
 *
 * Modes that include highlight.h for standard colors should use palette
 * entries starting at 128 for any extensions.  This ensures that even as the
 * number of standard entries increases, there won't be any conflict between
 * palette indexes.
 */

/* Used for plain identifiers, such as variable or function names.
 */
#define COLOR_IDENT         0
#define COLOR_IDENT_FG      "lightgray"
#define COLOR_IDENT_BG      "black"

/* Used for any non-word symbols that are not covered by another special case
 * defined elsewhere.
 */
#define COLOR_SYMBOL        1
#define COLOR_SYMBOL_FG     "white"
#define COLOR_SYMBOL_BG     "black"

/* Used for symbols that must match, as in braces.  They are highlighted so as
 * to make the structure of a line clear.
 */
#define COLOR_BRACE         2
#define COLOR_BRACE_FG      "yellow"
#define COLOR_BRACE_BG      "black"

/* Used for anything that is ignored by the compiler or interpreter for the
 * language.
 */
#define COLOR_COMMENT       3
#define COLOR_COMMENT_FG    "green"
#define COLOR_COMMENT_BG    "black"

/* Used for 'reserved words': any word that holds special meaning, such as
 * keywords, boolean constants, etc.
 */
#define COLOR_KEYWORD       4
#define COLOR_KEYWORD_FG    "cyan"
#define COLOR_KEYWORD_BG    "black"

/* Used for anything that is interpreted by a preprocessor -- this is somewhat
 * specific to C/C++, but I'm putting it here because perlmode also uses a
 * preprocessor color.
 */
#define COLOR_PREPROC       5
#define COLOR_PREPROC_FG    "brown"
#define COLOR_PREPROC_BG    "black"

/* Used for string constants of any type.
 */
#define COLOR_STRING        6
#define COLOR_STRING_FG     "brightblue"
#define COLOR_STRING_BG     "black"

/* Used for numerical constants of any type.
 */
#define COLOR_NUMBER        7
#define COLOR_NUMBER_FG     "brightblue"
#define COLOR_NUMBER_BG     "black"

/* Used for escaped characters in a string constant, to make it clear what a
 * '\' character applies to.
 */
#define COLOR_STRINGE       8
#define COLOR_STRINGE_FG    "blue"
#define COLOR_STRINGE_BG    "black"

/* Used for anything that the compiler will choke on at the lexical level, such
 * as a malformed numeric constant.
 */
#define COLOR_ILLEGAL       9
#define COLOR_ILLEGAL_FG    "red"
#define COLOR_ILLEGAL_BG    "black"

/* Used to indicate an internal error within the syntax highlighter.  The
 * appearance of this color is always a bug.
 */
#define COLOR_DEBUG         10
#define COLOR_DEBUG_FG      "yellow"
#define COLOR_DEBUG_BG      "red"

#endif /* LPE_HIGHLIGHT_H */
