/* cfg-core.c
 *
 * Copyright (c) 2000 Chris Smith
 *
 * Author:
 *  Gergely Nagy <algernon@debian.org>
 *
 * This file is distributed under the GPL, version 2 or at your option any
 * later version.  See COPYING for details.
 */

#include <config.h>
#include "cfg-core.h"
#include "strfuncs.h"

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

#ifdef HAVE_STRING_H
#include <string.h>
#endif

typedef enum {
    LPE_OPT_STRING,
    LPE_OPT_INT,
    LPE_OPT_BOOL
} LpeOptionType;

typedef struct _LpeOption LpeOption;
struct _LpeOption {
    LpeOptionType type;
    union {
	char *string;
	long integer;
	int boolean;
    } value;
};

LpeOption **LpeOptionTable;
char *LpeOptionHash;
long LpeOptionTableSize;

static LpeOption *cfg_core_get_option (char *name);
static void cfg_core_set_option (char *name, LpeOption * value);

LpeOption *
cfg_core_get_option (char *name)
{
    char *t_hentry;
    char *t_sstring;
    unsigned int pos = 0;
    LpeOption *opt = NULL;

    /* 
     * If the lookup table is not initialized yet,
     * we don't perform any lookup.
     */
    if (!LpeOptionHash)
	return NULL;

    /* 
     * We search for "\n[name]=" in the lookup table...
     */
    t_sstring = (char *) alloca (strlen (name) + 5);
    sprintf (t_sstring, "\n%s=", name);
    if ((t_hentry = strstr (LpeOptionHash, t_sstring)) != NULL)
    {
	/* 
	 * All right, we found it!
	 * Now extract the position from it...
	 */
	strcat (t_sstring, "%d\n");
	sscanf (t_hentry, t_sstring, &pos);
	opt = LpeOptionTable[pos];
    }
    /* 
     * Well... this is not really good...
     * export MALLOC_CHECK_=2 and it aborts right here
     */
    /* free ( t_sstring ); */
    return opt;
}

void
cfg_core_set_option (char *name, LpeOption * value)
{
    LpeOption *opt;
    char *hentry;
    char num[128];
    char *tmp;

    /* 
     * First of all, look up if it exists.
     */
    if ((opt = cfg_core_get_option (name)) != NULL)
    {
	if (opt->type == LPE_OPT_STRING)
	    free (opt->value.string);

	switch (value->type)
	{
	    case LPE_OPT_STRING:
		opt->value.string = value->value.string;
		break;
	    case LPE_OPT_INT:
		opt->value.integer = value->value.integer;
		break;
	    case LPE_OPT_BOOL:
		opt->value.boolean = value->value.boolean;
		break;
	}
	opt->type = value->type;
    } else
    {
	/* 
	 * Determine the position we'll put the new entry...
	 */
	LpeOptionTableSize++;
	LpeOptionTable =
	    realloc (LpeOptionTable,
		     sizeof (LpeOption) * (LpeOptionTableSize + 1));

	/* 
	 * Insert the entry
	 */
	/* sizeof(LpeOption), not sizeof(pointer)! */
	opt = (LpeOption *) malloc (sizeof (*value));
	switch (value->type)
	{
	    case LPE_OPT_STRING:
		opt->value.string = value->value.string;
		break;
	    case LPE_OPT_INT:
		opt->value.integer = value->value.integer;
		break;
	    case LPE_OPT_BOOL:
		opt->value.boolean = value->value.boolean;
		break;
	}
	opt->type = value->type;
	LpeOptionTable[LpeOptionTableSize] = opt;

	/* 
	 * And now insert a reference into the lookup table...
	 */
	sprintf (num, "%li", LpeOptionTableSize);
	hentry = (char *) malloc (strlen (name) + strlen (num) + 4);
	sprintf (hentry, "%s=%s\n", name, num);
	tmp =
	    (char *) malloc (strlen (LpeOptionHash) + strlen (hentry) +
			     10);
	tmp[0] = '\0';
	strcat (tmp, LpeOptionHash);
	strcat (tmp, hentry);
	free (LpeOptionHash);
	LpeOptionHash = tmp;
	free (hentry);
    }
}

