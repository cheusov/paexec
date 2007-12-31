/*
 * Copyright (c) 2006-2007, Aleksey Cheusov <vle@gmx.net>
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation.  I make no representations
 * about the suitability of this software for any purpose.  It is
 * provided "as is" without express or implied warranty.
 *
 */

#ifdef HAVE_CONFIG_H
/* if you need, add extra includes to config.h
   Use may use config.h for getopt_long for example */
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <unistd.h>
#include <limits.h>

/***********************************************************/

#ifndef BUFSIZE
#define BUFSIZE 2048
#endif

#ifndef PAEXEC_VERSION
#define PAEXEC_VERSION "x.y.z"
#endif

/***********************************************************
  No, I don't want to use overbloated autoconf.
*/
#ifndef NO_HARDCODE

#ifdef __linux__
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <getopt.h>
#endif

#if defined( __NetBSD__) || defined(sun)
#include <getopt.h>
#endif

#endif /* NO_HARDCODE */

/*
  End of getopt_long tricks
***********************************************************/

#define DMALLOC_FUNC_CHECK
#include <maa.h>

#include "wrappers.h"
#include "nonblock_helpers.h"

void usage ()
{
	printf ("\
paexec - processes the list of autonomous tasks in parallel\n\
usage: paexec [OPTIONS] [files...]\n\
OPTIONS:\n\
  -h --help                give this help\n\
  -V --version             show version\n\
\n\
  -n --procs <procs|+num>  list of processors|number of processors\n\
  -c --cmd <command>       path to command\n\
  -t --transport <trans>   path to transport program\n\
\n\
  -r --show-proc           include processor id (or number) to the output\n\
  -l --show-line           include line number (0-based) to the output\n\
  -p --show-pid            include pid of processor to the output\n\
\n\
  -d --debug               debug mode, for debugging only\n\
-n and -c are mandatory options\n\
\n\
");
}

/* arguments */
char *arg_procs     = NULL;
char *arg_cmd       = NULL;
char *arg_transport = NULL;
int debug = 0;

/**/
int *fd_in       = NULL;
int *fd_out      = NULL;

char **buf_out   = NULL;

size_t *size_out = NULL;

int *busy        = NULL;
int busy_count   = 0;

pid_t *pids      = NULL;

int *line_nums   = NULL;

int max_fd    = 0;

int line_num = 0;

char *buf_stdin   = NULL;
size_t size_stdin = 0;

char **procs    = NULL;
int procs_count = 0;

int show_pid      = 0;
int show_line_num = 0;
int show_proc     = 0;

void init (void)
{
	int i;
	char full_cmd [2000];

	/* arrays */
	pids  = xmalloc (procs_count * sizeof (*pids));

	fd_in  = xmalloc (procs_count * sizeof (*fd_in));
	fd_out = xmalloc (procs_count * sizeof (*fd_out));

	buf_out = xmalloc (procs_count * sizeof (*buf_out));

	size_out = xmalloc (procs_count * sizeof (*size_out));

	busy     = xmalloc (procs_count * sizeof (*busy));

	line_nums = xmalloc (procs_count * sizeof (*line_nums));

	/* stdin */
	buf_stdin = xmalloc (BUFSIZE);

	/* in/out */
	for (i=0; i < procs_count; ++i){
		buf_out [i] = xmalloc (BUFSIZE);

		size_out [i] = 0;

		busy [i] = 0;

		if (arg_transport)
			snprintf (full_cmd, sizeof (full_cmd), "%s %s %s",
					  arg_transport, procs [i], arg_cmd);
		else
			snprintf (full_cmd, sizeof (full_cmd), "%s", arg_cmd);

		pids [i] = pr_open (
			full_cmd,
			PR_CREATE_STDIN | PR_CREATE_STDOUT,
			&fd_in [i], &fd_out [i], NULL);

		nonblock (fd_out [i]);

		if (fd_in [i] > max_fd){
			max_fd = fd_in [i];
		}
		if (fd_out [i] > max_fd){
			max_fd = fd_out [i];
		}
	}

	nonblock (0);
}

int find_free_node (void)
{
	int i;
	for (i=0; i < procs_count; ++i){
		if (!busy [i])
			return i;
	}

	err_fatal (NULL, "internal error: there is no free node\n");
}

