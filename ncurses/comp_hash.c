
/***************************************************************************
*                            COPYRIGHT NOTICE                              *
****************************************************************************
*                ncurses is copyright (C) 1992-1995                        *
*                          Zeyd M. Ben-Halim                               *
*                          zmbenhal@netcom.com                             *
*                          Eric S. Raymond                                 *
*                          esr@snark.thyrsus.com                           *
*                                                                          *
*        Permission is hereby granted to reproduce and distribute ncurses  *
*        by any means and for any fee, whether alone or as part of a       *
*        larger distribution, in source or in binary form, PROVIDED        *
*        this notice is included with any such distribution, and is not    *
*        removed from any of its header files. Mention of ncurses in any   *
*        applications linked with it is highly appreciated.                *
*                                                                          *
*        ncurses comes AS IS with no warranty, implied or expressed.       *
*                                                                          *
***************************************************************************/


/*
 *	comp_hash.c --- Routines to deal with the hashtable of capability
 *			names.
 *
 */

#include <string.h>
#include "config.h"
#include "tic.h"
#include "hashsize.h"

#ifdef MAIN_PROGRAM
#include <stdlib.h>
#include <ctype.h>
#undef  DEBUG
#define DEBUG(level, params) /*nothing*/
#endif

static  int hash_function(const char *);

/*
 *	_nc_make_hash_table()
 *
 *	Takes the entries in table[] and hashes them into hash_table[]
 *	by name.  There are CAPTABSIZE entries in table[] and HASHTABSIZE
 *	slots in hash_table[].
 *
 */

#ifdef MAIN_PROGRAM
static void _nc_make_hash_table(struct name_table_entry *table,
		     struct name_table_entry **hash_table)
{
int	i;
int	hashvalue;
int	collisions = 0;

	for (i = 0; i < CAPTABSIZE; i++) {
	    hashvalue = hash_function(table[i].nte_name);       

	    if (hash_table[hashvalue] != (struct name_table_entry *) 0)
		collisions++;

	    if (hash_table[hashvalue] != 0)
		table[i].nte_link = (short)(hash_table[hashvalue] - table);
	    hash_table[hashvalue] = &table[i];
	}

	DEBUG(4, ("Hash table complete: %d collisions out of %d entries", collisions, CAPTABSIZE));
}
#endif


/*
 *	int hash_function(string)
 *
 *	Computes the hashing function on the given string.
 *
 *	The current hash function is the sum of each consectutive pair
 *	of characters, taken as two-byte integers, mod Hashtabsize.
 *
 */

static
int
hash_function(const char *string)
{
long	sum = 0;

	DEBUG(9, ("hashing %s", string));
	while (*string) {
	    sum += (long)(*string + (*(string + 1) << 8));
	    string++;
	}

	DEBUG(9, ("sum is %ld", sum));
	return (int)(sum % HASHTABSIZE);
}


/*
 *	struct name_table_entry *
 *	find_entry(string)
 *
 *	Finds the entry for the given string in the hash table if present.
 *	Returns a pointer to the entry in the table or 0 if not found.
 *
 */

#ifndef MAIN_PROGRAM
struct name_table_entry const *
_nc_find_entry(const char *string, const struct name_table_entry *const *hash_table)
{
int	hashvalue;
struct name_table_entry	const *ptr;

	hashvalue = hash_function(string);

	if ((ptr = hash_table[hashvalue]) != 0) {
		while (strcmp(ptr->nte_name, string) != 0) {
			if (ptr->nte_link < 0)
				return 0;
	    		ptr = ptr->nte_link + hash_table[HASHTABSIZE];
		}
	}

	return (ptr);
}

/*
 *	struct name_table_entry *
 *	find_type_entry(string, type, table)
 *
 *	Finds the first entry for the given name with the given type in the
 *	given table if present (as distinct from find_entry, which finds the
 *	the last entry regardless of type).  You can use this if you detect
 *	a name clash.  It's slower, though.  Returns a pointer to the entry
 *	in the table or 0 if not found.
 */

struct name_table_entry const *
_nc_find_type_entry(const char *string,
		    int type,
		    const struct name_table_entry *table)
{
struct name_table_entry	const *ptr;

	for (ptr = table; ptr < table + CAPTABSIZE; ptr++) {
	    if (ptr->nte_type == type && strcmp(string, ptr->nte_name) == 0)
		return(ptr);
	}

	return ((struct name_table_entry *)NULL);
}
#endif

#ifdef MAIN_PROGRAM
/*
 * This filter reads from standard input a list of tab-delimited columns,
 * (e.g., from Caps.filtered) computes the hash-value of a specified column and
 * writes the hashed tables to standard output.
 *
 * By compiling the hash table at build time, we're able to make the entire
 * set of terminfo and termcap tables readonly (and also provide some runtime
 * performance enhancement).
 */

