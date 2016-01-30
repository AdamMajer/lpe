/* strfuncs.c
 *
 * Copyright (c) 1999 Chris Smith
 *
 * Author:
 *  The GLib team, copied from glib 1.2.6 sourcetree
 *
 * This file is distributed under the GPL, version 2 or at your option any
 * later version.  See COPYING for details.
 */

#include <config.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <ctype.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif

int
g_strcasecmp (const char *s1, const char *s2)
{

#ifdef HAVE_STRCASECMP
    if (s1 == NULL || s2 == NULL)
	return 0;

    return strcasecmp (s1, s2);
#else

    int c1, c2;

    if (s1 == NULL || s2 == NULL)
	return 0;

    while (*s1 && *s2)
    {
	/* According to A. Cox, some platforms have islower's that * don't
	 * work right on non-uppercase */
	c1 = isupper ((char) *s1) ? tolower ((char) *s1) : *s1;
	c2 = isupper ((char) *s2) ? tolower ((char) *s2) : *s2;
	if (c1 != c2)
	    return (c1 - c2);
	s1++;
	s2++;
    }

    return (((int) (char) *s1) - ((int) (char) *s2));
#endif
}

int
g_strncasecmp (const char *s1, const char *s2, unsigned int n)
{
#ifdef HAVE_STRNCASECMP
    return strncasecmp (s1, s2, n);
#else
    int c1, c2;

    if (s1 == NULL || s2 == NULL)
	return 0;

    while (n-- && *s1 && *s2)
    {
	/* According to A. Cox, some platforms have islower's that * don't
	 * work right on non-uppercase */
	c1 = isupper ((char) *s1) ? tolower ((char) *s1) : *s1;
	c2 = isupper ((char) *s2) ? tolower ((char) *s2) : *s2;
	if (c1 != c2)
	    return (c1 - c2);
	s1++;
	s2++;
    }

    if (n)
	return (((int) (char) *s1) - ((int) (char) *s2));
    else
	return 0;
#endif
}

void
g_strdown (char *string)
{
    register char *s;

    if (string == NULL)
	return;

    s = string;

    while (*s)
    {
	*s = tolower (*s);
	s++;
    }
}

void
g_strup (char *string)
{
    register char *s;

    if (string == NULL)
	return;

    s = string;

    while (*s)
    {
	*s = toupper (*s);
	s++;
    }
}

char *
g_strdup (const char *string)
{
    char *new_str;

    if (string)
    {
	new_str = (char *) malloc (strlen (string) + 1);
	strcpy (new_str, string);
	new_str[strlen (string)] = '\0';
    } else
	new_str = NULL;

    return new_str;
}

char *
g_strndup (const char *string, unsigned int n)
{
    char *new_str;

    if (string)
    {
	new_str = (char *) malloc (n + 1);
	strncpy (new_str, string, n);
	new_str[n] = '\0';
    } else
	new_str = NULL;

    return new_str;
}
