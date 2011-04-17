/*
 * Copyright (c) 2007-2011 Aleksey Cheusov <vle@gmx.net>
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
#include <unistd.h>
#include <limits.h>
#include <signal.h>
#include <ctype.h>
#include <sys/wait.h>
#include <errno.h>

/***********************************************************/

#ifndef BUFSIZE
#define BUFSIZE 2048
#endif

#ifndef PAEXEC_VERSION
#define PAEXEC_VERSION "x.y.z"
#endif

#include <maa.h>

#include "wrappers.h"
#include "common.h"
#include "tasks.h"

static void usage (void)
{
	fprintf (stderr, "\
paexec - parallel executor\n\
         that distributes tasks over CPUs or machines in a network.\n\
usage: paexec [OPTIONS] [files...]\n\
OPTIONS:\n\
  -h               give this help\n\
  -V               show version\n\
\n\
  -n <nodes|+num>  list of nodes|number of subprocesses\n\
  -c <command>     path to a command\n\
  -t <trans>       path to a transport program\n\
\n\
  -r               include a node (or a number) to the output\n\
  -l               include 0-based task number to the output\n\
  -p               include a pid of subprocess to the output\n\
\n\
  -e               print an empty line when end-of-task is reached\n\
  -E               implies -e and flushes stdout\n\
\n\
  -i               copy input lines (i.e. tasks) to stdout\n\
  -I               implies -i and flushes stdout\n\
\n\
  -s|-g            graph of tasks is given on stdin, by default a list of\n\
                   independent tasks is read from stdin\n\
\n\
  -d               debug mode, for debugging only\n\
\n\
  -z               failed nodes are marked as dead\n\
  -Z <timeout>     timeout to restart faild command, imply -z\n\
  -w               wait for restoring nodes (needs -Z)\n\
\n\
  -W               heavier tasks are processed first, a weight\n\
                   of task is a sum of its own weight and weights\n\
                   of all tasks that depend on it,\n\
                   directly or indirectly\n\
\n\
  -m s=<success>\n\
  -m f=<failure>\n\
  -m F=<fatal>     set alternative messages instead of default\n\
                   'success', 'failure' and 'fatal'.\n\
-n and -c are mandatory options\n\
\n\
");
}

/* arguments */
static char *arg_nodes     = NULL;
static char *arg_cmd       = NULL;
static char *arg_transport = NULL;

/**/
static int *fd_in       = NULL;
static int *fd_out      = NULL;

static char **buf_out      = NULL;
static size_t *bufsize_out = NULL;
static size_t *size_out    = NULL;

static int *busy        = NULL;
static int busy_count   = 0;

static pid_t *pids      = NULL;

static int *node2taskid = NULL;

typedef enum {
	rt_undef   = -1,
	rt_success = 0,
	rt_failure = 1,
} ret_code_t;
static ret_code_t *ret_codes   = NULL;

static int max_fd   = 0;

static char *buf_stdin      = NULL;

static char **nodes    = NULL;
static int nodes_count = 0;
static int alive_nodes_count = 0;

static int show_pid      = 0;
static int show_taskid   = 0;
static int show_node     = 0;

static int print_eot      = 0;
static int flush_eot      = 0;

static int print_i2o      = 0;
static int flush_i2o      = 0;

static int initial_bufsize = BUFSIZE;

static int debug           = 0;

static char **node2task         = NULL;
static size_t *node2task_buf_sz = NULL;

static int end_of_stdin = 0;

static const char *poset_success = "success";
static const char *poset_failure = "failure";
static const char *poset_fatal   = "fatal";

static int resistant = 0;
static int resistance_timeout = 0;
static int resistance_last_restart = 0;

static int wait_mode = 0;

static int use_weights = 0;

static void exit_with_error (const char * routine attr_unused, const char *msg);

static void close_all_ins (void)
{
	int i;
	for (i=0; i < nodes_count; ++i){
		if (!busy [i] && fd_in [i] != -1){
			close (fd_in [i]);
			fd_in [i] = -1;
		}
	}
}

static void bad_input_line (const char *line)
{
	fprintf (stderr, "Bad input line: %s\n", line);
	exit_with_error (NULL, NULL);
}