#ifndef HAVE_STRDUP
static char *strdup (char *s)
{
  char *p;

  p = malloc((unsigned)(strlen(s)+1));
  if (p)
    strcpy(p,s);
  return(p);
}
#endif /* not HAVE_STRDUP */

#define MAX_COLUMNS BUFSIZ	/* this _has_ to be worst-case */
#define typeCalloc(type,elts) (type *)calloc(elts,sizeof(type))

static char **parse_columns(char *buffer)
{
	static char **list;

	int col = 0;

	if (list == 0)
		list = typeCalloc(char *, MAX_COLUMNS);

	if (*buffer != '#') {
		while (*buffer != '\0') {
			char *s;
			for (s = buffer; (*s != '\0') && !isspace(*s); s++)
				/*EMPTY*/;
			if (s != buffer) {
				char mark = *s;
				*s = '\0';
				if ((s - buffer) > 1
				 && (*buffer == '"')
				 && (s[-1] == '"')) {	/* strip the quotes */
					buffer++;
					s[-1] = '\0';
				}
				list[col] = buffer;
				col++;
				if (mark == '\0')
					break;
				while (*++s && isspace(*s))
					/*EMPTY*/;
				buffer = s;
			} else
				break;
		}
	}
	return col ? list : 0;
}

int main(int argc, char **argv)
{
	struct name_table_entry *name_table = typeCalloc(struct name_table_entry, CAPTABSIZE);
	struct name_table_entry **hash_table = typeCalloc(struct name_table_entry *, HASHTABSIZE);
	char *root_name = "";
	int  column = 0;
	int  n;
	char buffer[BUFSIZ];

	static const char * typenames[] = { "BOOLEAN", "NUMBER", "STRING" };

	short BoolCount = 0;
	short NumCount  = 0;
	short StrCount  = 0;

	/* The first argument is the column-number (starting with 0).
	 * The second is the root name of the tables to generate.
	 */
	if (argc <= 2
	 || (column = atoi(argv[1])) <= 0
	 || (column >= MAX_COLUMNS)
	 || *(root_name = argv[2]) == 0) {
		fprintf(stderr, "usage: make_hash column root_name\n");
		exit(1);
	}

	/*
	 * Read the table into our arrays.
	 */
	for (n = 0; (n < CAPTABSIZE) && fgets(buffer, BUFSIZ, stdin); ) {
		char **list, *nlp = strchr(buffer, '\n');
		if (nlp)
		    *nlp = '\0';
		list = parse_columns(buffer);
		if (list == 0)	/* blank or comment */
		    continue;
		name_table[n].nte_link = -1;	/* end-of-hash */
		name_table[n].nte_name = strdup(list[column]);
		if (!strcmp(list[2], "bool")) {
			name_table[n].nte_type  = BOOLEAN;
			name_table[n].nte_index = BoolCount++;
		} else if (!strcmp(list[2], "num")) {
			name_table[n].nte_type  = NUMBER;
			name_table[n].nte_index = NumCount++;
		} else if (!strcmp(list[2], "str")) {
			name_table[n].nte_type  = STRING;
			name_table[n].nte_index = StrCount++;
		} else {
			fprintf(stderr, "Unknown type: %s\n", list[2]);
			exit(1);
		}
		n++;
	}
	_nc_make_hash_table(name_table, hash_table);

	/*
	 * Write the compiled tables to standard output
	 */
	printf("static struct name_table_entry const _nc_%s_table[] =\n",
		root_name);
	printf("{\n");
	for (n = 0; n < CAPTABSIZE; n++) {
		sprintf(buffer, "\"%s\"", 
			name_table[n].nte_name);
		printf("\t{ %15s,\t%10s,\t%3d, %3d }%c\n",
			buffer,
			typenames[name_table[n].nte_type],
			name_table[n].nte_index,
			name_table[n].nte_link,
			n < CAPTABSIZE - 1 ? ',' : ' ');
	}
	printf("};\n\n");

	printf("const struct name_table_entry * const _nc_%s_hash_table[%d] =\n",
		root_name,
		HASHTABSIZE+1);
	printf("{\n");
	for (n = 0; n < HASHTABSIZE; n++) {
		if (hash_table[n] != 0) {
			sprintf(buffer, "_nc_%s_table + %3d",
				root_name,
				hash_table[n] - name_table);
		} else {
			strcpy(buffer, "0");
		}
		printf("\t%s,\n", buffer);
	}
	printf("\t_nc_%s_table\t/* base-of-table */\n", root_name);
	printf("};\n\n");

	printf("#if (BOOLCOUNT!=%d)||(NUMCOUNT!=%d)||(STRCOUNT!=%d)\n",
		BoolCount, NumCount, StrCount);
	printf("#error	--> term.h and comp_captab.c disagree about the <--\n");
	printf("#error	--> numbers of booleans, numbers and/or strings <--\n");
	printf("#endif\n\n");

	return 0;
}
#endif
