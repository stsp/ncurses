
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
 *	comp_parse.c -- parser driver loop and use handling.
 *
 *	_nc_read_entry_source(FILE *, literal, bool, bool (*hook)())
 *	_nc_resolve_uses(void)
 *	_nc_free_entries(void)
 *
 *	Use this code by calling _nc_read_entry_source() on as many source
 *	files as you like (either terminfo or termcap syntax).  If you
 *	want use-resolution, call _nc_resolve_uses().  To free the list 
 *	storage, do _nc_free_entries().
 *
 */

#include <curses.priv.h>

#include <string.h>
#include <ctype.h>

#include "tic.h"
#include "term.h"
#include "term_entry.h"

/****************************************************************************
 *
 * Entry queue handling
 *
 ****************************************************************************/
/*
 *  The entry list is a doubly linked list with NULLs terminating the lists:
 *
 *	  ---------   ---------   ---------
 *	  |       |   |       |   |       |   offset
 *        |-------|   |-------|   |-------|
 *	  |   ----+-->|   ----+-->|  NULL |   next
 *	  |-------|   |-------|   |-------|
 *	  |  NULL |<--+----   |<--+----   |   last
 *	  ---------   ---------   ---------
 *	      ^                       ^
 *	      |                       |
 *	      |                       |
 *	   _nc_head                _nc_tail
 */

ENTRY *_nc_head, *_nc_tail;

static void enqueue(ENTRY *ep)
/* add an entry to the in-core list */
{
	ENTRY	*newp = (ENTRY *)malloc(sizeof(ENTRY));

	if (newp == NULL)
	    _nc_err_abort("Out of memory");

	(void) memcpy(newp, ep, sizeof(ENTRY));

	newp->last = _nc_tail;
	_nc_tail = newp;

	newp->next = (ENTRY *)NULL;
	if (newp->last)
	    newp->last->next = newp;
}

void _nc_free_entries(ENTRY *head)
/* free the allocated storage consumed by list entries */
{
    ENTRY	*ep, *next;

    for (ep = head; ep; ep = next)
    {
	/*
	 * This conditional lets us disconnect storage from the list.
	 * To do this, copy an entry out of the list, then null out
	 * the string-table member in the original and any use entries
	 * it references.
	 */
	if (ep->tterm.str_table)
	    free(ep->tterm.str_table);

	next = ep->next;

	free(ep);
    }
}

bool _nc_entry_match(char *n1, char *n2)
/* do any of the aliases in a pair of terminal names match? */
{
    char	*pstart, *qstart, *pend, *qend;
    char	nc1[MAX_NAME_SIZE+1], nc2[MAX_NAME_SIZE+1];

    if (strchr(n1, '|') == NULL)
    {
	(void) strcpy(nc1, n1);
	(void) strcat(nc1, "|");
	n1 = nc1;
    }

    if (strchr(n2, '|') == NULL)
    {
	(void) strcpy(nc2, n2);
	(void) strcat(nc2, "|");
	n2 = nc2;
    }

    for (pstart = n1; (pend = strchr(pstart, '|')); pstart = pend + 1)
	for (qstart = n2; (qend = strchr(qstart, '|')); qstart = qend + 1)
	    if ((pend-pstart == qend-qstart)
	     && memcmp(pstart, qstart, (size_t)(pend-pstart)) == 0)
		return(TRUE);

    	return(FALSE);
}

/****************************************************************************
 *
 * Entry compiler and resolution logic
 *
 ****************************************************************************/

void _nc_read_entry_source(FILE *fp, char *buf,
			   int literal, bool silent,
			   bool (*hook)(ENTRY *))
/* slurp all entries in the given file into core */
{
    ENTRY	thisentry;
    bool	oldsuppress = _nc_suppress_warnings;
    int		immediate = 0;

    if (silent)
	_nc_suppress_warnings = TRUE;	/* shut the lexer up, too */

    for (_nc_reset_input(fp, buf); _nc_parse_entry(&thisentry, literal, silent) != ERR; )
    {
	if (!isalnum(thisentry.tterm.term_names[0]))
	    _nc_err_abort("terminal names must start with letter or digit");

	/*
	 * This can be used for immediate compilation of entries with no
	 * use references to disk, so as to avoid chewing up a lot of
	 * core when the resolution code could fetch entries off disk.
	 */
	if (hook != NULLHOOK && (*hook)(&thisentry))
	    immediate++;
	else
	    enqueue(&thisentry);
    }

    if (_nc_tail)
    {
	/* set up the head pointer */
	for (_nc_head = _nc_tail; _nc_head->last; _nc_head = _nc_head->last)
	    continue;

	DEBUG(1, ("head = %s", _nc_head->tterm.term_names));
	DEBUG(1, ("tail = %s", _nc_tail->tterm.term_names));
    }
    else if (!immediate)
	DEBUG(1, ("no entries parsed"));

    _nc_suppress_warnings = oldsuppress;
}

