/*
 * $Id: inserts.c,v 1.9 2004/07/03 20:15:16 tom Exp $
 *
 * Demonstrate the winsstr() and winsch functions.
 * Thomas Dickey - 2002/10/19
 */

#include <test.priv.h>

#define TABSIZE 8

typedef enum {
    oDefault = 0,
    oMove = 1,
    oWindow = 2,
    oMoveWindow = 3
} Options;

static bool m_opt = FALSE;
static bool w_opt = FALSE;
static int n_opt = -1;

static void
legend(WINDOW *win, int level, Options state, char *buffer, int length)
{
    char *showstate;

    switch (state) {
    default:
    case oDefault:
	showstate = "";
	break;
    case oMove:
	showstate = " (mvXXX)";
	break;
    case oWindow:
	showstate = " (winXXX)";
	break;
    case oMoveWindow:
	showstate = " (mvwinXXX)";
	break;
    }

    wmove(win, 0, 0);
    wprintw(win,
	    "The Strings/Chars displays should match.  Enter any characters, except:\n");
    wprintw(win,
	    "down-arrow or ^N to repeat on next line, 'w' for inner window, 'q' to exit.\n");
    wclrtoeol(win);
    wprintw(win, "Level %d,%s inserted %d characters <%s>", level,
	    showstate, length, buffer);
}

static int
ColOf(char *buffer, int length, int margin)
{
    int n;
    int result;

    for (n = 0, result = margin + 1; n < length; ++n) {
	int ch = UChar(buffer[n]);
	switch (ch) {
	case '\n':
	    /* actually newline should clear the remainder of the line
	     * and move to the next line - but that seems a little awkward
	     * in this example.
	     */
	case '\r':
	    result = 0;
	    break;
	case '\b':
	    if (result > 0)
		--result;
	    break;
	case '\t':
	    result += (TABSIZE - (result % TABSIZE));
	    break;
	case '\177':
	    result += 2;
	    break;
	default:
	    ++result;
	    if (ch < 32)
		++result;
	    break;
	}
    }
    return result;
}