static void init__read_poset_tasks (void)
{
	char *buf = NULL;
	size_t len = 0;

	int id1, id2;

	char *tok1, *tok2, *tok3;
	int tok_cnt;
	char *p;

	char buf_copy [2000];

	/* */
	if (debug){
		fprintf (stderr, "start: init__read_poset_tasks\n");
	}

	/* */
	if (!graph_mode){
		/* completely independent tasks */
		return;
	}

	/* reading all tasks with their dependancies */
	while (buf = xfgetln (stdin, &len), buf != NULL){
		if (len > 0 && buf [len-1] == '\n'){
			buf [len-1] = 0;
			--len;
		}

		strncpy (buf_copy, buf, sizeof (buf_copy));

		tok1 = tok2 = tok3 = NULL;
		tok_cnt = 0;

		for (p=buf; *p; ++p){
			if (*p == ' '){
				*p = 0;
			}else if (p == buf || p [-1] == 0){
				if (!tok1){
					tok1 = p;
					tok_cnt = 1;
				}else if (!tok2){
					tok2 = p;
					tok_cnt = 2;
				}else if (!tok3){
					tok3 = p;
					tok_cnt = 3;
				}else{
					bad_input_line (buf_copy);
				}
			}
		}

		if (!tok_cnt)
			continue;

		if (tok_cnt == 3 && !strcmp (tok1, "weight:")){
			/* weight: <task> <weight> */
			id2 = tasks__add_task (xstrdup (tok2), atoi (tok3));
			continue;
		}

		if (tok_cnt == 2){
			/* <task-from> <task-to> */
			id1 = tasks__add_task (xstrdup (tok1), 1);
			id2 = tasks__add_task (xstrdup (tok2), 1);

			tasks__add_task_arc (id1, id2);
			continue;
		}

		if (tok_cnt == 1){
			/* <task> */
			id1 = tasks__add_task (xstrdup (tok1), 1);
			continue;
		}

		bad_input_line (buf_copy);
	}

	/* */
	if (debug){
		fprintf (stderr, "end: init__read_poset_tasks\n");
	}
}

static void init__child_processes (void)
{
	char full_cmd [2000];
	int i;

	/* */
	if (debug){
		fprintf (stderr, "start: init__child_processes\n");
	}

	/* */
	for (i=0; i < nodes_count; ++i){
		if (pids [i] != (pid_t) -1)
			continue;

		if (!buf_out [i]){
			/* +1 for \0 and -d option */
			buf_out [i] = xmalloc (initial_bufsize+1);
		}
		if (!bufsize_out [i])
			bufsize_out [i] = initial_bufsize;

		size_out [i] = 0;

		busy [i] = 0;

		ret_codes [i] = rt_undef;

		if (arg_transport && arg_transport [0])
			snprintf (full_cmd, sizeof (full_cmd), "%s %s %s",
					  arg_transport, nodes [i], arg_cmd);
		else
			snprintf (full_cmd, sizeof (full_cmd), "%s", arg_cmd);

		pids [i] = pr_open (
			full_cmd,
			PR_CREATE_STDIN | PR_CREATE_STDOUT,
			&fd_in [i], &fd_out [i], NULL);

		++alive_nodes_count;

		nonblock (fd_out [i]);

		if (fd_in [i] > max_fd){
			max_fd = fd_in [i];
		}
		if (fd_out [i] > max_fd){
			max_fd = fd_out [i];
		}
	}

	/* */
	if (debug){
		fprintf (stderr, "end: init__child_processes\n");
	}
}

static void mark_node_as_dead (int node)
{
	if (debug){
		fprintf (stderr, "mark_node_as_dead (%d)\n", node);
	}

	pids [node] = (pid_t) -1;

	if (busy [node]){
		busy [node] = 0;
		--busy_count;
	}

	if (fd_in [node] >= 0)
		close (fd_in  [node]);
	if (fd_out [node] >= 0)
		close (fd_out [node]);

	fd_in  [node] = -1;
	fd_out [node] = -1;

	tasks__mark_task_as_failed (node2taskid [node]);
	--alive_nodes_count;
}

static int sigalrm_tics = 0;
static void handler_sigalrm (int dummy attr_unused)
{
	++sigalrm_tics;
	alarm (1);
}

static void handler_sigchld (int dummy attr_unused)
{
	int status;
	pid_t pid;

	while (pid = waitpid(-1, &status, WNOHANG), pid > 0){
	}
}

static void set_sigalrm_handler (void)
{
	struct sigaction sa;

	sa.sa_handler = handler_sigalrm;
	sigemptyset (&sa.sa_mask);
	sa.sa_flags = 0;
	sigaction (SIGALRM, &sa, NULL);
}