int _nc_resolve_uses(void)
/* try to resolve all use capabilities */
{
    ENTRY	*qp, *rp, *lastread = NULL;
    bool	keepgoing;
    int		i, j, unresolved, total_unresolved, multiples;

    DEBUG(2, ("RESOLUTION BEGINNING"));

    /*
     * Check for multiple occurrences of the same name.
     */
    multiples = 0;
    for_entry_list(qp)
    {
	int matchcount = 0;

	for_entry_list(rp)
	    if (qp > rp
		&& _nc_entry_match(qp->tterm.term_names, rp->tterm.term_names))
	    {
		matchcount++;
		if (matchcount == 1)
		{
		    (void) fprintf(stderr, "Name collision between %s",
			   _nc_first_name(qp->tterm.term_names));
		    multiples++;
		}
		if (matchcount >= 1)
		    (void) fprintf(stderr, " %s", _nc_first_name(rp->tterm.term_names));
	    }
	if (matchcount >= 1)
	    (void) putc('\n', stderr);
    }
    if (multiples > 0)
	return(FALSE);

    DEBUG(2, ("NO MULTIPLE NAME OCCURRENCES"));

    /*
     * First resolution stage: replace names in use arrays with entry
     * pointers.  By doing this, we avoid having to do the same name
     * match once for each time a use entry is itself unresolved.
     */
    total_unresolved = 0;
    for_entry_list(qp)
    {
	unresolved = 0;
	for (i = 0; i < qp->nuses; i++)
	{
	    bool	foundit;
	    char	*lookfor = (char *)(qp->uses[i]);

	    foundit = FALSE;

	    /* first, try to resolve from in-core records */
	    for_entry_list(rp)
		if (rp != qp
		    && _nc_name_match(rp->tterm.term_names, lookfor, "|"))
		{
		    DEBUG(2, ("%s: resolving use=%s (in core)",
			      _nc_first_name(qp->tterm.term_names), lookfor));

		    qp->uses[i] = rp;
		    foundit = TRUE;
		}

	    /* if that didn't work, try to merge in a compiled entry */
	    if (!foundit)
	    {
		TERMTYPE	thisterm;
		char		filename[PATH_MAX];

		if (_nc_read_entry(lookfor, filename, &thisterm) == 1)
		{
		    DEBUG(2, ("%s: resolving use=%s (compiled)",
			      _nc_first_name(qp->tterm.term_names), lookfor));

		    rp = (ENTRY *)malloc(sizeof(ENTRY));
		    memcpy(&rp->tterm, &thisterm, sizeof(TERMTYPE));
		    rp->nuses = 0;
		    rp->next = lastread;
		    lastread = rp;

		    qp->uses[i] = rp;
		    foundit = TRUE;
		}
	    }

	    /* no good, mark this one unresolvable and complain */
	    if (!foundit)
	    {
		unresolved++;
		total_unresolved++;

		if (!_nc_suppress_warnings)
		{
		    if (unresolved == 1)
			(void) fprintf(stderr,
			   "%s: use resolution failed on",
			   _nc_first_name(qp->tterm.term_names));
		    (void) fputc(' ', stderr);
		    (void) fputs(lookfor, stderr);
		}

		qp->uses[i] = (ENTRY *)NULL;
	    }
	}
	if (!_nc_suppress_warnings && unresolved)
	    (void) fputc('\n', stderr);
    }
    if (total_unresolved)
    {
	/* free entries read in off disk */
	_nc_free_entries(lastread);
	return(FALSE);
    }

    DEBUG(2, ("NAME RESOLUTION COMPLETED OK"));

    /*
     * OK, at this point all (char *) references have been successfully 
     * replaced by (ENTRY *) pointers.  Time to do the actual merges.
     */
    do {
	TERMTYPE	merged;

	keepgoing = FALSE;

	for_entry_list(qp)
	    if (qp->nuses > 0)
	    {
		DEBUG(2, ("%s: attempting merge", _nc_first_name(qp->tterm.term_names)));
		/*
		 * If any of the use entries we're looking for is
		 * incomplete, punt.  We'll catch this entry on a
		 * subsequent pass.
		 */
		for (i = 0; i < qp->nuses; i++)
		    if (((ENTRY *)qp->uses[i])->nuses)
		    {
			DEBUG(2, ("%s: use entry %d unresolved",
				  _nc_first_name(qp->tterm.term_names), i));
			goto incomplete;
		    }

		/*
		 * First, make sure there's no garbage in the merge block.  
		 * as a side effect, copy into the merged entry the name
		 * field and string table pointer.
		 */
		memcpy(&merged, &qp->tterm, sizeof(TERMTYPE));

		/*
		 * Now merge in each use entry in the proper 
		 * (reverse) order.
		 */
		for (; qp->nuses; qp->nuses--)
		    _nc_merge_entry(&merged,
				&((ENTRY *)qp->uses[qp->nuses-1])->tterm);

		/*
		 * Now merge in the original entry.
		 */
		_nc_merge_entry(&merged, &qp->tterm);

		/*
		 * Replace the original entry with the merged one.
		 */
		memcpy(&qp->tterm, &merged, sizeof(TERMTYPE));

		/*
		 * We know every entry is resolvable because name resolution
		 * didn't bomb.  So go back for another pass.
		 */
		/* FALLTHRU */
	    incomplete:
		keepgoing = TRUE;
	    }
    } while
	(keepgoing);

    DEBUG(2, ("MERGES COMPLETED OK"));

    /*
     * The exit condition of the loop above is such that all entries
     * must now be resolved.  Now handle cancellations.  In a resolved
     * entry there should be no cancellation markers.
     */
    for_entry_list(qp)
    {
	for (j = 0; j < BOOLCOUNT; j++)
	    if (qp->tterm.Booleans[j] == CANCELLED_BOOLEAN)
		qp->tterm.Booleans[j] = FALSE;
	for (j = 0; j < NUMCOUNT; j++)
	    if (qp->tterm.Numbers[j] == CANCELLED_NUMERIC)
		qp->tterm.Numbers[j] = ABSENT_NUMERIC;
	for (j = 0; j < STRCOUNT; j++)
	    if (qp->tterm.Strings[j] == CANCELLED_STRING)
		qp->tterm.Strings[j] = ABSENT_STRING;
    }

    /*
     * We'd like to free entries read in off disk at this point, but can't.
     * The merge_entry() code doesn't copy the strings in the use entries,
     * it just aliases them.  If this ever changes, do a 
     * free_entries(lastread) here.
     */

    DEBUG(2, ("RESOLUTION FINISHED"));

    return(TRUE);
}