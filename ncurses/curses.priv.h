
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
 * $Id: curses.priv.h,v 1.37 1996/11/09 21:46:35 tom Exp $
 *
 *	curses.priv.h
 *
 *	Header file for curses library objects which are private to
 *	the library.
 *
 */

#ifndef CURSES_PRIV_H
#define CURSES_PRIV_H 1

#define MODULE_ID(id) /*nothing*/

#include <config.h>

#include <stdlib.h>
#include <sys/types.h>

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#if HAVE_SYS_BSDTYPES_H
#include <sys/bsdtypes.h>	/* needed for ISC */
#endif

#if HAVE_LIMITS_H
# include <limits.h>
#elif HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif

#ifndef PATH_MAX
# if defined(_POSIX_PATH_MAX)
#  define PATH_MAX _POSIX_PATH_MAX
# elif defined(MAXPATHLEN)
#  define PATH_MAX MAXPATHLEN
# else
#  define PATH_MAX 255	/* the Posix minimum path-size */
# endif
#endif

#include <assert.h>
#include <stdio.h>

#include <errno.h>

#if !HAVE_EXTERN_ERRNO
extern int errno;
#endif

#if defined(HAVE_POLL) && defined(HAVE_SYS_STROPTS_H) && defined(HAVE_POLL_H)
#define USE_FUNC_POLL 1
#else
#define USE_FUNC_POLL 0
#endif

/*
 * The internal types (e.g., struct screen) must precede <curses.h>, otherwise
 * we cannot construct lint-libraries (structures must be fully-defined).
 */

struct try {
	struct try      *child;     /* ptr to child.  NULL if none          */
	struct try      *sibling;   /* ptr to sibling.  NULL if none        */
	unsigned char    ch;        /* character at this node               */
	unsigned short   value;     /* code of string so far.  0 if none.   */
};

/*
 * Structure for soft labels.
 */

typedef struct
{
	char *text;             /* text for the label */
	char *form_text;        /* formatted text (left/center/...) */
	int x;                  /* x coordinate of this field */
	char dirty;             /* this label has changed */
	char visible;           /* field is visible */
} slk_ent;

typedef struct {
	char dirty;             /* all labels have changed */
	char hidden;            /* soft labels are hidden */
	struct _win_st *win;
	slk_ent *ent;
	char *buffer;           /* buffer for labels */
	short maxlab;           /* number of available labels */
	short labcnt;           /* number of allocated labels */
	short maxlen;           /* length of labels */
} SLK;

/*
 * Structure for palette tables
 */

typedef struct
{
    short red, green, blue;
}
color_t;

#define MAXCOLUMNS    135
#define MAXLINES      66
#define FIFO_SIZE     MAXLINES

struct screen {
	int             _ifd;           /* input file ptr for screen        */
	FILE            *_ofp;          /* output file ptr for screen       */
	int             _checkfd;       /* filedesc for typeahead check     */
	struct term     *_term;         /* terminal type information        */
	short           _lines;         /* screen lines                     */
	short           _columns;       /* screen columns                   */
	short           _lines_avail;   /* lines available for stdscr       */
	short           _topstolen;     /* lines stolen from top            */
	struct _win_st  *_curscr;       /* current screen                   */
	struct _win_st  *_newscr;       /* virtual screen to be updated to  */
	struct _win_st  *_stdscr;       /* screen's full-window context     */
	struct try      *_keytry;       /* "Try" for use with keypad mode   */
	unsigned int    _fifo[FIFO_SIZE];       /* input push-back buffer   */
	signed char     _fifohead,      /* head of fifo queue               */
	                _fifotail,      /* tail of fifo queue               */
	                _fifopeek;      /* where to peek for next char      */
	int             _endwin;        /* are we out of window mode?       */
	unsigned long   _current_attr;  /* terminal attribute current set   */
	int             _coloron;       /* is color enabled?                */
	int             _cursor;        /* visibility of the cursor         */
	int             _cursrow;       /* physical cursor row              */
	int             _curscol;       /* physical cursor column           */
	int             _nl;            /* True if NL -> CR/NL is on        */
	int             _raw;           /* True if in raw mode              */
	int             _cbreak;        /* 1 if in cbreak mode              */
	                                /* > 1 if in halfdelay mode         */
	int             _echo;          /* True if echo on                  */
	int             _use_meta;      /* use the meta key?                */
	SLK             *_slk;          /* ptr to soft key struct / NULL    */
	int             _baudrate;      /* used to compute padding          */