static void set_sigchld_handler (void)
{
	struct sigaction sa;

	sa.sa_handler = handler_sigchld;
	sigemptyset (&sa.sa_mask);
	sa.sa_flags = 0;
	sigaction (SIGCHLD, &sa, NULL);
}

static void ignore_sigpipe (void)
{
	struct sigaction sa;

	sa.sa_handler = SIG_IGN;
	sigemptyset (&sa.sa_mask);
	sa.sa_flags = 0;
	sigaction (SIGPIPE, &sa, NULL);
}

static void block_signals (void)
{
	sigset_t set;

	sigemptyset (&set);
	xsigaddset (&set, SIGALRM);
	xsigaddset (&set, SIGCHLD);

	xsigprocmask (SIG_BLOCK, &set, NULL);
}

static void unblock_signals (void)
{
	sigset_t set;

	sigemptyset (&set);
	xsigaddset (&set, SIGALRM);
	xsigaddset (&set, SIGCHLD);

	xsigprocmask (SIG_UNBLOCK, &set, NULL);
}

static void wait_for_sigalrm (void)
{
	sigset_t set;

	sigemptyset (&set);

	sigsuspend (&set);
}

static void init (void)
{
	char *env_bufsize = getenv ("PAEXEC_BUFSIZE");

	/* BUFSIZE */
	if (env_bufsize){
		initial_bufsize = atoi (env_bufsize);
	}

	/* arrays */
	pids  = xmalloc (nodes_count * sizeof (*pids));
	memset (pids,-1, nodes_count * sizeof (*pids));

	fd_in  = xmalloc (nodes_count * sizeof (*fd_in));
	memset (fd_in, -1, nodes_count * sizeof (*fd_in));

	fd_out = xmalloc (nodes_count * sizeof (*fd_out));
	memset (fd_out, -1, nodes_count * sizeof (*fd_out));

	node2task        = xmalloc (nodes_count * sizeof (*node2task));
	node2task_buf_sz = xmalloc (nodes_count * sizeof (*node2task_buf_sz));
	memset (node2task, 0, nodes_count * sizeof (*node2task));
	memset (node2task_buf_sz, 0, nodes_count * sizeof (*node2task_buf_sz));

	buf_out = xmalloc (nodes_count * sizeof (*buf_out));
	memset (buf_out, 0, nodes_count * sizeof (*buf_out));

	bufsize_out = xmalloc (nodes_count * sizeof (*bufsize_out));
	memset (bufsize_out, 0, nodes_count * sizeof (*bufsize_out));

	size_out = xmalloc (nodes_count * sizeof (*size_out));
	memset (size_out, 0, nodes_count * sizeof (*size_out));

	busy = xmalloc (nodes_count * sizeof (*busy));
	memset (busy, 0, nodes_count * sizeof (*busy));

	node2taskid = xmalloc (nodes_count * sizeof (*node2taskid));

	ret_codes = xmalloc (nodes_count * sizeof (*ret_codes));

	/* stdin */
	buf_stdin = xmalloc (initial_bufsize);
	buf_stdin [0] = 0;

	/* tasks */
	tasks__init ();

	/**/
	init__read_poset_tasks ();

	/**/
	tasks__check_for_cycles ();

	/**/
	switch (use_weights){
		case 1:
			tasks__make_sum_weights ();
			break;
		case 2:
			tasks__make_max_weights ();
			break;
	}

	if (debug)
		tasks__print_sum_weights ();

	/* in/out */
	init__child_processes ();

	/* signal handlers */
	set_sigchld_handler ();
	set_sigalrm_handler ();

	/* alarm(2) */
	if (resistance_timeout)
		alarm (1);

	/* ignore SIGPIPE signal */
	ignore_sigpipe ();
}

static void kill_childs (void)
{
	int i;
	for (i=0; i < nodes_count; ++i){
		if (pids [i] != (pid_t) -1){
			kill (pids [i], SIGTERM);
		}
	}
}

static void wait_for_childs (void)
{
	int i;

	if (debug){
		printf ("wait for childs\n");
	}

	for (i=0; i < nodes_count; ++i){
		if (pids [i] != (pid_t) -1){
			mark_node_as_dead (i);
		}
	}
}

static void exit_with_error (const char * routine attr_unused, const char *msg)
{
	kill_childs ();
	wait_for_childs ();

	fflush (stdout);
	/*	err_fatal (routine, msg);*/
	if (msg)
		fprintf (stderr, "%s\n", msg);

	exit (1);
}

