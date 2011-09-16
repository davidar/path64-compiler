/*
   Copyright (C) 2010 PathScale Inc. All Rights Reserved.
*/
/*
 * Copyright (C) 2008 PathScale, LLC.  All Rights Reserved.
 */

/*
 * Copyright 2002, 2003, 2004, 2005, 2006 PathScale, Inc.  All Rights Reserved.
 */

/*

  Copyright (C) 2000, 2001 Silicon Graphics, Inc.  All Rights Reserved.

   Path64 is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   Path64 is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with Path64; see the file COPYING.  If not, write to the Free
   Software Foundation, 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

   Special thanks goes to SGI for their continued support to open source

*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <stdarg.h>
#include "string_utils.h"

#define BLANK	' '
#define DOT	'.'
#define SLASH	'/'
#define PERCENT	'%'

char *
string_copy (const char *s)
{
        char *new;
	if (s == NULL) return NULL;
        new = (char *) malloc(strlen(s)+1);
        strcpy(new, s);
        return new;
}

/* make a string starting at position i, and extending for len chars */
char *
substring_copy (const char *s, int i, int len)
{
        char *new;
	if (s == NULL) return NULL;
        new = (char *) malloc(len+1);
        strncpy(new, s+i, len);
	new[len] = '\0';
        return new;
}

/* concatenate two strings */
char *
concat_strings (const char *a, const char *b)
{
	/* allocate new space, make sure it fits */
	char *new = (char *) malloc(strlen(a) + strlen(b) + 1);
	strcpy(new, a);
	strcat(new, b);
	return new;
}

/* return suffix of string */
char *
get_suffix (const char *s)
{
        char *suffix;
        suffix = strrchr (s, DOT);
        if (suffix == NULL) return NULL;
        suffix++;       /* skip the period */
        return suffix;  /* points inside s, not new string! */
}

/* change suffix of string */
/* if no suffix in string, then just return string */
char *
change_suffix (char *s, const char *suffix)
{
        char *new = string_copy(s);
        char *p = get_suffix(new);
	if (p == NULL) return s;
	if (strlen(suffix) <= strlen(p)) {
        	/* new suffix is not longer than old suffix */
        	strcpy(p, suffix);
		return new;
	}
	else {
		/* new suffix is longer */
		*p = NIL;
		return concat_strings(new, suffix);
	}
}


/* string has blank space */
boolean
has_blank (const char *s)
{
	const char *p;
	for (p = s; *p != NIL; p++) {
		if (*p == BLANK) return TRUE;
	}
	return FALSE;
}

/* Replace old_pattern in base string with new_pattern */
/* This modifies the base string in place */
void
replace_substring (char *base, const char *old_pattern, const char *new_pattern)
{
	char *p = strstr (base, old_pattern);
	strncpy(p,new_pattern,strlen(old_pattern));
}

/* Expand template with arg.
 * Basically, the arg string replaces the %* string in the template,
 * e.g. -foo%s with bar becomes -foobar.
 * For now, we assume that %* is last thing in template.
 */
char *
expand_template_string (const char *template, const char *arg)
{
        char *percent;
	char *new = string_copy(template);	/* because we change it */
        percent = strchr (new, PERCENT);
        if (percent == NULL) return new;
	*percent = NIL;
	return concat_strings (new, arg);
}

/* must call this before using a string list */
string_list_t *
init_string_list (void)
{
	string_list_t *p;
	p = (string_list_t *) malloc(sizeof(string_list_t));
	p->head = p->tail = NULL; 
	return p; 
}

void
free_string_list(string_list_t *list)
{
        string_item_t *item = list->head;
        while(item != NULL) {
                free(item->name);
                item = item->next;
        }

        free(list);
}

/* add string that has already been allocated */
static void
add_existing_string (string_list_t *list, char *s)
{
	string_item_t *p;
	p = (string_item_t *) malloc(sizeof(string_item_t));
	p->name = s;
	p->next = NULL;
	if (list->head == NULL) {
		list->head = list->tail = p;
	} else {
		list->tail->next = p;
		list->tail = p;
	}
}

/* add string after another */
void
add_after_string (string_list_t *list, string_item_t *item, char *s)
{
	string_item_t *p;
	p = (string_item_t *) malloc(sizeof(string_item_t));
	p->name = s;
	p->next = item->next;
	item->next = p;
	if (list->tail == item) {
		list->tail = p;
	}
}