void send_to_node (void)
{
	int n = find_free_node ();

	if (debug){
		printf ("send to %d (pid: %d)\n", n, (int) pids [n]);
	}

	busy [n]      = 1;
	size_out [n]  = 0;
	line_nums [n] = line_num;

	++busy_count;

	xwrite (fd_in [n], buf_stdin, strlen (buf_stdin));
	xwrite (fd_in [n], "\n", 1);
}

void print_line (int num, int offs)
{
	if (show_proc){
		if (procs && procs [num])
			printf ("%s ", procs [num]);
		else
			printf ("%d ", num);
	}
	if (show_line_num){
		printf ("%d ", line_nums [num]);
	}
	if (show_pid){
		printf ("%d ", (int) pids [num]);
	}

	printf ("%s\n", buf_out [num] + offs);
}

void loop (void)
{
	fd_set rset;

	int printed  = 0;

	int end_of_stdin = 0;
	int ret          = 0;
	int cnt          = 0;
	int i, j;
	int new_sz       = 0;
	char *buf_out_i  = 0;

	FD_ZERO (&rset);

	FD_SET (0, &rset);

	while (ret = xselect (max_fd+1, &rset, NULL, NULL, NULL), ret > 0){
		if (debug){
			printf ("select ret=%d\n", ret);
		}

		/* stdin */
		if (FD_ISSET (0, &rset)){
			cnt = xread (0, buf_stdin + size_stdin, 1 /*BUFSIZE - size_stdin*/);
			if (cnt){
				for (i=0; i < cnt; ++i){
					if (buf_stdin [size_stdin + i] == '\n'){
						buf_stdin [size_stdin + i] = 0;

						if (debug){
							printf ("stdin: %s\n", buf_stdin);
						}

						send_to_node ();

						memmove (buf_stdin,
								 buf_stdin + size_stdin + i + 1,
								 cnt - i - 1);

						size_stdin = 0;
						cnt        -= i + 1;

						i = -1;

						++line_num;
					}
				}

				size_stdin += cnt;
			}else{
				end_of_stdin = 1;
				for (i=0; i < procs_count; ++i){
					if (!busy [i]){
						xclose (fd_in [i]);
						fd_in [i] = -1;
					}
				}
			}
		}

		/* fd_out */
		for (i=0; i < procs_count; ++i){
			if (FD_ISSET (fd_out [i], &rset)){
				buf_out_i = buf_out [i];

				cnt = xread (fd_out [i],
							 buf_out_i + size_out [i],
							 BUFSIZE - size_out [i]);

				if (debug){
					buf_out_i [size_out [i] + cnt] = 0;
					printf ("cnt = %d\n", cnt);
					printf ("buf_out [%d] = %s\n", i, buf_out_i);
					printf ("size_out [%d] = %d\n", i, (int) size_out [i]);
				}

				if (!cnt){
					err_fatal (__func__, "Unexpected eof\n");
				}

				printed = 0;
				cnt += size_out [i];
				for (j=size_out [i]; j < cnt; ++j){
					if (buf_out_i [j] == '\n'){
						buf_out_i [j] = 0;

						if (printed == j){
							assert (busy [i] == 1);

							busy [i] = 0;
							--busy_count;

							if (end_of_stdin){
								xclose (fd_in [i]);
								fd_in [i] = -1;
							}

							break;
						}

						print_line (i, printed);

						printed = j + 1;
					}
				}

				if (printed){
					new_sz = cnt - printed;

					memmove (buf_out_i,
							 buf_out_i + printed,
							 new_sz);

					size_out [i] = new_sz;
				}

				if (size_out [i] == BUFSIZE){
					err_fatal (NULL, "Too long line!\n");
				}
			}
		}

		/* stdin */
		if (!end_of_stdin && busy_count != procs_count){
			FD_SET (0, &rset);
		}else{
			FD_CLR (0, &rset);
		}

		/* fd_out */
		for (i=0; i < procs_count; ++i){
			if (busy [i]){
				FD_SET (fd_out [i], &rset);
			}else{
				FD_CLR (fd_out [i], &rset);
			}
		}

		if (debug){
			printf ("busy_count = %d\n", busy_count);
			printf ("end_of_stdin = %d\n", end_of_stdin);
			for (i=0; i < procs_count; ++i){
				printf ("busy [%d]=%d\n", i, busy [i]);
			}
		}

		/* exit ? */
		if (!busy_count && end_of_stdin)
			break;
	}

	if (debug){
		printf ("wait for childs\n");
	}

	for (i=0; i < procs_count; ++i){
		pr_wait (pids [i]);
	}
}

