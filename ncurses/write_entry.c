
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
 *	write_entry.c -- write a terminfo structure onto the file system
 */

#include <curses.priv.h>

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>

#include "tic.h"
#include "term.h"
#include "term_entry.h"

#if !HAVE_EXTERN_ERRNO
extern int errno;
#endif

#ifndef S_ISDIR
#define S_ISDIR(mode) ((mode & S_IFMT) == S_IFDIR)
#endif

static int write_object(FILE *, TERMTYPE *);

static char	*destination = TERMINFO;

/*
 *	make_directory(char *path)
 *
 *	Make a directory if it doesn't exist.
 */
static int make_directory(char *path)
{
int	rc;
struct	stat	statbuf;
char	fullpath[PATH_MAX];

	if (path == destination || *path == '/')
		(void)strcpy(fullpath, path);
	else
		(void)sprintf(fullpath, "%s/%s", destination, path);

	if ((rc = stat(path, &statbuf)) < 0) {
		rc = mkdir(path, 0777);
	} else {
		if (access(path, R_OK|W_OK|X_OK) < 0) {
			_nc_err_abort("%s: permission denied", fullpath);
		} else if (!(S_ISDIR(statbuf.st_mode))) {
			_nc_err_abort("%s: not a directory", fullpath);
		}
	}
	return rc;
}

void  _nc_set_writedir(char *dir)
/* set the write directory for compiled entries */
{
    if (dir != 0)
	destination = dir;
    else if (getenv("TERMINFO") != NULL)
	destination = getenv("TERMINFO");

    if (make_directory(destination) < 0)
    {
	char	*home;

	/* ncurses extension...fall back on user's private directory */
	if ((home = getenv("HOME")) != (char *)NULL)
	{
	    destination = malloc(sizeof(PRIVATE_INFO) + strlen(home));
	    (void) sprintf(destination, PRIVATE_INFO, home);

	    if (make_directory(destination) < 0)
		_nc_err_abort("%s: permission denied (errno %d)",
			destination, errno);
	}
    }

    /*
     * Note: because of this code, this logic should be exercised
     * *once only* per run.
     */
    if (chdir(destination) < 0)
	_nc_err_abort("%s: not a directory", destination);
}

/*
 *	check_writeable(char code)
 *
 *	Miscellaneous initialisations
 *
 *	Check for access rights to destination directories
 *	Create any directories which don't exist.
 *
 */