static int find_free_node (void)
{
	int i;
	for (i=0; i < nodes_count; ++i){
		if (pids [i] != (pid_t) -1 && !busy [i])
			return i;
	}

	exit_with_error (NULL, "internal error: there is no free node\n");
	return -1;
}

static void print_header (int num)
{
	if (show_node){
		if (nodes && nodes [num])
			printf ("%s ", nodes [num]);
		else
			printf ("%d ", num);
	}
	if (show_taskid){
		printf ("%d ", node2taskid [num]);
	}
	if (show_pid){
		printf ("%ld ", (long) pids [num]);
	}
}

static void print_line (int num, const char *line)
{
	print_header (num);
	printf ("%s\n", line);
}

static void print_EOT (int num)
{
	if (print_eot){
		print_line (num, "");
		if (flush_eot)
			fflush (stdout);
	}
}

static void send_to_node (void)
{
	int n = find_free_node ();
	size_t task_len = strlen (current_task);

	assert (n >= 0);

	if (debug){
		printf ("send to %d (pid: %ld)\n", n, (long) pids [n]);
	}

	busy [n]      = 1;
	size_out [n]  = 0;
	node2taskid [n] = current_taskid;

	++busy_count;

	if (task_len >= node2task_buf_sz [n]){
		node2task_buf_sz [n] = task_len + 1;
		node2task [n] = xrealloc (node2task [n], node2task_buf_sz [n]);
	}
	memcpy (node2task [n], current_task, task_len + 1);

	if (print_i2o){
		print_line (n, current_task);
		if (flush_i2o){
			fflush (stdout);
		}
	}

	if (-1 == write (fd_in [n], current_task, task_len) ||
		-1 == write (fd_in [n], "\n", 1))
	{
		if (resistant){
			mark_node_as_dead (n);
			if (poset_fatal [0])
				print_line (n, poset_fatal);
			print_EOT (n);

			if (alive_nodes_count == 0 && !wait_mode){
				exit_with_error (NULL, "all nodes failed");
			}
			return;
		}else{
			err_fatal_errno (NULL, "Sending task to the node failed:");
		}
	}
}

static int unblock_select_block (
	int nfds, fd_set * readfds, fd_set * writefds,
	fd_set * exceptfds, struct timeval * timeout)
{
	int ret;
	char msg [200];
	unblock_signals ();

	do {
		errno = 0;
		ret = select (nfds, readfds, writefds, exceptfds, timeout);
	}while (ret == -1 && errno == EINTR);

	if (ret == -1){
		snprintf (msg, sizeof (msg), "select(2) failed: %s", strerror (errno));
		exit_with_error (NULL, msg);
	}

	block_signals ();
	return ret;
}

static int try_to_reinit_failed_nodes (void)
{
	if (resistance_timeout &&
		sigalrm_tics - resistance_last_restart >= resistance_timeout)
	{
		resistance_last_restart = sigalrm_tics;
		init__child_processes ();
		return 1;
	}

	return 0;
}

static int condition (
	fd_set *rset, int max_descr,
	int *ret, const char **task)
{
	*ret = -777;
	*task = NULL;

	if (busy_count < alive_nodes_count && !end_of_stdin &&
		(*task = tasks__get_new_task ()) != NULL)
	{
		return 1;
	}

	if (!task && !graph_mode && feof (stdin)){
		end_of_stdin = 1;
		close_all_ins ();
	}

	if (busy_count > 0
			&& (*ret = unblock_select_block (
					max_descr+1, rset, NULL, NULL, NULL)) != 0)
	{
		return 1;
	}

	if (failed_taskids_count > 0 && wait_mode){
		wait_for_sigalrm ();
		try_to_reinit_failed_nodes ();
		return 1;
	}

	return 0;
}