/* add string to end of list */
void
add_string (string_list_t *list, const char *s)
{
	/* don't worry about blanks in this version of add_string */
	add_existing_string (list, string_copy(s));
}

void
add_arg(string_list_t *args, const char *format, ...)
{
        char *arg;
        va_list ap;

        va_start(ap, format);

        vasprintf(&arg, format, ap);
        add_string(args, arg);
        free(arg);

        va_end(ap);
}

/* add each blank-separated string to list */
void
add_multi_strings (string_list_t *list, const char *s
#ifdef KEY
			, boolean only_one
#endif
		  )
{
	/* first copy into new string area */
	char *new = string_copy(s);
	if (has_blank(new)) {
		char *t;
		/* break into multiple strings */
		for (t = new; *t != NIL; t++) {
			if (*t == BLANK) {
				*t = NIL;
				add_existing_string(list, new);
				new = t+1;	/* set to next string */
#ifdef KEY
				if (only_one)
				    break;
#endif
			}
		}
	}
	add_existing_string (list, new);
}

/* add string to end of list if not already in list */
void
add_string_if_new (string_list_t *list, const char *s)
{
	string_item_t *p;
	for (p = list->head; p != NULL; p = p->next) {
		if (strcmp(p->name, s) == 0)
			return;		/* already in list */
	}
	/* string not in list */
	add_string (list, s);
}

/* add string to end of list if a file with the same base name is not on the list already */
void
add_string_if_new_basename (string_list_t *list, const char *s)
{
	string_item_t *p;
        char *dup_s = strdup(s);
	char *base_s = strdup(basename(dup_s));
	free(dup_s);
        for (p = list->head; p != NULL; p = p->next) {
		char *base_name;
                char *dup_filename = strdup(p->name);
                base_name = basename(dup_filename); //basename could modify its argument

                if (strcmp(base_name, base_s) == 0 && strlen(base_name) > 1 ){
                        free(dup_filename);
                        free(base_s);
                        return;         /* already in list */
                } else {
                        free(dup_filename);
                }
        }
        /* string not in list */
        add_string (list, s);
        free(base_s);
}



/* replace old in list with new */
void
replace_string (string_list_t *list, const char *old, const char *new)
{
	string_item_t *p;
	for (p = list->head; p != NULL; p = p->next) {
		if (strcmp(p->name, old) == 0) {
			p->name = string_copy(new);
		}
	}
	/* string not in list */
}

/* append string list b at end of string list a */
void
append_string_lists (string_list_t *a, string_list_t *b)
{
	if (a->head == NULL) {
		/* first list is empty */
		a->head = b->head;
		a->tail = b->tail;
	} else if (b->head == NULL) {
		/* nothing to append */
	} else {
		a->tail->next = b->head;
		a->tail = b->tail;
	}
}

void
print_string_list (FILE *f, string_list_t *list)
{
	string_item_t *p;
	for (p = list->head; p != NULL; p = p->next) {
		fprintf(f, "%s ", p->name);
	}
	fprintf(f, "\n");
}

const char *
ends_with(const char *s, const char *sfx)
{
	int len = strlen(s);
	int slen = strlen(sfx);

	if (len >= slen && strcmp(s + len - slen, sfx) == 0)
		return s + len - slen;

	return NULL;
}

/* must call this before using a string pair list */
string_pair_list_t *
init_string_pair_list (void)
{
  string_pair_list_t *p;
  p = (string_pair_list_t *) malloc(sizeof(string_pair_list_t));
  p->head = p->tail = NULL;
  return p; 
}

/* add string pair to end of list */
void
add_string_pair (string_pair_list_t *list, const char *key, const char *val)
{
  string_pair_item_t *p;
  p = (string_pair_item_t *) malloc(sizeof(string_pair_item_t));
  p->key = string_copy(key);
  p->val = string_copy(val);
  p->next = NULL;
  if (list->head == NULL) {
    list->head = list->tail = p;
  } else {
    list->tail->next = p;
    list->tail = p;
  }
}

int string_list_size(const string_list_t *l)
{
  const string_item_t *i;
  int len = 0;

  FOREACH_STRING(i, l) {
    len += 1;
  }

  return len;
}
