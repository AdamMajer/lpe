/* buffer.c
 *
 * Copyright (c) 1999 Chris Smith
 *
 * This file is distributed under the GPL, version 2 or at your option any
 * later version.  See COPYING for details.
 */

#include "options.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <dlfcn.h>
#include <unistd.h>
#include <sys/file.h>
#include <dirent.h>
#include <sys/stat.h>
#include <libgen.h>
#include LIMITS_H

#include "lpe.h"
#include "genmode.h"
#include "buffer.h"
#include "cfg.h"
#include "strfuncs.h"
#include "common.h"

#if !defined(HAVE_FLOCK) || defined(DUMMY_FLOCK)
#define flock(a,b)
#endif

/* This points to the current buffer in a circular doubly linked list of open
 * buffers.
 */
buffer *the_buf;

/* The killbuf -- a singly linked list (prev pointer is invalid and should be
 * always set to NULL) of lines that were deleted via the kill command.  These
 * can be inserted into the buffer using the yank command, so they must be
 * saved.  The kill buf is purposely built from the same linked list node
 * structure as the main buffer.  This is because it feels cleaner not to
 * require memory allocation to delete part of the buffer, so I didn't want to
 * have to allocate new nodes in the killbuf linked list.
 *
 * is_killing is a flag indicating if the last thing the user did was to kill
 * a line.  If so, killing a line now would add it to the end of the killbuf as
 * part of a killed block.  If not, killing a line would clear the killbuf and
 * insert the newly killed line.
 */
buf_line *killbuf;
int is_killing;

void
set_buf_mode (buffer * buf, char *reqname)
{
    DIR *dir;
    char name[PATH_MAX];
    char *pathpos, *nextcolon;
    void *handle = NULL;
    int found;

    if ((reqname) && (!strcmp (reqname, "genmode")))
    {
	buf->mode.handle = NULL;
	buf->mode.init = gen_init;
	buf->mode.uninit = NULL;
	buf->mode.enter = NULL;
	buf->mode.leave = NULL;
	buf->mode.extkey = NULL;
	buf->mode.flashbrace = NULL;
	buf->mode.highlight = NULL;
	buf->mode.indent = NULL;

	gen_init (buf);
	return;
    }

    pathpos = getenv ("LPE_MODULE_PATH");
    if (pathpos == NULL)
	pathpos =
	    cfg_get_global_string_with_default ("module_path",
						DEF_MODULE_PATH);
    found = 0;

    do
    {
	char *basename;
	struct dirent *ent;

	if (*pathpos == '~')
	{
	    char *home = getenv ("HOME");

	    strcpy (name, (home == NULL) ? "/" : home);
	    pathpos++;
	} else
	    name[0] = '\0';

	nextcolon = strchr (pathpos, ':');

	if (nextcolon != NULL)
	{
	    strncat (name, pathpos, nextcolon - pathpos);
	    pathpos += nextcolon - pathpos + 1;
	} else
	    strcpy (name, pathpos);

	basename = name + strlen (name);
	if (*(basename - 1) != '/')
	{
	    *(basename++) = '/';
	    *basename = '\0';
	}

	if (reqname != NULL)
	{
	    struct stat st;

	    strcpy (basename, reqname);
	    strcat (basename, ".so");
	    stat (name, &st);

	    if (S_ISREG (st.st_mode))
	    {
		handle = dlopen (name, RTLD_LAZY);
		if (handle != NULL)
		    found = 1;
	    }

	} else
	{
	    dir = opendir (name);
	    if (dir == NULL)
		continue;

	    while ((!found) && ((ent = readdir (dir)) != NULL))
	    {
		struct stat st;
		int (*accept) (buffer *);

		strcpy (basename, ent->d_name);
		stat (name, &st);

		if (!S_ISREG (st.st_mode))
		    continue;

		handle = dlopen (name, RTLD_LAZY);
		if (handle == NULL)
		    continue;

		accept = (int (*)(buffer *)) dlsym (handle, "mode_accept");
		if ((accept == NULL) || !(*accept) (buf))
		{
		    dlclose (handle);
		    continue;
		}

		found = 1;
	    }
	    closedir (dir);
	}
    }
    while ((!found) && (nextcolon != NULL));

    if (found)
    {
	buf->mode.handle = handle;
	buf->mode.data = NULL;
	buf->mode.init = (void (*)(buffer *)) dlsym (handle, "mode_init");
	buf->mode.uninit =
	    (void (*)(buffer *)) dlsym (handle, "mode_uninit");
	buf->mode.enter =
	    (void (*)(buffer *)) dlsym (handle, "mode_enter");
	buf->mode.leave =
	    (void (*)(buffer *)) dlsym (handle, "mode_leave");
	buf->mode.extkey =
	    (void (*)(buffer *, int)) dlsym (handle, "mode_extkey");
	buf->mode.flashbrace =
	    (int (*)(buffer *)) dlsym (handle, "mode_flashbrace");
	buf->mode.highlight =
	    (int (*)(buffer *, buf_line *, int, int *, int *))
	    dlsym (handle, "mode_highlight");
	buf->mode.indent =
	    (int (*)(buffer *, char)) dlsym (handle, "mode_indent");
    }

    else
    {
	buf->mode.handle = NULL;
	buf->mode.data = NULL;
	buf->mode.init = gen_init;
	buf->mode.uninit = NULL;
	buf->mode.enter = NULL;
	buf->mode.leave = NULL;
	buf->mode.extkey = NULL;
	buf->mode.flashbrace = NULL;
	buf->mode.highlight = NULL;
	buf->mode.indent = NULL;
    }

    if (buf->mode.init != NULL)
	(*buf->mode.init) (buf);
}