	/* cursor movement costs; units are 10ths of milliseconds */
	int             _char_padding;  /* cost of character put            */
	int             _cr_cost;       /* cost of (carriage_return)        */
	int             _cup_cost;      /* cost of (cursor_address)         */
	int             _home_cost;     /* cost of (cursor_home)            */
	int             _ll_cost;       /* cost of (cursor_to_ll)           */
#ifdef TABS_OK
	int             _ht_cost;       /* cost of (tab)                    */
	int             _cbt_cost;      /* cost of (backtab)                */
#endif /* TABS_OK */
	int             _cub1_cost;     /* cost of (cursor_left)            */
	int             _cuf1_cost;     /* cost of (cursor_right)           */
	int             _cud1_cost;     /* cost of (cursor_down)            */
	int             _cuu1_cost;     /* cost of (cursor_up)              */
	int             _cub_cost;      /* cost of (parm_cursor_left)       */
	int             _cuf_cost;      /* cost of (parm_cursor_right)      */
	int             _cud_cost;      /* cost of (parm_cursor_down)       */
	int             _cuu_cost;      /* cost of (parm_cursor_up)         */
	int             _hpa_cost;      /* cost of (column_address)         */
	int             _vpa_cost;      /* cost of (row_address)            */
	/* used in lib_doupdate.c */
	int             _ed_cost;       /* cost of (clr_eos)                */
	int             _el_cost;       /* cost of (clr_eol)                */
	int             _el1_cost;      /* cost of (clr_bol)                */
	int             _dch1_cost;     /* cost of (delete_character)       */
	int             _ich1_cost;     /* cost of (insert_character)       */
	int             _dch_cost;      /* cost of (parm_dch)               */
	int             _ich_cost;      /* cost of (parm_ich)               */
	int		_ech_cost;	/* cost of (erase_chars)	    */
	int		_rep_cost;	/* cost of (repeat_char)	    */
	/* used in lib_mvcur.c */
	char *          _address_cursor;
	int             _carriage_return_length;
	int             _cursor_home_length;
	int             _cursor_to_ll_length;
	/* used in lib_color.c */
	color_t		*_color_table;	/* screen's color palette	     */
	int		_color_count;	/* count of colors in palette	     */
	unsigned char	*_color_pairs;	/* screen's color pair list	     */
	int		_pair_count;	/* count of color pairs		     */
};

/* Ncurses' public interface follows the internal types */
#include <curses.h>	/* we'll use -Ipath directive to get the right one! */

typedef	struct {
	int	line;                   /* lines to take, < 0 => from bottom*/
	int	(*hook)(struct _win_st *, int); /* callback for user        */
	struct _win_st *w;              /* maybe we need this for cleanup   */
} ripoff_t;

/* The terminfo source is assumed to be 7-bit ASCII */
#define is7bits(c)	((unsigned)(c) < 128)

#ifndef min
#define min(a,b)	((a) > (b)  ?  (b)  :  (a))
#endif

#ifndef max
#define max(a,b)	((a) < (b)  ?  (b)  :  (a))
#endif

/* usually in <unistd.h> */
#ifndef STDOUT_FILENO
#define STDOUT_FILENO 1
#endif

#ifndef STDERR_FILENO
#define STDERR_FILENO 2
#endif

#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS 0
#endif

#ifndef EXIT_FAILURE
#define EXIT_FAILURE 1
#endif

#ifndef R_OK
#define	R_OK	4		/* Test for read permission.  */
#endif
#ifndef W_OK
#define	W_OK	2		/* Test for write permission.  */
#endif
#ifndef X_OK
#define	X_OK	1		/* Test for execute permission.  */
#endif
#ifndef F_OK
#define	F_OK	0		/* Test for existence.  */
#endif

#define TextOf(c)    ((c) & (chtype)A_CHARTEXT)
#define AttrOf(c)    ((c) & (chtype)A_ATTRIBUTES)

#define BLANK        (' '|A_NORMAL)

#define CHANGED     -1

#define FreeIfNeeded(p)  if(p != 0) free(p)

/*
 * ht/cbt expansion flakes out randomly under Linux 1.1.47, but only when
 * we're throwing control codes at the screen at high volume.  To see this,
 * re-enable TABS_OK and run worm for a while.  Other systems probably don't
 * want to define this either due to uncertainties about tab delays and
 * expansion in raw mode.
 */
#undef TABS_OK	/* OK to use tab/backtab for local motions? */

#ifdef TRACE
#define T(a)	if (_nc_tracing & TRACE_CALLS) _tracef a
#define TR(n, a)	if (_nc_tracing & (n)) _tracef a
#define TPUTS_TRACE(s)	_nc_tputs_trace = s;
extern unsigned _nc_tracing;
extern char *_nc_tputs_trace;
extern char *_nc_visbuf(const char *);
extern long _nc_outchars;
#else
#define T(a)
#define TR(n, a)
#define TPUTS_TRACE(s)
#endif