static void check_writeable(int code)
{
static const char dirnames[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
static bool verified[sizeof(dirnames)];

char		dir[2];
char		*s;

	if (code == 0 || (s = strchr(dirnames, code)) == 0)
	    _nc_err_abort("Illegal terminfo subdirectory \"%c\"", code);

	if (verified[s-dirnames])
	    return;

	dir[0] = code;
	dir[1] = '\0';
	(void) make_directory(dir);

	verified[s-dirnames] = TRUE;
}

/*
 *	_nc_write_entry()
 *
 *	Save the compiled version of a description in the filesystem.
 *
 *	make a copy of the name-list
 *	break it up into first-name and all-but-last-name
 *	creat(first-name)
 *	write object information to first-name
 *	close(first-name)
 *      for each name in all-but-last-name
 *	    link to first-name
 *
 *	Using 'time()' to obtain a reference for file timestamps is unreliable,
 *	e.g., with NFS, because the filesystem may have a different time
 *	reference.  We check for pre-existence of links by latching the first
 *	timestamp from a file that we create.
 *
 *	The _nc_warning() calls will report a correct line number only if
 *	_nc_curr_line is properly set before the write_entry() call.
 */

void _nc_write_entry(TERMTYPE *const tp)
{
struct stat	statbuf;
FILE		*fp;
char		name_list[MAX_TERMINFO_LENGTH];
char		*first_name, *other_names;
char		*ptr;
char		filename[PATH_MAX];
char		linkname[PATH_MAX];
#ifdef USE_SYMLINKS
char		symlinkname[PATH_MAX];
#endif /* USE_SYMLINKS */
static int	call_count;
static time_t	start_time;		/* time at start of writes */

	if (call_count++ == 0) {
		start_time = 0;
	}

	(void) strcpy(name_list, tp->term_names);
	DEBUG(7, ("Name list = '%s'", name_list));

	first_name = name_list;

	ptr = &name_list[strlen(name_list) - 1];
	other_names = ptr + 1;

	while (ptr > name_list  &&  *ptr != '|')
	    	ptr--;

	if (ptr != name_list) {
	    	*ptr = '\0';

	    	for (ptr = name_list; *ptr != '\0'  &&  *ptr != '|'; ptr++)
			continue;
	    
	    	if (*ptr == '\0')
			other_names = ptr;
	    	else {
			*ptr = '\0';
			other_names = ptr + 1;
	    	}
	}

	DEBUG(7, ("First name = '%s'", first_name));
	DEBUG(7, ("Other names = '%s'", other_names));

	_nc_set_type(first_name);

	if (strlen(first_name) > sizeof(filename)-3)
	    	_nc_warning("terminal name too long.");

	sprintf(filename, "%c/%s", first_name[0], first_name);

	/*
	 * Has this primary name been written since the first call to
	 * write_entry()?  If so, the newer write will step on the older,
	 * so warn the user.
	 */
	if (start_time > 0 &&
	    stat(filename, &statbuf) >= 0
	    && statbuf.st_mtime >= start_time)
	{
		_nc_warning("name multiply defined.");
	}

	check_writeable(first_name[0]);
	fp = fopen(filename, "w");
	if (fp == NULL) {
	    	perror(filename);
	    	_nc_syserr_abort("can't open %s/%s", destination, filename);
	}
	DEBUG(1, ("Created %s", filename));

	if (write_object(fp, tp) == ERR) {
	    	_nc_syserr_abort("error writing %s/%s", destination, filename);
	}
	fclose(fp);

	if (start_time == 0) {
	    	if (stat(filename, &statbuf) < 0
		 || (start_time = statbuf.st_mtime) == 0) {
	    		_nc_syserr_abort("error obtaining time from %s/%s",
				destination, filename);
		}
	}
	while (*other_names != '\0') {
	    	ptr = other_names++;
	    	while (*other_names != '|'  &&  *other_names != '\0')
			other_names++;

	    	if (*other_names != '\0')
			*(other_names++) = '\0';

	    	if (strlen(ptr) > sizeof(linkname)-3) {
			_nc_warning("terminal alias %s too long.", ptr);
			continue;
	    	}

		check_writeable(ptr[0]);
	    	sprintf(linkname, "%c/%s", ptr[0], ptr);

	    	if (strcmp(filename, linkname) == 0) {
			_nc_warning("self-synonym ignored");
	    	}
	    	else if (stat(linkname, &statbuf) >= 0  &&
						statbuf.st_mtime < start_time)
	    	{
			_nc_warning("alias %s multiply defined.", ptr);
		}
		else
	    	{
#ifdef USE_SYMLINKS
			strcpy(symlinkname, "../");
			strcat(symlinkname, filename);
#endif /* USE_SYMLINKS */
			unlink(linkname);
#ifndef USE_SYMLINKS
			if (link(filename, linkname) < 0)
#else
			if (symlink(symlinkname, linkname) < 0)
#endif /* USE_SYMLINKS */
			    _nc_syserr_abort("can't link %s to %s", filename, linkname);
			DEBUG(1, ("Linked %s", linkname));
	    	}
	}
}

#undef LITTLE_ENDIAN	/* BSD/OS defines this as a feature macro */
#define HI(x)			((x) / 256)
#define LO(x)			((x) % 256)
#define LITTLE_ENDIAN(p, x)	(p)[0] = LO(x), (p)[1] = HI(x)

static int write_object(FILE *fp, TERMTYPE *tp)
{
char		*namelist;
size_t		namelen, boolmax, nummax, strmax;
char		zero = '\0';
size_t		i;
short		nextfree;
short		offsets[STRCOUNT];
unsigned char	buf[MAX_ENTRY_SIZE];

	namelist = tp->term_names;
	namelen = strlen(namelist) + 1;

	boolmax = 0;
	for (i = 0; i < BOOLWRITE; i++)
		if (tp->Booleans[i])
			boolmax = i+1;

	nummax = 0;
	for (i = 0; i < NUMWRITE; i++)
		if (tp->Numbers[i] != ABSENT_NUMERIC)
			nummax = i+1;

	strmax = 0;
	for (i = 0; i < STRWRITE; i++)
		if (tp->Strings[i] != ABSENT_STRING)
			strmax = i+1;

	nextfree = 0;
	for (i = 0; i < strmax; i++)
	    if (tp->Strings[i] == ABSENT_STRING)
		offsets[i] = -1;
	    else if (tp->Strings[i] == CANCELLED_STRING)
		offsets[i] = -2;
	    else
	    {
		offsets[i] = nextfree;
		nextfree += strlen(tp->Strings[i]) + 1;
	    }

	/* fill in the header */
	LITTLE_ENDIAN(buf,    MAGIC);
	LITTLE_ENDIAN(buf+2,  min(namelen, MAX_NAME_SIZE + 1));
	LITTLE_ENDIAN(buf+4,  boolmax);
	LITTLE_ENDIAN(buf+6,  nummax);
	LITTLE_ENDIAN(buf+8,  strmax);
	LITTLE_ENDIAN(buf+10, nextfree);

	/* write out the header */
	if (fwrite(buf, 12, 1, fp) != 1
		||  fwrite(namelist, sizeof(char), (size_t)namelen, fp) != namelen
		||  fwrite(tp->Booleans, sizeof(char), (size_t)boolmax, fp) != boolmax)
	    	return(ERR);

	/* the even-boundary padding byte */
	if ((namelen+boolmax) % 2 != 0  &&  fwrite(&zero, sizeof(char), 1, fp) != 1)
	    	return(ERR);

#ifdef SHOWOFFSET
	(void) fprintf(stderr, "Numerics begin at %04lx\n", ftell(fp));
#endif /* SHOWOFFSET */

	/* the numerics */
	for (i = 0; i < nummax; i++)
	{
		if (tp->Numbers[i] == -1)	/* HI/LO won't work */
			buf[2*i] = buf[2*i + 1] = 0377;
		else
			LITTLE_ENDIAN(buf + 2*i, tp->Numbers[i]);
	}
	if (fwrite(buf, 2, (size_t)nummax, fp) != nummax)
		return(ERR);
 
#ifdef SHOWOFFSET
	(void) fprintf(stderr, "String offets begin at %04lx\n", ftell(fp));
#endif /* SHOWOFFSET */

	/* the string offsets */
	for (i = 0; i < strmax; i++)
		if (offsets[i] == -1)	/* HI/LO won't work */
			buf[2*i] = buf[2*i + 1] = 0377;
		else if (offsets[i] == -2)	/* HI/LO won't work */
		{
			buf[2*i] = 0376;
			buf[2*i + 1] = 0377;
		}
		else
			LITTLE_ENDIAN(buf + 2*i, offsets[i]);
	if (fwrite(buf, 2, (size_t)strmax, fp) != strmax)
		return(ERR);

#ifdef SHOWOFFSET
	(void) fprintf(stderr, "String table begins at %04lx\n", ftell(fp));
#endif /* SHOWOFFSET */

	/* the strings */
	for (i = 0; i < strmax; i++)
	    if (tp->Strings[i] != ABSENT_STRING && tp->Strings[i] != CANCELLED_STRING)
		if (fwrite(tp->Strings[i], sizeof(char), strlen(tp->Strings[i]) + 1, fp) != strlen(tp->Strings[i]) + 1)
		    return(ERR);

        return(OK);
}


