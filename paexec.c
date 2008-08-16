/*
 * Copyright (c) 2007-2008 Aleksey Cheusov <vle@gmx.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifdef HAVE_CONFIG_H
/* if you need, add extra includes to config.h
   Use may use config.h for getopt_long for example */
#include "config.h"
#endif

#ifndef NO_PORTABHACKS_H
#include "portabhacks.h"
#endif

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <unistd.h>
#include <limits.h>
#include <signal.h>

/***********************************************************/

#ifndef BUFSIZE
#define BUFSIZE 2048
#endif

#ifndef PAEXEC_VERSION
#define PAEXEC_VERSION "x.y.z"
#endif

#include <maa.h>

#include "wrappers.h"
#include "nonblock_helpers.h"

static void usage (void)
{
	fprintf (stderr, "\
paexec - parallel executor\n\
         processes the list of autonomous tasks in parallel\n\
usage: paexec [OPTIONS] [files...]\n\
OPTIONS:\n\
  -h --help                give this help\n\
  -V --version             show version\n\
\n\
  -n --nodes <nodes|+num>  list of nodes|number of subprocesses\n\
  -c --cmd <command>       path to command\n\
  -t --transport <trans>   path to transport program\n\
\n\
  -r --show-node           include node (or number) to the output\n\
  -l --show-line           include 0-based task number (input line number)\n\
                           to the output\n\
  -p --show-pid            include pid of subprocess to the output\n\
\n\
  -e --eot                 print an empty line when end-of-task is reached\n\
  -E --eot-flush           implies -e and flushes stdout\n\
\n\
  -i --i2o                 copy input lines (i.e. tasks) to stdout\n\
  -I --i2o-flush           implies -i and flushes stdout\n\
\n\
  -d --debug               debug mode, for debugging only\n\
-n and -c are mandatory options\n\
\n\
");
}

/* arguments */
static char *arg_nodes     = NULL;
static char *arg_cmd       = NULL;
static char *arg_transport = NULL;
static int debug = 0;

/**/
static int *fd_in       = NULL;
static int *fd_out      = NULL;

static char **buf_out      = NULL;
static size_t *bufsize_out = NULL;
static size_t *size_out    = NULL;

static int *busy        = NULL;
static int busy_count   = 0;

static pid_t *pids      = NULL;

static int *line_nums   = NULL;

static int max_fd    = 0;

static int line_num = 0;

static char *buf_stdin      = NULL;
static size_t bufsize_stdin = 0;
static size_t size_stdin    = 0;

static char **nodes    = NULL;
static int nodes_count = 0;

static int show_pid      = 0;
static int show_line_num = 0;
static int show_node     = 0;

static int print_eot      = 0;
static int flush_eot      = 0;

static int print_i2o      = 0;
static int flush_i2o      = 0;

static int initial_bufsize = BUFSIZE;