#define LEN(n) ((length - (n) > n_opt) ? n_opt : (length - (n)))
static void
test_inserts(int level)
{
    int ch;
    int limit;
    int row = 1;
    int col;
    int row2, col2;
    int length;
    char buffer[BUFSIZ];
    WINDOW *look = 0;
    WINDOW *work = 0;
    WINDOW *show = 0;
    int margin = (2 * TABSIZE) - 1;
    Options option = ((m_opt ? oMove : oDefault)
		      | ((w_opt || (level > 0)) ? oWindow : oDefault));

    setlocale(LC_ALL, "");

    putenv("TABSIZE=8");
    initscr();
    (void) cbreak();		/* take input chars one at a time, no wait for \n */
    (void) noecho();		/* don't echo input */
    keypad(stdscr, TRUE);

    limit = LINES - 5;
    if (level > 0) {
	look = newwin(limit, COLS - (2 * (level - 1)), 0, level - 1);
	work = newwin(limit - 2, COLS - (2 * level), 1, level);
	show = newwin(4, COLS, limit + 1, 0);
	box(look, 0, 0);
	wnoutrefresh(look);
	limit -= 2;
    } else {
	work = stdscr;
	show = derwin(stdscr, 4, COLS, limit + 1, 0);
    }
    keypad(work, TRUE);

    for (col = margin + 1; col < COLS; col += TABSIZE)
	mvwvline(work, row, col, '.', limit - 2);

    mvwvline(work, row, margin, ACS_VLINE, limit - 2);
    mvwvline(work, row, margin + 1, ACS_VLINE, limit - 2);
    limit /= 2;

    mvwaddstr(work, 1, 2, "String");
    mvwaddstr(work, limit + 1, 2, "Chars");
    wnoutrefresh(work);

    buffer[length = 0] = '\0';
    legend(show, level, option, buffer, length);
    wnoutrefresh(show);

    doupdate();

    /*
     * Show the characters inserted in color, to distinguish from those that
     * are shifted.
     */
    if (has_colors()) {
	start_color();
	init_pair(1, COLOR_WHITE, COLOR_BLUE);
	wbkgdset(work, COLOR_PAIR(1) | ' ');
    }

    while ((ch = wgetch(work)) != 'q') {
	if (ch == ERR) {
	    beep();
	    break;
	}
	wmove(work, row, margin + 1);
	switch (ch) {
	case 'w':
	    test_inserts(level + 1);

	    touchwin(look);
	    touchwin(work);
	    touchwin(show);

	    wnoutrefresh(look);
	    wnoutrefresh(work);
	    wnoutrefresh(show);

	    doupdate();
	    break;
	case CTRL('N'):
	case KEY_DOWN:
	    if (row < limit) {
		++row;
		/* put the whole string in, all at once */
		col2 = margin + 1;
		switch (option) {
		case oDefault:
		    if (n_opt > 1) {
			for (col = 0; col < length; col += n_opt) {
			    col2 = ColOf(buffer, col, margin);
			    if (move(row, col2) != ERR) {
				insnstr(buffer + col, LEN(col));
			    }
			}
		    } else {
			if (move(row, col2) != ERR) {
			    insstr(buffer);
			}
		    }
		    break;
		case oMove:
		    if (n_opt > 1) {
			for (col = 0; col < length; col += n_opt) {
			    col2 = ColOf(buffer, col, margin);
			    mvinsnstr(row, col2, buffer + col, LEN(col));
			}
		    } else {
			mvinsstr(row, col2, buffer);
		    }
		    break;
		case oWindow:
		    if (n_opt > 1) {
			for (col = 0; col < length; col += n_opt) {
			    col2 = ColOf(buffer, col, margin);
			    if (wmove(work, row, col2) != ERR) {
				winsnstr(work, buffer + col, LEN(col));
			    }
			}
		    } else {
			if (wmove(work, row, col2) != ERR) {
			    winsstr(work, buffer);
			}
		    }
		    break;
		case oMoveWindow:
		    if (n_opt > 1) {
			for (col = 0; col < length; col += n_opt) {
			    col2 = ColOf(buffer, col, margin);
			    mvwinsnstr(work, row, col2, buffer + col, LEN(col));
			}
		    } else {
			mvwinsstr(work, row, col2, buffer);
		    }
		    break;
		}

		/* do the corresponding single-character insertion */
		row2 = limit + row;
		for (col = 0; col < length; ++col) {
		    col2 = ColOf(buffer, col, margin);
		    switch (option) {
		    case oDefault:
			if (move(row2, col2) != ERR) {
			    insch(buffer[col]);
			}
			break;
		    case oMove:
			mvinsch(row2, col2, buffer[col]);
			break;
		    case oWindow:
			if (wmove(work, row2, col2) != ERR) {
			    winsch(work, buffer[col]);
			}
			break;
		    case oMoveWindow:
			mvwinsch(work, row2, col2, buffer[col]);
			break;
		    }
		}
	    } else {
		beep();
	    }
	    break;
	case KEY_BACKSPACE:
	    ch = '\b';
	    /* FALLTHRU */
	default:
	    if (ch <= 0 || ch > 255) {
		beep();
		break;
	    }
	    buffer[length++] = ch;
	    buffer[length] = '\0';

	    /* put the string in, one character at a time */
	    col = ColOf(buffer, length - 1, margin);
	    switch (option) {
	    case oDefault:
		if (move(row, col) != ERR) {
		    insstr(buffer + length - 1);
		}
		break;
	    case oMove:
		mvinsstr(row, col, buffer + length - 1);
		break;
	    case oWindow:
		if (wmove(work, row, col) != ERR) {
		    winsstr(work, buffer + length - 1);
		}
		break;
	    case oMoveWindow:
		mvwinsstr(work, row, col, buffer + length - 1);
		break;
	    }

	    /* do the corresponding single-character insertion */
	    switch (option) {
	    case oDefault:
		if (move(limit + row, col) != ERR) {
		    insch(ch);
		}
		break;
	    case oMove:
		mvinsch(limit + row, col, ch);
		break;
	    case oWindow:
		if (wmove(work, limit + row, col) != ERR) {
		    winsch(work, ch);
		}
		break;
	    case oMoveWindow:
		mvwinsch(work, limit + row, col, ch);
		break;
	    }

	    wnoutrefresh(work);

	    legend(show, level, option, buffer, length);
	    wnoutrefresh(show);

	    doupdate();
	    break;
	}
    }
    if (level > 0) {
	delwin(show);
	delwin(work);
	delwin(look);
    }
}

static void
usage(void)
{
    static const char *tbl[] =
    {
	"Usage: inserts [options]"
	,""
	,"Options:"
	,"  -n NUM  limit string-inserts to NUM bytes on ^N replay"
	,"  -m      perform wmove/move separately from insert-functions"
	,"  -w      use window-parameter even when stdscr would be implied"
    };
    unsigned n;
    for (n = 0; n < sizeof(tbl) / sizeof(tbl[0]); ++n)
	fprintf(stderr, "%s\n", tbl[n]);
    ExitProgram(EXIT_FAILURE);
}

int
main(int argc GCC_UNUSED, char *argv[]GCC_UNUSED)
{
    int ch;

    setlocale(LC_ALL, "");

    while ((ch = getopt(argc, argv, "mn:w")) != EOF) {
	switch (ch) {
	case 'm':
	    m_opt = TRUE;
	    break;
	case 'n':
	    n_opt = atoi(optarg);
	    if (n_opt == 0)
		n_opt = -1;
	    break;
	case 'w':
	    w_opt = TRUE;
	    break;
	default:
	    usage();
	    break;
	}
    }
    if (optind < argc)
	usage();

    putenv("TABSIZE=8");
    initscr();
    (void) cbreak();		/* take input chars one at a time, no wait for \n */
    (void) noecho();		/* don't echo input */
    keypad(stdscr, TRUE);

    test_inserts(0);
    endwin();
    ExitProgram(EXIT_SUCCESS);
}
