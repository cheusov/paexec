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
      -h --help                        give this help\n\
      -V --version                     show version\n\
      -v --verbose                     verbose mode\n\
\n\
      -n --nodes <nodes|+num>          list of cluster nodes|number of nodes\n\
      -c --cmd <command>               path to program\n\
      -t --transport <transport>       path to the transport program\n\
-n, -c and -t is mandatory option\n\
");
}

/* arguments */
char *arg_nodes     = NULL;
char *arg_cmd       = NULL;
char *arg_transport = NULL;
int verbose = 0;

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

char **nodes    = NULL;
int nodes_count = 0;

int show_pid      = 0;
int show_line_num = 0;

void init (void)
{
	int i;
	char cmd_arg [2000];

	/* arrays */
	pids  = xmalloc (nodes_count * sizeof (*pids));

	fd_in  = xmalloc (nodes_count * sizeof (*fd_in));
	fd_out = xmalloc (nodes_count * sizeof (*fd_out));

	buf_out = xmalloc (nodes_count * sizeof (*buf_out));

	size_out = xmalloc (nodes_count * sizeof (*size_out));

	busy     = xmalloc (nodes_count * sizeof (*busy));

	line_nums = xmalloc (nodes_count * sizeof (*line_nums));

	/* stdin */
	buf_stdin = xmalloc (BUFSIZE);

	/* in/out */
	for (i=0; i < nodes_count; ++i){
		buf_out [i] = xmalloc (BUFSIZE);

		size_out [i] = 0;

		busy [i] = 0;

		if (arg_transport)
			snprintf (cmd_arg, sizeof (cmd_arg), "%s %s %s",
					  arg_transport, nodes [i], arg_cmd);
		else
			snprintf (cmd_arg, sizeof (cmd_arg), "%s", arg_cmd);

		pids [i] = pr_open (
			cmd_arg,
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

void write_to_exec (void)
{
	int i;
	for (i=0; i < nodes_count; ++i){
		if (!busy [i]){
			if (verbose){
				printf ("send to %d (pid: %d)\n", i, (int) pids [i]);
			}

			busy [i]      = 1;
			size_out [i]  = 0;
			line_nums [i] = line_num;

			++busy_count;

			xwrite (fd_in [i], buf_stdin, strlen (buf_stdin));
			xwrite (fd_in [i], "\n", 1);

			return;
		}
	}

	abort ();
}

void print_line (int num)
{
	if (show_line_num){
		printf ("%d ", line_nums [num]);
	}
	if (show_pid){
		printf ("%d ", (int) pids [num]);
	}

	printf ("%s\n", buf_out [num]);
}

void loop (void)
{
	fd_set rset;

	int ret = 0;
	int eof = 0;
	int cnt = 0;

	int i, j;

	FD_ZERO (&rset);

	FD_SET (0, &rset);

	while (ret = xselect (max_fd+1, &rset, NULL, NULL, NULL), ret > 0){
		if (verbose){
			printf ("select ret=%d\n", ret);
		}

		/* stdin */
		if (FD_ISSET (0, &rset)){
			cnt = xread (0, buf_stdin + size_stdin, 1 /*BUFSIZE - size_stdin*/);
			if (cnt){
				for (i=0; i < cnt; ++i){
					if (buf_stdin [size_stdin + i] == '\n'){
						buf_stdin [size_stdin + i] = 0;

						if (verbose){
							printf ("stdin: %s\n", buf_stdin);
						}

						write_to_exec ();

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
				eof = 1;
				for (i=0; i < nodes_count; ++i){
					if (!busy [i]){
						xclose (fd_in [i]);
					}
				}
			}
		}

		/* fd_out */
		for (i=0; i < nodes_count; ++i){
			if (FD_ISSET (fd_out [i], &rset)){
				cnt = xread (fd_out [i],
							 buf_out [i] + size_out [i],
							 BUFSIZE - size_out [i]);

				if (verbose){
					buf_out [i] [size_out [i] + cnt] = 0;
					printf ("cnt = %d\n", cnt);
					printf ("buf_out [%d] = %s\n", i, buf_out [i]);
					printf ("size_out [%d] = %d\n", i, (int) size_out [i]);
				}

				if (!cnt){
					err_fatal (__func__, "Unexpected eof\n");
				}

				for (j=0; j < cnt; ++j){
					if (buf_out [i] [size_out [i] + j] == '\n'){
						buf_out [i] [size_out [i] + j] = 0;

						if (!buf_out [i] [0]){
							assert (busy [i] == 1);
							busy [i] = 0;
							--busy_count;

							if (eof){
								xclose (fd_in [i]);
							}

							break;
						}

						print_line (i);

						memmove (buf_out [i],
								 buf_out [i] + size_out [i] + j + 1,
								 cnt - j - 1);

						size_out [i] = 0;
						cnt         -= j + 1;

						j = -1;
					}
				}

				size_out [i] += cnt;
				if (size_out [i] == BUFSIZE){
					err_fatal (NULL, "Too long line!\n");
				}
			}
		}

		/* stdin */
		FD_CLR (0, &rset);
		if (!eof && busy_count != nodes_count){
			FD_SET (0, &rset);
		}

		/* fd_out */
		for (i=0; i < nodes_count; ++i){
			FD_CLR (fd_out [i], &rset);
			if (busy [i]){
				FD_SET (fd_out [i], &rset);
			}
		}

		if (verbose){
			printf ("busy_count = %d\n", busy_count);
			printf ("eof = %d\n", eof);
			for (i=0; i < nodes_count; ++i){
				printf ("busy [%d]=%d\n", i, busy [i]);
			}
		}

		/* exit ? */
		if (!busy_count && eof)
			break;
	}

	if (verbose){
		printf ("wait for childs\n");
	}

	for (i=0; i < nodes_count; ++i){
		pr_wait (pids [i]);
	}
}

void split_nodes (void)
{
	char *last = NULL;
	char *p = arg_nodes;
	char c;
	int i;

	if (arg_nodes [0] == '+'){
		/* "+NUM" format */
		nodes_count = strtol (arg_nodes + 1, NULL, 10);
		if (nodes_count == LONG_MAX)
			err_fatal_errno ("split_nodes", "invalid option -n:");

		if (arg_transport){
			nodes = xmalloc (nodes_count * sizeof (nodes [0]));

			for (i=0; i < nodes_count; ++i){
				char num [50];
				snprintf (num, sizeof (num), "%d", i);
				nodes [i] = xstrdup (num);
			}
		}

		return;
	}

	/* "node1 nodes2 ..." format */
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

					++nodes_count;

					nodes = xrealloc (
						nodes,
						nodes_count * sizeof (*nodes));

					nodes [nodes_count - 1] = xstrdup (last);

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
		{ "help",     0, 0, 'h' },
		{ "version",  0, 0, 'V' },
		{ "verbose",  0, 0, 'v' },
		{ "show-pid", 0, 0, 'p' },
		{ "show-line-num", 0, 0, 'l' },
		{ "nodes",    1, 0, 'n' },
		{ "cmd",      1, 0, 'c' },
		{ "transport",1, 0, 't' },
		{ NULL,       0, 0, 0 },
	};

	while (c = getopt_long (*argc, *argv, "hVvpln:c:t:", longopts, NULL),
		   c != EOF)
	{
		switch (c) {
			case 'V':
				printf ("paexec v. 0.2\n");
				exit (0);
				break;
			case 'h':
				usage ();
				exit (0);
				break;
			case 'v':
				verbose = 1;
				break;
			case 'n':
				arg_nodes = xstrdup (optarg);
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
			default:
				usage ();
				exit (1);
		}
	}

	if (arg_nodes){
		split_nodes ();
	}else{
		fprintf (stderr, "-n option is mandatory\n");
		exit (1);
	}

	if (!arg_cmd){
		fprintf (stderr, "-c option is mandatory\n");
		exit (1);
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

	if (arg_nodes)
	xfree (arg_nodes);

	if (arg_transport)
		xfree (arg_transport);

	if (arg_cmd)
		xfree (arg_cmd);

	if (nodes){
		for (i=0; i < nodes_count; ++i){
			xfree (nodes [i]);
		}
		xfree (nodes);
	}

	if (fd_in)
		xfree (fd_in);
	if (fd_out)
		xfree (fd_out);

	if (buf_stdin)
		xfree (buf_stdin);

	if (buf_out){
		for (i=0; i < nodes_count; ++i){
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

	/*	log_to_file (); */

	process_args (&argc, &argv);

	if (verbose){
		printf ("nodes_count = %d\n", nodes_count);
		for (i=0; i < nodes_count; ++i){
			printf ("nodes [%d]=%s\n", i, nodes [i]);
		}
		printf ("cmd = %s\n", arg_cmd);
	}

	init ();

	log_info ("I started");

	loop ();

	free_memory ();

	maa_shutdown ();
	return 0;
}