static void loop (void)
{
	char msg [2000];
	fd_set rset;

	int printed  = 0;

	int ret          = 0;
	int cnt          = 0;
	int i, j;
	char *buf_out_i  = 0;
	const char *task = NULL;
	const char *curr_line = NULL;

	if (graph_mode && tasks_count == 1){
		/* no tasks */
		close_all_ins ();
		wait_for_childs ();
		return;
	}

	FD_ZERO (&rset);

	FD_CLR (0, &rset);

	while (condition (&rset, max_fd, &ret, &task)){
		/* ret == -777 means select(2) was not called */

		if (try_to_reinit_failed_nodes ())
			continue;

		if (ret == -1 && errno == EINTR){
			continue;
		}

		if (debug){
			printf ("select ret=%d\n", ret);
		}

		if (ret == -777 && task)
			send_to_node ();

		/* fd_out */
		for (i=0; ret != -777 && i < nodes_count; ++i){
			if (fd_out [i] >= 0 && FD_ISSET (fd_out [i], &rset)){
				buf_out_i = buf_out [i];

				cnt = read (fd_out [i],
							buf_out_i + size_out [i],
							bufsize_out [i] - size_out [i]);

				if (debug && cnt >= 0){
					buf_out_i [size_out [i] + cnt] = 0;
					printf ("cnt = %d\n", cnt);
					printf ("fd_out [%d] = %d\n", i, fd_out [i]);
					printf ("buf_out [%d] = %s\n", i, buf_out_i);
					printf ("size_out [%d] = %d\n", i, (int) size_out [i]);
				}

				if (cnt == -1 || cnt == 0){
					if (resistant){
						FD_CLR (fd_out [i], &rset);
						mark_node_as_dead (i);
						if (poset_fatal [0])
							print_line (i, poset_fatal);
						print_EOT (i);

						if (alive_nodes_count == 0 && !wait_mode){
							exit_with_error ("loop", "all nodes failed");
						}
						continue;
					}else{
						if (cnt == 0){
							snprintf (
								msg, sizeof (msg),
								"Node %s exited unexpectedly",
								nodes [i]);
						}else{
							snprintf (
								msg, sizeof (msg),
								"reading from node %s failed: %s",
								nodes [i], strerror (errno));
						}

						exit_with_error ("loop", msg);
					}
				}

				printed = 0;
				cnt += size_out [i];
				for (j=size_out [i]; j < cnt; ++j){
					if (buf_out_i [j] == '\n'){
						buf_out_i [j] = 0;

						curr_line = buf_out_i + printed;

						if (printed == j){
							/* end of task marker */
							assert (busy [i] == 1);

							busy [i] = 0;
							--busy_count;

							if (end_of_stdin){
								close (fd_in [i]);
								fd_in [i] = -1;
							}

							/* an empty line means end-of-task */
							if (graph_mode){
								switch (ret_codes [i]){
									case rt_failure:
										print_header (i);
										tasks__delete_task_rec (node2taskid [i]);
										printf ("\n");
										break;
									case rt_success:
										tasks__delete_task (node2taskid [i], 0);
										break;
									case rt_undef:
										print_line (i, "?");
										break;
									default:
										abort ();
								}

								end_of_stdin = (remained_tasks_count == 0);
								if (end_of_stdin)
									close_all_ins ();
							}else{
								tasks__delete_task (node2taskid [i], 0);
							}

							print_EOT (i);
							break;
						}

						if (graph_mode){
							if (!strcmp (curr_line, poset_success)){
								ret_codes [i] = rt_success;
							}else if (!strcmp (curr_line, poset_failure)){
								ret_codes [i] = rt_failure;
							}else{
								ret_codes [i] = rt_undef;
							}
						}

						print_line (i, curr_line);

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

					/* +1 for \0 and -d option */
					buf_out [i] = xrealloc (buf_out [i], bufsize_out [i]+1);
				}
			}
		}

		/* fd_out */
		for (i=0; i < nodes_count; ++i){
			if (fd_out [i] < 0)
				continue;

			if (busy [i]){
				FD_SET (fd_out [i], &rset);
			}else{
				FD_CLR (fd_out [i], &rset);
			}
		}

		if (debug){
			printf ("alive_nodes_count = %d\n", alive_nodes_count);
			printf ("busy_count = %d\n", busy_count);
			printf ("end_of_stdin = %d\n", end_of_stdin);
			for (i=0; i < nodes_count; ++i){
				printf ("busy [%d]=%d\n", i, busy [i]);
				printf ("pid [%d]=%d\n", i, (int) pids [i]);
			}
		}

		/* exit ? */
		if (graph_mode)
			end_of_stdin = (remained_tasks_count == 0);

		if (!busy_count && end_of_stdin)
			break;
	}

	close_all_ins ();
	wait_for_childs ();
}

static void split_nodes__plus_notation (void)
{
	int i;

	nodes_count = (int) strtol (arg_nodes + 1, NULL, 10);
	if (nodes_count == (int) LONG_MAX)
		err_fatal_errno (NULL, "invalid option -n:");

	nodes = xmalloc (nodes_count * sizeof (nodes [0]));

	for (i=0; i < nodes_count; ++i){
		char num [50];
		snprintf (num, sizeof (num), "%d", i);
		nodes [i] = xstrdup (num);
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
	unsigned char c0 = arg_nodes [0];
	if (c0 == '+'){
		/* "+NUM" format */
		split_nodes__plus_notation ();
	}else if (isalnum (c0) || c0 == '/' || c0 == '_'){
		/* list of nodes */
		split_nodes__list ();
	}else{
		err_fatal (NULL, "invalid argument for option -n\n");
	}

	/* final check */
	if (nodes_count == 0)
		err_fatal (NULL, "invalid argument for option -n\n");
}

static void process_args (int *argc, char ***argv)
{
	int c;
#ifdef HAVE_GETOPT_LONG
	struct option longopts [] = {
		{ "help",      0, 0, 1000 + 'h' },
		{ "version",   0, 0, 1000 + 'V' },

		{ "nodes",     1, 0, 1000 + 'n' },
		{ "cmd",       1, 0, 1000 + 'c' },
		{ "transport", 1, 0, 1000 + 't' },

		{ "show-node", 0, 0, 1000 + 'r' },
		{ "show-line", 0, 0, 1000 + 'l' },
		{ "show-pid",  0, 0, 1000 + 'p' },

		{ "eot",       0, 0, 1000 + 'e' },
		{ "eot-flush", 0, 0, 1000 + 'E' },

		{ "i2o",       0, 0, 1000 + 'i' },
		{ "i2o-flush", 0, 0, 1000 + 'I' },

		{ "pos",       0, 0, 1000 + 's' },
		{ "graph",     0, 0, 1000 + 'g' },

		{ "debug",     0, 0, 1000 + 'd' },

		{ "resistant", 0, 0, 1000 + 'z' },

		{ NULL,        0, 0, 0 },
	};
#endif

	static const char optstring [] = "hVdvrlpeEiIwzZ:n:c:t:sgm:W:";

	while (c =
#ifdef HAVE_GETOPT_LONG
		   getopt_long (*argc, *argv, optstring, longopts, NULL),
#else
		   getopt (*argc, *argv, optstring),
#endif
		   c != EOF)
	{
		if (c >= 1000){
			fprintf (stderr, "Long options will be removed in "
			                 "the future, please don't use them!\n\n\n\n");
			c %= 1000;
		}

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
				show_taskid = 1;
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
			case 's':
			case 'g':
				graph_mode = 1;
				break;
			case 'w':
				wait_mode = 1;
				break;
			case 'z':
				resistant = 1;
				break;
			case 'Z':
				resistant = 1;
				resistance_timeout = atoi (optarg);
				break;
			case 'm':
				if (optarg [0] == 's' && optarg [1] == '=')
					poset_success = xstrdup (optarg+2);
				else if (optarg [0] == 'f' && optarg [1] == '=')
					poset_failure = xstrdup (optarg+2);
				else if (optarg [0] == 'F' && optarg [1] == '=')
					poset_fatal = xstrdup (optarg+2);
				else{
					err_fatal (NULL, "bad argument for -m\n");
				}

				break;
			case 'W':
				use_weights = atoi (optarg);
				graph_mode  = 1;
				break;
			default:
				usage ();
				exit (1);
		}
	}

	if (!resistance_timeout && wait_mode){
		err_fatal (NULL, "-w is useless without -Z\n");
	}

	if (arg_nodes){
		split_nodes ();
	}else{
		err_fatal (NULL, "-n option is mandatory!\n");
	}

	if (!arg_cmd){
		err_fatal (NULL, "-c option is mandatory!\n");
	}

	if (use_weights < 0 || use_weights > 2){
		err_fatal (NULL, "Only -W1 and -W2 are supported!\n");
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
			if (nodes [i])
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
			if (buf_out [i])
				xfree (buf_out [i]);
		}
		xfree (buf_out);
	}

	if (node2task){
		for (i=0; i < nodes_count; ++i){
			if (node2task [i])
				xfree (node2task [i]);
		}
		xfree (node2task);
	}
	if (node2task_buf_sz)
		xfree (node2task_buf_sz);

	if (size_out)
		xfree (size_out);

	if (busy)
		xfree (busy);

	if (pids)
		xfree (pids);

	if (node2taskid)
		xfree (node2taskid);

	if (ret_codes)
		xfree (ret_codes);

	tasks__destroy ();
}

int main (int argc, char **argv)
{
	int i;

	block_signals ();

	maa_init ("paexec");
	/* log_stream ("paexec", stderr); */

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

	unblock_signals ();

	return 0;
}