void split_procs (void)
{
	char *last = NULL;
	char *p = arg_procs;
	char c;
	int i;

	if (arg_procs [0] == '+'){
		/* "+NUM" format */
		procs_count = strtol (arg_procs + 1, NULL, 10);
		if (procs_count == LONG_MAX)
			err_fatal_errno ("split_procs", "invalid option -n:");

		if (arg_transport){
			procs = xmalloc (procs_count * sizeof (procs [0]));

			for (i=0; i < procs_count; ++i){
				char num [50];
				snprintf (num, sizeof (num), "%d", i);
				procs [i] = xstrdup (num);
			}
		}

		return;
	}

	/* "node1 procs2 ..." format */
	for (;;){
		c = *p;

		switch (c){
			case ' ':
			case '\t':
			case '\r':
			case '\n':
			case 0:
				if (last){
					*p = 0;

					++procs_count;

					procs = xrealloc (
						procs,
						procs_count * sizeof (*procs));

					procs [procs_count - 1] = xstrdup (last);

					last = NULL;
				}
				break;
			default:
				if (!last){
					last = p;
				}
				break;
		}

		if (!c)
			break;

		++p;
	}
}

void process_args (int *argc, char ***argv)
{
	int c;

	struct option longopts [] = {
		{ "help",      0, 0, 'h' },
		{ "version",   0, 0, 'V' },

		{ "procs",     1, 0, 'n' },
		{ "cmd",       1, 0, 'c' },
		{ "transport", 1, 0, 't' },

		{ "show-proc", 0, 0, 'r' },
		{ "show-line", 0, 0, 'l' },
		{ "show-pid",  0, 0, 'p' },

		{ "debug",     0, 0, 'd' },

		{ NULL,        0, 0, 0 },
	};

	while (c = getopt_long (*argc, *argv, "hVvrlpn:c:t:", longopts, NULL),
		   c != EOF)
	{
		switch (c) {
			case 'V':
				printf ("paexec %s written by Aleksey Cheusov\n", PAEXEC_VERSION);
				exit (0);
				break;
			case 'h':
				usage ();
				exit (0);
				break;
			case 'd':
				debug = 1;
				break;
			case 'n':
				arg_procs = xstrdup (optarg);
				break;
			case 'c':
				arg_cmd = xstrdup (optarg);
				break;
			case 't':
				arg_transport = xstrdup (optarg);
				break;
			case 'p':
				show_pid = 1;
				break;
			case 'l':
				show_line_num = 1;
				break;
			case 'r':
				show_proc = 1;
				break;
			default:
				usage ();
				exit (1);
		}
	}

	if (arg_procs){
		split_procs ();
	}else{
		err_fatal (NULL, "-n option is mandatory!\n");
	}

	if (!arg_cmd){
		err_fatal (NULL, "-c option is mandatory!\n");
	}
}

void log_to_file (void)
{
	char logfile [2000] = "logfile";

	log_file ("paexec", logfile);
}

void free_memory (void)
{
	int i;

	if (arg_procs)
		xfree (arg_procs);

	if (arg_transport)
		xfree (arg_transport);

	if (arg_cmd)
		xfree (arg_cmd);

	if (procs){
		for (i=0; i < procs_count; ++i){
			xfree (procs [i]);
		}
		xfree (procs);
	}

	if (fd_in)
		xfree (fd_in);
	if (fd_out)
		xfree (fd_out);

	if (buf_stdin)
		xfree (buf_stdin);

	if (buf_out){
		for (i=0; i < procs_count; ++i){
			xfree (buf_out [i]);
		}
		xfree (buf_out);
	}

	if (size_out)
		xfree (size_out);

	if (busy)
		xfree (busy);

	if (pids)
		xfree (pids);

	if (line_nums)
		xfree (line_nums);
}

int main (int argc, char **argv)
{
	int i;

	maa_init ("paexec");
	log_stream ("paexec", stderr);

	process_args (&argc, &argv);

	if (debug){
		printf ("procs_count = %d\n", procs_count);
		for (i=0; i < procs_count; ++i){
			printf ("procs [%d]=%s\n", i, procs [i]);
		}
		printf ("cmd = %s\n", arg_cmd);
	}

	init ();

	loop ();

	free_memory ();

	log_close ();
	maa_shutdown ();
	return 0;
}
