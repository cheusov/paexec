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
  -s --pos                 partially ordered set of tasks is given on stdin\n\
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

typedef enum {
	rt_undef   = -1,
	rt_success = 0,
	rt_failure = 1,
} ret_code_t;
static ret_code_t *ret_codes   = NULL;

static int max_fd   = 0;

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

static int debug           = 0;

static int poset_of_tasks  = 0;
static hsh_HashTable tasks;
static int tasks_count = 1; /* 0 - special meaning, not task ID */
static const char ** id2task = NULL;
static int remained_tasks_count = 0;

static int *arcs_from = NULL;
static int *arcs_to   = NULL;
static int arcs_count = 0;

static int *tasks_graph_deg = NULL;

static char *current_task = NULL;
static size_t current_task_sz = 0;

static char *NL_found = NULL;
static int end_of_stdin = 0;

static const char *poset_success = "success";
static const char *poset_failure = "failure";

static int *deleted_tasks = NULL;

static void close_all_ins (void)
{
	int i;
	for (i=0; i < nodes_count; ++i){
		if (!busy [i] && fd_in [i] != -1){
			xclose (fd_in [i]);
			fd_in [i] = -1;
		}
	}
}

static void delete_task (int task, int print_task)
{
	int i, to;

	assert (task >= 0);

	for (i=0; i < arcs_count; ++i){
		to = arcs_to [i];
		if (arcs_from [i] == task){
			if (tasks_graph_deg [to] > 0)
				--tasks_graph_deg [to];
		}
	}

	if (tasks_graph_deg [task] >= -1){
		tasks_graph_deg [task] = -2;

		--remained_tasks_count;
	}

	end_of_stdin = (remained_tasks_count == 0);
	if (end_of_stdin)
		close_all_ins ();

	if (print_task){
		if (!deleted_tasks [task]){
			printf ("%s ", id2task [task]);
			deleted_tasks [task] = 1;
		}
	}
}

static void delete_task_rec2 (int task)
{
	int i, to;

	assert (task >= 0);

	delete_task (task, 1);

	for (i=0; i < arcs_count; ++i){
		if (arcs_from [i] == task){
			to = arcs_to [i];
			delete_task_rec2 (to);
		}
	}
}

static void delete_task_rec (int task)
{
	memset (deleted_tasks, 0, tasks_count * sizeof (*deleted_tasks));

	delete_task_rec2 (task);
}

static const char * get_new_task_from_stdin (void)
{
	NL_found = memchr (buf_stdin, '\n', size_stdin);
	if (!NL_found)
		return NULL;

	*NL_found = 0;

	return buf_stdin;
}

static int get_new_task_num_from_graph (void)
{
	int i;

	for (i=1; i < tasks_count; ++i){
		assert (tasks_graph_deg [i] >= -2);

		if (tasks_graph_deg [i] == 0)
			return i;
	}

	return -1;
}

static const char * get_new_task_from_graph (void)
{
	/* topological sort of task graph */
	int num = get_new_task_num_from_graph ();
	if (num == -1)
		return NULL;

	line_num = num;
	tasks_graph_deg [num] = -1;
	return id2task [num];
}

static const char *get_new_task (void)
{
	const char *task = NULL;
	size_t task_len = 0;

	if (poset_of_tasks)
		task = get_new_task_from_graph ();
	else
		task = get_new_task_from_stdin ();

	if (!task)
		return NULL;

	task_len = strlen (task);

	if (task_len >= current_task_sz){
		current_task_sz = task_len+1;
		current_task = (char *) xrealloc (current_task, current_task_sz);
	}

	strcpy (current_task, task);

	if (!poset_of_tasks){
		++NL_found;
		size_stdin -= NL_found - buf_stdin;
		memmove (buf_stdin, NL_found, size_stdin);

		++line_num;
	}

	return current_task;
}

typedef union {
	int integer;
	const void *ptr;
} int_ptr_union_t;

static int add_task (const char *s)
{
	int_ptr_union_t r;

	r.integer = 0;
	r.ptr = hsh_retrieve (tasks, s);
	if (r.ptr){
		return r.integer;
	}else{
		r.ptr = NULL;
		r.integer = tasks_count;
		hsh_insert (tasks, s, r.ptr);

		++tasks_count;
		++remained_tasks_count;

		id2task = (const char **) xrealloc (
			id2task, tasks_count * sizeof (*id2task));
		id2task [tasks_count-1] = s;

		return tasks_count-1;
	}
}

