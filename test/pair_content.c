/****************************************************************************
 * Copyright 2018-2024,2025 Thomas E. Dickey                                *
 *                                                                          *
 * Permission is hereby granted, free of charge, to any person obtaining a  *
 * copy of this software and associated documentation files (the            *
 * "Software"), to deal in the Software without restriction, including      *
 * without limitation the rights to use, copy, modify, merge, publish,      *
 * distribute, distribute with modifications, sublicense, and/or sell       *
 * copies of the Software, and to permit persons to whom the Software is    *
 * furnished to do so, subject to the following conditions:                 *
 *                                                                          *
 * The above copyright notice and this permission notice shall be included  *
 * in all copies or substantial portions of the Software.                   *
 *                                                                          *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS  *
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF               *
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.   *
 * IN NO EVENT SHALL THE ABOVE COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,   *
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR    *
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR    *
 * THE USE OR OTHER DEALINGS IN THE SOFTWARE.                               *
 *                                                                          *
 * Except as contained in this notice, the name(s) of the above copyright   *
 * holders shall not be used in advertising or otherwise to promote the     *
 * sale, use or other dealings in this Software without prior written       *
 * authorization.                                                           *
 ****************************************************************************/
/*
 * $Id: pair_content.c,v 1.26 2025/07/05 15:21:56 tom Exp $
 */

#define NEED_TIME_H
#include <test.priv.h>

#if USE_EXTENDED_COLOR
typedef int my_color_t;
#else
typedef NCURSES_COLOR_T my_color_t;
#endif

typedef struct {
    my_color_t fg;
    my_color_t bg;
} MYPAIR;

static int f_opt;
static int i_opt;
static int l_opt;
static int n_opt;
static int p_opt;
static int r_opt;
static int s_opt;

#if USE_EXTENDED_COLOR
static int x_opt;
#endif

static MYPAIR *expected;

static TimeType initial_time;
static TimeType finish_time;

static GCC_NORETURN void
finish(int code)
{
    free(expected);
    ExitProgram(code);
}

static void
failed(const char *msg)
{
    printw("%s", msg);
    getch();
    endwin();
    finish(EXIT_FAILURE);
}

#if USE_EXTENDED_COLOR
static int
InitPair(int pair, int fg, int bg)
{
    int rc;
    if (x_opt) {
	rc = init_extended_pair(pair, fg, bg);
    } else {
	rc = init_pair((NCURSES_PAIRS_T) pair,
		       (NCURSES_COLOR_T) fg,
		       (NCURSES_COLOR_T) bg);
    }
    return rc;
}

static int
PairContent(int pair, int *fgp, int *bgp)
{
    int rc;
    if (x_opt) {
	rc = extended_pair_content(pair, fgp, bgp);
    } else {
	short fg, bg;
	if ((rc = pair_content((short) pair, &fg, &bg)) == OK) {
	    *fgp = fg;
	    *bgp = bg;
	}
    }
    return rc;
}
#else
#define InitPair(pair,fg,bg)      init_pair((NCURSES_COLOR_T)pair,(NCURSES_COLOR_T)fg,(NCURSES_COLOR_T)bg)
#define PairContent(pair,fgp,bgp) pair_content((NCURSES_PAIRS_T)pair,fgp,bgp)
#endif

static my_color_t
random_color(void)
{
    return (my_color_t) (rand() % COLORS);
}

static void
setup_test(void)
{
    setlocale(LC_ALL, "");
    initscr();
    cbreak();
    noecho();
    scrollok(stdscr, TRUE);
    if (has_colors()) {
	start_color();

	if (!f_opt)
	    f_opt = 1;
	if (!l_opt)
	    l_opt = COLOR_PAIRS;
	if (l_opt <= 1)
	    failed("color-pair limit must be greater than one");

	if (!n_opt) {
	    int pair;
	    size_t need = (size_t) ((l_opt > COLOR_PAIRS)
				    ? l_opt
				    : COLOR_PAIRS) + 1;

	    expected = typeCalloc(MYPAIR, need);
	    if (s_opt) {
		my_color_t fg;
		my_color_t bg;
		pair = f_opt;
		for (fg = 0; fg < COLORS; ++fg) {
		    for (bg = 0; bg < COLORS; ++bg) {
			if (pair < l_opt) {
			    InitPair(pair, fg, bg);
			    expected[pair].fg = (my_color_t) fg;
			    expected[pair].bg = (my_color_t) bg;
			    ++pair;
			} else {
			    break;
			}
		    }
		}
	    } else {
		for (pair = f_opt; pair < l_opt; ++pair) {
		    expected[pair].fg = random_color();
		    expected[pair].bg = random_color();
		    InitPair(pair, expected[pair].fg, expected[pair].bg);
		}
	    }
	}
    } else {
	failed("This demo requires a color terminal");
    }
    GetClockTime(&initial_time);
}