/********************
 * Public functions *
 ********************/
void
cfg_core_init (void)
{
    LpeOptionTable = NULL;
    LpeOptionHash = g_strdup ("\n");
    LpeOptionTableSize = -1;
}

void
cfg_core_set_int (char *name, long value)
{
    LpeOption *opt;

    opt = (LpeOption *) malloc (sizeof (LpeOption));
    opt->type = LPE_OPT_INT;
    opt->value.integer = value;
    cfg_core_set_option (name, opt);
    free (opt);
}

void
cfg_core_set_bool (char *name, int value)
{
    LpeOption *opt;

    opt = (LpeOption *) malloc (sizeof (LpeOption));
    opt->type = LPE_OPT_BOOL;
    opt->value.boolean = (value <= 0) ? 0 : 1;
    cfg_core_set_option (name, opt);
    free (opt);
}

void
cfg_core_set_str (char *name, char *value)
{
    LpeOption *opt;

    opt = (LpeOption *) malloc (sizeof (LpeOption));
    opt->type = LPE_OPT_STRING;
    opt->value.string = g_strdup (value);
    cfg_core_set_option (name, opt);
    free (opt);
}

long
cfg_core_get_int (char *name)
{
    LpeOption *opt;

    opt = cfg_core_get_option (name);
    if (opt)
    {
	switch (opt->type)
	{
	    case LPE_OPT_INT:
		return opt->value.integer;
		break;
	    case LPE_OPT_BOOL:
		return (opt->value.boolean <= 0) ? 0 : 1;
		break;
	    case LPE_OPT_STRING:
		return atol (opt->value.string);
		break;
	}
    }
    return -1;
}

char *
cfg_core_get_str (char *name)
{
    LpeOption *opt;
    char *str;

    opt = cfg_core_get_option (name);
    if (opt)
    {
	switch (opt->type)
	{
	    case LPE_OPT_STRING:
		return g_strdup ( opt->value.string );
		break;
	    case LPE_OPT_BOOL:
		return (opt->value.boolean <= 0) ? NULL : "";
		break;
	    case LPE_OPT_INT:
		str = (char *) malloc (65);
		sprintf (str, "%li", opt->value.integer);
		return str;
		break;
	}
    }
    return NULL;
}

int
cfg_core_get_bool (char *name)
{
    LpeOption *opt;

    opt = cfg_core_get_option (name);
    if (opt)
    {
	switch (opt->type)
	{
	    case LPE_OPT_INT:
		return (opt->value.integer <= 0) ? 0 : 1;
		break;
	    case LPE_OPT_BOOL:
		return (opt->value.boolean <= 0) ? 0 : 1;
		break;
	    case LPE_OPT_STRING:
		return (atol (opt->value.string) <= 0) ? 0 : 1;
		break;
	}
    }
    return -1;
}

/*
 * This one is intended to be exported to SLang
 */
void
cfg_core_set_any (char *name, int type, ...)
{
    va_list arglist;

    va_start (arglist, type);
    switch (type)
    {
	case 0:
	    cfg_core_set_str (name, va_arg (arglist, char *));
	    break;
	case 1:
	    cfg_core_set_int (name, va_arg (arglist, long));
	    break;
	case 2:
	    cfg_core_set_bool (name, va_arg (arglist, int));
	    break;
    }
}

void
cfg_core_destroy (void)
{
    int i;

    for (i = 0; i < LpeOptionTableSize; i++)
    {
	if (LpeOptionTable[i]->type == LPE_OPT_STRING)
	    free (LpeOptionTable[i]->value.string);

	free (LpeOptionTable[i]);
    }
    free (LpeOptionTable);
    free (LpeOptionHash);

    LpeOptionHash = NULL;
    LpeOptionTable = NULL;
    LpeOptionTableSize = -1;
}