static void print_cycle (const int *lnk, int s, int t)
{
	int k = lnk [s * tasks_count + t];
	if (k == s){
		fprintf (stderr, "  %s -> %s\n", id2task [s], id2task [t]);
	}else{
		print_cycle (lnk, s, k);
		print_cycle (lnk, k, t);
	}
}

static void init__check_cycles (void)
{
	int i, j, k;
	int from, to;
	int *lnk = NULL;

	lnk = xmalloc (tasks_count * tasks_count * sizeof (*lnk));
	memset (lnk, -1, tasks_count * tasks_count * sizeof (*lnk));

	/* initial arcs */
	for (i=0; i < arcs_count; ++i){
		from = arcs_from [i];
		to   = arcs_to [i];
		lnk [from * tasks_count + to] = from;
	}

	/* transitive closure (algorithm by Floyd) */
	for (k=0; k < tasks_count; ++k){
		for (i=0; i < tasks_count; ++i){
			for (j=0; j < tasks_count; ++j){
				if (lnk [i * tasks_count + j] == -1 &&
					lnk [i * tasks_count + k] >= 0 &&
					lnk [k * tasks_count + j] >= 0)
				{
					lnk [i * tasks_count + j] = k;
				}
			}
		}
	}

	/* looking for cycles */
	for (i=0; i < tasks_count; ++i){
		if (lnk [i * tasks_count + i] >= 0){
			fprintf (stderr, "Cyclic dependancy detected:\n");

			print_cycle (lnk, i, i);

			exit (1);
		}
	}
}

static void init__read_poset_tasks (void)
{
	char buf [BUFSIZE];
	int i;

	if (!poset_of_tasks){
		/* completely independent tasks */
		nonblock (0);
		return;
	}

	/* partially ordered set of tasks */
	tasks = hsh_create (NULL, NULL);

	/* reading all tasks with their dependancies */
	while (fgets (buf, sizeof (buf), stdin)){
		size_t len = strlen (buf);
		char *sep = strchr (buf, ' ');
		int id1, id2;
		char *s1, *s2;

		if (len > 0 && buf [len-1] == '\n'){
			buf [len-1] = 0;
		}

		if (sep){
			/* task2(to) */
			*sep = 0;
			s2 = xstrdup (sep+1);
			id2 = add_task (s2);
		}
		/* task1(from) */
		s1 = xstrdup (buf);
		id1 = add_task (s1);

		if (sep){
			/* (from,to) pair */
			++arcs_count;
			arcs_from = (int *) xrealloc (arcs_from,
										  arcs_count * sizeof (*arcs_from));
			arcs_to = (int *) xrealloc (arcs_to,
										arcs_count * sizeof (*arcs_to));

			arcs_from [arcs_count-1] = id1;
			arcs_to [arcs_count-1]   = id2;
		}
	}

	/* degree for each task */
	tasks_graph_deg = (int *) xmalloc (
		tasks_count * sizeof (*tasks_graph_deg));
	memset (tasks_graph_deg, 0, tasks_count * sizeof (*tasks_graph_deg));

	for (i=0; i < arcs_count; ++i){
		++tasks_graph_deg [arcs_to [i]];
	}
}

static void init__child_processes (void)
{
	char full_cmd [2000];
	int i;

	for (i=0; i < nodes_count; ++i){
		pids [i] = (pid_t) -1;
	}

	for (i=0; i < nodes_count; ++i){
		buf_out [i] = xmalloc (initial_bufsize);
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

		nonblock (fd_out [i]);

		if (fd_in [i] > max_fd){
			max_fd = fd_in [i];
		}
		if (fd_out [i] > max_fd){
			max_fd = fd_out [i];
		}
	}
}