static void
run_test(void)
{
    int pair;
    bool success = TRUE;
    for (pair = 1; pair < l_opt; ++pair) {
	my_color_t fg;
	my_color_t bg;
	if (PairContent(pair, &fg, &bg) == OK) {
	    if (expected != NULL) {
		if (fg != expected[pair].fg)
		    success = FALSE;
		if (bg != expected[pair].bg)
		    success = FALSE;
	    }
	}
    }
    if (i_opt) {
	addch(success ? '.' : '?');
	refresh();
    }
}

static void
finish_test(void)
{
    getch();
    endwin();
}

static void
usage(int ok)
{
    static const char *msg[] =
    {
	"Usage: pair_content [options]"
	,""
	,USAGE_COMMON
	,"Options:"
	," -f PAIR  first color pair to test (default: 1)"
	," -i       interactive, showing test-progress"
	," -F PAIR  last color pair to test (default: max_pairs-1)"
	," -n       do not initialize color pairs"
	," -p       print data for color pairs instead of testing"
	," -r NUM   repeat tests NUM times"
	," -s       initialize pairs sequentially rather than random"
#if USE_EXTENDED_COLOR
	," -x       use extended color pairs/values"
#endif
    };
    size_t n;
    for (n = 0; n < SIZEOF(msg); n++)
	fprintf(stderr, "%s\n", msg[n]);
    ExitProgram(ok ? EXIT_SUCCESS : EXIT_FAILURE);
}
/* *INDENT-OFF* */
VERSION_COMMON()
/* *INDENT-ON* */

int
main(int argc, char *argv[])
{
    int ch;

    while ((ch = getopt(argc, argv, OPTS_COMMON "f:iF:npr:sx")) != -1) {
	switch (ch) {
	case 'f':
	    if ((f_opt = atoi(optarg)) <= 0)
		usage(FALSE);
	    break;
	case 'i':
	    i_opt = 1;
	    break;
	case 'F':
	    if ((l_opt = atoi(optarg)) <= 0)
		usage(FALSE);
	    break;
	case 'n':
	    n_opt = 1;
	    break;
	case 'p':
	    p_opt = 1;
	    break;
	case 'r':
	    if ((r_opt = atoi(optarg)) <= 0)
		usage(FALSE);
	    break;
	case 's':
	    s_opt = 1;
	    break;
#if USE_EXTENDED_COLOR
	case 'x':
	    x_opt = 1;
	    break;
#endif
	default:
	    CASE_COMMON;
	    /* NOTREACHED */
	}
    }
    if (optind < argc)
	usage(FALSE);
    if (r_opt <= 0)
	r_opt = 1;

    setup_test();
    if (p_opt) {
	int i;
	endwin();
	for (i = f_opt; i < l_opt; ++i) {
	    my_color_t fg, bg;
	    if (PairContent(i, &fg, &bg) == OK) {
		printf("%d: %d %d\n", i, fg, bg);
	    } else {
		printf("%d: ? ?\n", i);
	    }
	}
    } else {
	int repeat;

	for (repeat = 0; repeat < r_opt; ++repeat) {
	    run_test();
	    if (i_opt) {
		addch('.');
		refresh();
	    }
	}

	if (i_opt) {
	    addch('\n');
	}
	printw("DONE: ");
	GetClockTime(&finish_time);
	printw("%.03f seconds", ElapsedSeconds(&initial_time, &finish_time));
	finish_test();
    }

    finish(EXIT_SUCCESS);
}