static void init (void)
{
	int i;
	char full_cmd [2000];
	char *env_bufsize = getenv ("PAEXEC_BUFSIZE");

	/* BUFSIZE */
	if (env_bufsize){
		initial_bufsize = atoi (env_bufsize);
	}

	bufsize_stdin = initial_bufsize;

	/* arrays */
	pids  = xmalloc (nodes_count * sizeof (*pids));

	fd_in  = xmalloc (nodes_count * sizeof (*fd_in));
	fd_out = xmalloc (nodes_count * sizeof (*fd_out));

	buf_out     = xmalloc (nodes_count * sizeof (*buf_out));
	bufsize_out = xmalloc (nodes_count * sizeof (*bufsize_out));
	size_out    = xmalloc (nodes_count * sizeof (*size_out));

	busy     = xmalloc (nodes_count * sizeof (*busy));

	line_nums = xmalloc (nodes_count * sizeof (*line_nums));

	/* stdin */
	buf_stdin = xmalloc (initial_bufsize);

	/* in/out */
	for (i=0; i < nodes_count; ++i){
		pids [i] = (pid_t) -1;
	}

	for (i=0; i < nodes_count; ++i){
		buf_out [i] = xmalloc (initial_bufsize);
		bufsize_out [i] = initial_bufsize;

		size_out [i] = 0;

		busy [i] = 0;

		if (arg_transport && arg_transport [0])
			snprintf (full_cmd, sizeof (full_cmd), "%s %s %s",
					  arg_transport, nodes [i], arg_cmd);
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

static void kill_childs (void)
{
	int i;
	for (i=0; i < nodes_count; ++i){
		if (pids [i] > 0){
			kill (pids [i], SIGTERM);
		}
	}
}

static void wait_for_childs (void)
{
	int i;
	for (i=0; i < nodes_count; ++i){
		if (pids [i] > 0){
			pr_wait (pids [i]);
			pids [i] = (pid_t) -1;
		}
	}
}

static void exit_with_error (const char *routine, const char *msg)
{
	kill_childs ();
	wait_for_childs ();

	err_fatal (routine, msg);
}

static int find_free_node (void)
{
	int i;
	for (i=0; i < nodes_count; ++i){
		if (!busy [i])
			return i;
	}

	exit_with_error (NULL, "internal error: there is no free node\n");
	return -1;
}

static void print_line (int num, const char *line)
{
	if (show_node){
		if (nodes && nodes [num])
			printf ("%s ", nodes [num]);
		else
			printf ("%d ", num);
	}
	if (show_line_num){
		printf ("%d ", line_nums [num]);
	}
	if (show_pid){
		printf ("%d ", (int) pids [num]);
	}

	printf ("%s\n", line);
}

static void send_to_node (void)
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

	if (print_i2o){
		print_line (n, buf_stdin);
		if (flush_i2o){
			fflush (stdout);
		}
	}
}

static void loop (void)
{
	fd_set rset;

	int printed  = 0;

	int end_of_stdin = 0;
	int ret          = 0;
	int cnt          = 0;
	int i, j;
	char *buf_out_i  = 0;
	char *NL_found   = NULL;

	FD_ZERO (&rset);

	FD_SET (0, &rset);

	while (ret = xselect (max_fd+1, &rset, NULL, NULL, NULL), ret > 0){
		if (debug){
			printf ("select ret=%d\n", ret);
		}

		/* stdin */
		if (FD_ISSET (0, &rset)){
			cnt = xread (0, buf_stdin + size_stdin, bufsize_stdin - size_stdin);
			if (cnt){
				size_stdin += cnt;

				if (size_stdin == bufsize_stdin){
					bufsize_stdin *= 2;
					buf_stdin = xrealloc (buf_stdin, bufsize_stdin);
				}
			}else{
				end_of_stdin = 1;
				for (i=0; i < nodes_count; ++i){
					if (!busy [i]){
						xclose (fd_in [i]);
						fd_in [i] = -1;
					}
				}
			}
		}

		NL_found = memchr (buf_stdin, '\n', size_stdin);
		if (NL_found && busy_count < nodes_count){
			*NL_found = 0;

			if (debug){
				printf ("stdin: %s\n", buf_stdin);
			}

			send_to_node ();

			++NL_found;
			size_stdin -= NL_found - buf_stdin;
			memmove (buf_stdin, NL_found, size_stdin);

			++line_num;
		}

		/* fd_out */
		for (i=0; i < nodes_count; ++i){
			if (FD_ISSET (fd_out [i], &rset)){
				buf_out_i = buf_out [i];

				cnt = xread (fd_out [i],
							 buf_out_i + size_out [i],
							 bufsize_out [i] - size_out [i]);

				if (debug){
					buf_out_i [size_out [i] + cnt] = 0;
					printf ("cnt = %d\n", cnt);
					printf ("buf_out [%d] = %s\n", i, buf_out_i);
					printf ("size_out [%d] = %d\n", i, (int) size_out [i]);
				}

				if (!cnt){
					exit_with_error (__func__, "Unexpected eof\n");
				}

				printed = 0;
				cnt += size_out [i];
				for (j=size_out [i]; j < cnt; ++j){
					if (buf_out_i [j] == '\n'){
						buf_out_i [j] = 0;

						if (printed == j){
							/* end of task marker */
							assert (busy [i] == 1);

							busy [i] = 0;
							--busy_count;

							if (end_of_stdin){
								xclose (fd_in [i]);
								fd_in [i] = -1;
							}

							if (print_eot){
								/* an empty line means end-of-task */
								print_line (i, buf_out [i] + printed);
								if (flush_eot){
									fflush (stdout);
								}
							}

							break;
						}

						print_line (i, buf_out [i] + printed);

						printed = j + 1;
					}
				}

				if (printed){
					cnt -= printed;

					memmove (buf_out_i,
							 buf_out_i + printed,
							 cnt);
				}
				
				size_out [i] = cnt;

				if (size_out [i] == bufsize_out [i]){
					bufsize_out [i] *= 2;
					buf_out [i] = xrealloc (buf_out [i], bufsize_out [i]);
				}
			}
		}

		/* stdin */
		NL_found = memchr (buf_stdin, '\n', size_stdin);
		if (!end_of_stdin && !NL_found && busy_count < nodes_count){
			FD_SET (0, &rset);
		}else{
			FD_CLR (0, &rset);
		}

		/* fd_out */
		for (i=0; i < nodes_count; ++i){
			if (busy [i]){
				FD_SET (fd_out [i], &rset);
			}else{
				FD_CLR (fd_out [i], &rset);
			}
		}

		if (debug){
			printf ("busy_count = %d\n", busy_count);
			printf ("end_of_stdin = %d\n", end_of_stdin);
			for (i=0; i < nodes_count; ++i){
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

	wait_for_childs ();
}

static void split_nodes__plus_notation (void)
{
	int i;

	nodes_count = (int) strtol (arg_nodes + 1, NULL, 10);
	if (nodes_count == (int) LONG_MAX)
		err_fatal_errno ("split_nodes", "invalid option -n:");

	if (arg_transport){
		nodes = xmalloc (nodes_count * sizeof (nodes [0]));

		for (i=0; i < nodes_count; ++i){
			char num [50];
			snprintf (num, sizeof (num), "%d", i);
			nodes [i] = xstrdup (num);
		}
	}
}

static void split_nodes__list (void)
{
	char *last = NULL;
	char *p = arg_nodes;
	char c;

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

static void split_nodes (void)
{
	if (arg_nodes [0] == '+'){
		/* "+NUM" format */
		split_nodes__plus_notation ();
	}else{
		/* list of nodes */
		split_nodes__list ();
	}

	/* final check */
	if (nodes_count == 0)
		err_fatal ("split_nodes", "invalid option -n:\n");
}

static void process_args (int *argc, char ***argv)
{
	int c;

	struct option longopts [] = {
		{ "help",      0, 0, 'h' },
		{ "version",   0, 0, 'V' },

		{ "nodes",     1, 0, 'n' },
		{ "cmd",       1, 0, 'c' },
		{ "transport", 1, 0, 't' },

		{ "show-node", 0, 0, 'r' },
		{ "show-line", 0, 0, 'l' },
		{ "show-pid",  0, 0, 'p' },

		{ "eot",       0, 0, 'e' },
		{ "eot-flush", 0, 0, 'E' },

		{ "i2o",       0, 0, 'i' },
		{ "i2o-flush", 0, 0, 'I' },

		{ "debug",     0, 0, 'd' },

		{ NULL,        0, 0, 0 },
	};

	while (c = getopt_long (*argc, *argv, "hVdvrlpeEiIn:c:t:", longopts, NULL),
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
			case 'r':
				show_node = 1;
				break;
			case 'e':
				print_eot = 1;
				break;
			case 'E':
				print_eot = 1;
				flush_eot = 1;
				break;
			case 'i':
				print_i2o = 1;
				break;
			case 'I':
				print_i2o = 1;
				flush_i2o = 1;
				break;
			default:
				usage ();
				exit (1);
		}
	}

	if (arg_nodes){
		split_nodes ();
	}else{
		err_fatal (NULL, "-n option is mandatory!\n");
	}

	if (!arg_cmd){
		err_fatal (NULL, "-c option is mandatory!\n");
	}
}

static void free_memory (void)
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
	log_stream ("paexec", stderr);

	process_args (&argc, &argv);

	if (debug){
		printf ("nodes_count = %d\n", nodes_count);
		for (i=0; i < nodes_count; ++i){
			printf ("nodes [%d]=%s\n", i, nodes [i]);
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