static void init (void)
{
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

	ret_codes = xmalloc (nodes_count * sizeof (*ret_codes));

	/* stdin */
	buf_stdin = xmalloc (initial_bufsize);
	buf_stdin [0] = 0;

	/* in/out */
	init__child_processes ();

	/**/
	init__read_poset_tasks ();

	/* recursive task deleting and rhomb-like dependencies */
	if (tasks_count)
		deleted_tasks = xmalloc (tasks_count * sizeof (*deleted_tasks));

	/**/
	init__check_cycles ();
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

	if (debug){
		printf ("wait for childs\n");
	}

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

static void print_header (int num)
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
}

static void print_line (int num, const char *line)
{
	print_header (num);
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

	xwrite (fd_in [n], current_task, strlen (current_task));
	xwrite (fd_in [n], "\n", 1);

	if (print_i2o){
		print_line (n, current_task);
		if (flush_i2o){
			fflush (stdout);
		}
	}
}

static void loop (void)
{
	fd_set rset;

	int printed  = 0;

	int ret          = 0;
	int cnt          = 0;
	int i, j;
	char *buf_out_i  = 0;

	const char *curr_line = NULL;

	if (poset_of_tasks && tasks_count == 1){
		/* no tasks */
		close_all_ins ();
		wait_for_childs ();
		return;
	}

	FD_ZERO (&rset);

	if (!poset_of_tasks)
		FD_SET (0, &rset);

	while (ret = -777,
		   (poset_of_tasks
			&& busy_count < nodes_count
			&& get_new_task_num_from_graph () != -1) ||
		   (ret = xselect (max_fd+1, &rset, NULL, NULL, NULL)) >= 0)
	{
		/* ret == -777 means select(2) was not called */

		if (debug){
			printf ("select ret=%d\n", ret);
		}

		/* stdin */
		if (ret != -777 && FD_ISSET (0, &rset)){
			cnt = xread (0, buf_stdin + size_stdin, 1/*bufsize_stdin - size_stdin*/);
			if (cnt){
				size_stdin += cnt;

				if (size_stdin == bufsize_stdin){
					bufsize_stdin *= 2;
					buf_stdin = xrealloc (buf_stdin, bufsize_stdin);
				}
			}else{
				end_of_stdin = 1;
				close_all_ins ();
			}
		}

		if (busy_count < nodes_count && get_new_task ())
			send_to_node ();

		/* fd_out */
		for (i=0; ret != -777 && i < nodes_count; ++i){
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

						curr_line = buf_out_i + printed;

						if (printed == j){
							/* end of task marker */
							assert (busy [i] == 1);

							busy [i] = 0;
							--busy_count;

							if (end_of_stdin){
								xclose (fd_in [i]);
								fd_in [i] = -1;
							}

							/* an empty line means end-of-task */
							if (poset_of_tasks){
								switch (ret_codes [i]){
									case rt_failure:
										print_header (i);
										delete_task_rec (line_nums [i]);
										printf ("\n");
										break;
									case rt_success:
										delete_task (line_nums [i], 0);
										break;
									case rt_undef:
										print_line (i, "?");
										break;
									default:
										abort ();
								}
							}

							if (print_eot){
								print_line (i, curr_line);
								if (flush_eot){
									fflush (stdout);
								}
							}

							break;
						}

						if (poset_of_tasks){
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
					buf_out [i] = xrealloc (buf_out [i], bufsize_out [i]);
				}
			}
		}

		/* stdin */
		if (!poset_of_tasks){
			NL_found = memchr (buf_stdin, '\n', size_stdin);
			if (!end_of_stdin && !NL_found && busy_count < nodes_count){
				FD_SET (0, &rset);
			}else{
				FD_CLR (0, &rset);
			}
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
		if (poset_of_tasks)
			end_of_stdin = (remained_tasks_count == 0);

		if (!busy_count && end_of_stdin && (poset_of_tasks || !size_stdin))
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
		err_fatal_errno ("split_nodes", "invalid option -n:");

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

		{ "pos",       0, 0, 's' },

		{ "debug",     0, 0, 'd' },

		{ NULL,        0, 0, 0 },
	};

	while (c = getopt_long (*argc, *argv, "hVdvrlpeEiIn:c:t:s", longopts, NULL),
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
			case 's':
				poset_of_tasks = 1;
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

	if (ret_codes)
		xfree (ret_codes);

	if (poset_of_tasks){
		hsh_destroy (tasks);
	}

	if (deleted_tasks)
		xfree (deleted_tasks);
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