/* Frees an entire linked list of lines; used for error recovery when reading
 * a buffer.
 */
void
free_list (buf_line * lst)
{
    buf_line *iter;

    for (iter = lst; iter != NULL;)
    {
	buf_line *t;

	t = iter->next;
	free (iter->txt);
	free (iter);
	iter = t;
    }
}

/* Reads text from the specified stdio stream up to the end of stream, storing
 * the result in a linked list of buf_line.  It returns a pointer to the first
 * line on success, or NULL on failure.  If the function fails, errno will
 * remain set to the error, or to ENOMEM to indicate a failure to allocate
 * memory.
 */
buf_line *
read_stream (FILE * fp)
{
    buf_line *start;
    buf_pos pos;

    start = (buf_line *) malloc (sizeof (buf_line));
    if (start == NULL)
    {
	errno = ENOMEM;
	return NULL;
    }

    start->txt = (char *) malloc (1);
    if (start->txt == NULL)
    {
	free (start);
	errno = ENOMEM;
	return NULL;
    }

    start->txt[0] = '\0';
    start->txt_len = 1;

    start->prev = NULL;
    start->next = NULL;

    pos.line = start;
    pos.col = 0;

    while (!feof (fp))
    {
	int c;
	c = getc (fp);

	if (c == EOF)
	{
	    if (ferror (fp))
	    {
		/* There was an error reading from the file.  Clean up the *
		 * buffer so far and then report the error. */
		int err;

		err = errno;
		free_list (start);
		errno = err;

		return NULL;
	    }

	    pos.line->txt[pos.col] = '\0';
	}

	else if (c == '\n')
	{
	    buf_line *t;

	    pos.line->txt[pos.col] = '\0';
	    t = (buf_line *) malloc (sizeof (buf_line));
	    if (t == NULL)
	    {
		free_list (start);
		errno = ENOMEM;
		return NULL;
	    }

	    t->txt = (char *) malloc (1);
	    if (t->txt == NULL)
	    {
		free_list (start);
		free (t);
		errno = ENOMEM;
		return NULL;
	    }

	    pos.line->next = t;
	    pos.line->next->txt_len = 1;

	    pos.line->next->prev = pos.line;
	    pos.line->next->next = NULL;

	    pos.line = pos.line->next;
	    pos.col = 0;
	}

	else
	{
	    pos.line->txt[pos.col] = (char) c;
	    if (++pos.col == pos.line->txt_len)
	    {
		char *ntxt;

		ntxt = (char *) realloc (pos.line->txt,
					 pos.line->txt_len +
					 cfg_get_global_int_with_default
					 ("realloc_granularity",
					  DEF_REALLOC_GRAN));
		if (ntxt == NULL)
		{
		    free_list (start);
		    errno = ENOMEM;
		    return NULL;
		}

		pos.line->txt = ntxt;
		pos.line->txt_len +=
		    cfg_get_global_int_with_default ("realloc_granularity",
						     DEF_REALLOC_GRAN);
	    }
	}
    }

    return start;
}

/* Writes text from a linked list of buf_line to the specified stdio stream.
 * It returns 0 in success, or -1 on error.  If the function fails, errno will
 * remain set to the error, or to ENOMEM to indicate a failure to allocate
 * memory.
 */
int
write_stream (FILE * fp, buf_line * start)
{
    buf_line *curline;

    for (curline = start; curline != NULL;)
    {
	if (fputs (curline->txt, fp) == -1)
	    return -1;

	curline = curline->next;
	if (curline != NULL)
	{
	    if (putc ('\n', fp) == EOF)
		return -1;
	}
    }

    return 0;
}

/* Strips the trailing carriage returns from all lines in a buffer.  This make$+ * files written in DOS and Windows much more pleasant to edit and work with.
 * It only should be done if the user has not specified the 'keepcr' flag in
 * the configuration file.
 *
 * It returns zero if there are any lines not terminated by a carriage return,
 * excluding the last line.  It returns non-zero if all lines in the files end
 * in a carriage return.  The intent is that when the file is later saved, the
 * carriage returns can be replaced to preserve the file's original format.
 */
static int stripcr(buffer *buf)
{
       buf_line *curline;
       int ret = 1;

       for (curline = buf->text;
            curline->next != NULL;
            curline = curline->next)
       {
               int i;

               i = strlen(curline->txt);
               if (curline->txt[i - 1] == '\r')
                       curline->txt[i - 1] = '\0';
               else
                       ret = 0;
       }

       return ret;
}