#define ALL_BUT_COLOR ((chtype)~(A_COLOR))
#define IGNORE_COLOR_OFF FALSE


/* Macro to put together character and attribute info and return it.
   If colors are in the attribute, they have precedence. */
#define ch_or_attr(ch,at) \
    ((PAIR_NUMBER(at) > 0) ? \
     ((((chtype)ch) & ALL_BUT_COLOR) | (at)) : ((((chtype)ch) | (at))))


#define toggle_attr_on(S,at) \
   if (PAIR_NUMBER(at) > 0)\
      (S) = ((S) & ALL_BUT_COLOR) | (at);\
   else\
      (S) |= (at);\
   T(("new attribute is %s", _traceattr((S))))


#define toggle_attr_off(S,at) \
   if (IGNORE_COLOR_OFF == TRUE) {\
      if (PAIR_NUMBER(at) == 0xff) /* turn off color */\
	 (S) &= ~(at);\
      else /* leave color alone */\
	 (S) &= ~((at)|ALL_BUT_COLOR);\
   } else {\
      if (PAIR_NUMBER(at) > 0x00) /* turn off color */\
	 (S) &= ~(at);\
      else /* leave color alone */\
	 (S) &= ~((at)|ALL_BUT_COLOR);\
   }\
   T(("new attribute is %s", _traceattr((S))));

/* lib_acs.c */
extern void init_acs(void);	/* no prefix, this name is traditional */

/* lib_mvcur.c */
#define INFINITY	1000000	/* cost: too high to use */

extern void _nc_mvcur_init(void);
extern void _nc_mvcur_resume(void);
extern void _nc_mvcur_wrap(void);
extern int _nc_mvcur_scrolln(int, int, int, int);

/* lib_mouse.c */
extern void _nc_mouse_init(SCREEN *);
extern bool _nc_mouse_event(SCREEN *);
extern bool _nc_mouse_inline(SCREEN *);
extern bool _nc_mouse_parse(int);
extern void _nc_mouse_wrap(SCREEN *);
extern void _nc_mouse_resume(SCREEN *);
extern int _nc_max_click_interval;

/* elsewhere ... */
extern WINDOW *_nc_makenew(int, int, int, int, int);
extern chtype _nc_background(WINDOW *);
extern chtype _nc_render(WINDOW *, chtype);
extern int _nc_initscr(void);
extern int _nc_keypad(bool);
extern int _nc_outch(int);
extern int _nc_setupscreen(short, short const, FILE *);
extern int _nc_timed_wait(int, int, int *);
extern int _nc_waddch_nosync(WINDOW *, const chtype);
extern void _nc_backspace(WINDOW *win);
extern void _nc_do_color(int, int (*)(int));
extern void _nc_freewin(WINDOW *win);
extern void _nc_get_screensize(void);
extern void _nc_hash_map(void);
extern void _nc_outstr(char *str);
extern void _nc_scroll_optimize(void);
extern void _nc_scroll_window(WINDOW *, int const, short const, short const);
extern void _nc_signal_handler(bool);
extern void _nc_synchook(WINDOW *win);

/*
 * On systems with a broken linker, define 'SP' as a function to force the
 * linker to pull in the data-only module with 'SP'.
 */
#ifndef BROKEN_LINKER
#define BROKEN_LINKER 0
#endif

#if BROKEN_LINKER
#define SP _nc_screen()
extern SCREEN *_nc_screen(void);
extern int _nc_alloc_screen(void);
extern void _nc_set_screen(SCREEN *);
#else
extern SCREEN *SP;
#define _nc_alloc_screen() ((SP = (SCREEN *) calloc(1, sizeof(*SP))) != NULL)
#define _nc_set_screen(sp) SP = sp
#endif

#if !HAVE_USLEEP
extern int _nc_usleep(unsigned int);
#define usleep(msecs) _nc_usleep(msecs)
#endif

/*
 * We don't want to use the lines or columns capabilities internally,
 * because if the application is running multiple screens under
 * X windows, it's quite possible they could all have type xterm
 * but have different sizes!  So...
 */
#define screen_lines	SP->_lines
#define screen_columns	SP->_columns

extern int _nc_slk_format;  /* != 0 if slk_init() called */
extern int _nc_slk_initialize(WINDOW *, int);

/* Macro to check whether or not we use a standard format */
#define SLK_STDFMT (_nc_slk_format < 3)
/* Macro to determine height of label window */
#define SLK_LINES  (SLK_STDFMT ? 1 : (_nc_slk_format - 2))

extern int _nc_ripoffline(int line, int (*init)(WINDOW *,int));

#define UNINITIALISED ((struct try * ) -1)

extern bool _nc_idlok, _nc_idcok;

#endif /* CURSES_PRIV_H */