/* Opens a file in the specified buffer, with the specified file name.  Loads
 * the file from disk and initializes all members of the buffer.  If the
 * filename is NULL, opens standard input.  If the modename is NULL, chooses
 * an appropriate mode by normal means.  Returns -1 on error or 0 on success.
 * In case of an error, if the error was in file I/O, errno is set to the
 * system error code.  If the error was out of memory, errno is explicitly set
 * to ENOMEM to indicate this fact.
 */
int
open_buffer (buffer * buf, char *fname, char *modename)
{
    FILE *fp;

    if (fname != NULL) fp = fopen(fname, "r");
    else fp = stdin;

    if (fp == NULL)
    {
	if (errno != ENOENT)
	    return -1;

	buf->text = (buf_line *) malloc (sizeof (buf_line));
	if (buf->text == NULL)
	{
	    errno = ENOMEM;
	    return -1;
	}

	buf->text->txt = (char *) malloc (1);
	if (buf->text->txt == NULL)
	{
	    free (buf->text);
	    errno = ENOMEM;
	    return -1;
	}

	buf->text->txt[0] = '\0';
	buf->text->txt_len = 1;

	buf->text->prev = NULL;
	buf->text->next = NULL;

	buf->rdonly = 0;
    }

    else
    {
	int err;

	flock (fileno (fp), LOCK_SH);

	buf->text = read_stream (fp);
	err = errno;

	if (access (fname, W_OK) == -1)
	    buf->rdonly = 1;
	else
	    buf->rdonly = 0;

	flock (fileno (fp), LOCK_UN);
	fclose (fp);

	if (buf->text == NULL)
	{
	    errno = err;
	    return -1;
	}
    }

    buf->modified = 0;

    buf->scrollpos = buf->text;
    buf->scrollnum = 0;
    buf->scrollcol = 0;
    buf->pos.line = buf->text;
    buf->pos.col = 0;
    buf->scr_col = 0;
    buf->preferred_col = 0;
    buf->linenum = 0;

    if (fname != NULL)
    {
        buf->fname = calloc(100, sizeof(char));
        strcpy(buf->fname, fname);
        buf->name = strrchr (buf->fname, '/');
        if (buf->name == NULL)
	    buf->name = buf->fname;
        else
	    ++buf->name;

	buf->fname_valid = 1;
    }
    else
    {
        buf->fname = strdup("(stdin)");
        buf->name = buf->fname;
        buf->fname_valid = 0;
    }

    buf->mode_name = NULL;
    set_buf_mode (buf, modename);

    return 0;
}

/* Close the specified buffer, cleaning up after it. */
void
close_buffer (buffer * buf)
{
    if (!buf)
        return;

    if (buf->mode.uninit)
	(*buf->mode.uninit) (buf);
    if (buf->mode.handle)
	dlclose (buf->mode.handle);

    while (buf->text != NULL)
    {
	buf_line *t;
	t = buf->text->next;
	free (buf->text->txt);
	free (buf->text);
	buf->text = t;
    }

    free(buf->fname);
}

/* Save the current buffer to disk, using the file name specified by the buf
 * structure.  Return 0 on success, -1 on failure with errno set to the error
 * responsible for failure.
 */
int
save_buffer (buffer * buf)
{
    FILE *fp;
    int ret, copied = 1;
    struct stat st;
    char *dir, *file, *bakname;

    /* If the previous file exists, copy it to a backup file before
     * writing the new file.  This means two things:
     * 1.  The previous file isn't destroyed if we somehow fail to write.
     * 2.  We copy-on-write hardlinks, rather than clobbering all instances of the inode.
     */
    ret = strlen( buf->fname ) / 4096 + 1;
    dir = (char*)malloc( sizeof(char) * 4096 * ret );
    while( getcwd( dir, 4096 * ret ) == 0 ){
        ret++;
        free( dir );
        dir = (char*)malloc( sizeof(char*) * 4096 * ret );
    }
    strcpy( dir, buf->fname );
    file = strdup( buf->fname );
    bakname = (char*)malloc( sizeof(char) * (strlen(dir) + strlen(file) + 7 ));
    sprintf(bakname,"%s/.%s.lpe",dirname(dir),basename(file));
    if(copy(buf->fname,bakname) != 0)
        copied = 0;
    fp = fopen (buf->fname, "w");
    if (fp == NULL) {
        if(copied)
            unlink(bakname);
        ret = -1;
        goto save_buffer_return;
    }
    flock (fileno (fp), LOCK_EX);

    ret = write_stream (fp, buf->text);

    flock (fileno (fp), LOCK_UN);
    fclose (fp);

    if (ret == 0) {
        buf->modified = 0;
        if(copied)
            unlink(bakname);
    } else {
        unlink(buf->fname);
        if(copied)
            rename(bakname,buf->fname); /* FIXME: Will loose all link information */
    }
save_buffer_return:
    free( dir );
    free( file );
    free( bakname );
    return ret;
}
