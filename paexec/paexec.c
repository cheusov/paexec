/*
 * Copyright (c) 2007-2013 Aleksey Cheusov <vle@gmx.net>
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
/* if you need, add extra includes to config.h */
#include "config.h"
#endif

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <limits.h>
#include <ctype.h>
#include <errno.h>
#include <sys/wait.h>

/***********************************************************/

#include "decls.h"

#include "wrappers.h"
#include "common.h"
#include "tasks.h"
#include "nodes.h"
#include "signals.h"
#include "pr.h"

#ifndef BUFSIZE
#define BUFSIZE 2048
#endif

#ifndef PAEXEC_VERSION
#define PAEXEC_VERSION "x.y.z"
#endif

static void usage (void)
{
	fprintf (stderr, "\
paexec -- parallel executor\n\
         that distributes tasks over CPUs or machines in a network.\n\
usage: paexec    [OPTIONS]\n\
       paexec -C [OPTIONS] cmd [args...]\n\
OPTIONS:\n\
  -h               give this help\n\
  -V               show version\n\
\n\
  -n <+num>        number of subprocesses to run\n\
  -n <nodes>       list of nodes separated by space character\n\
  -n <:filename>   filename containing a list of nodes, one node per line\n\
  -c <command>     set a command\n\
  -C               use free arguments as a command and its arguments\n\
  -t <transport>   set a transport program\n\
\n\
  -x               run command once per task\n\
  -X               implies -x and ignore calculator's stdout.\n\
  -y               magic line is used as an end-of-task marker\n\
  -0               change paexec to expect NUL character as\n\
                   a line separator instead of newline\n\
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
  -m   s=<success> set an alternative for 'success' message\n\
       f=<failure> set an alternative for 'failure' message\n\
       F=<fatal>   set an alternative for 'fatal' message\n\
       t=<EOT>     set an alternative for EOT marker\n\
       d=<delimiter>    set the delimiter for -g mode.\n\
                        The default is space character.\n\
       w=<weight>  set an alternative for 'weight:' marker\n\
\n\
  -J replstr\n\
                   execute command for each task, replacing\n\
                   one or more occurrences of replstr with the entire task.\n\
\n\
-n and -c are mandatory options\n\
\n\
");
}

static const char magic_eot [] = "HG>&OSO@#;L8N;!&.U4ZC_9X:0AF,2Y>SRXAD_7U&QZ5S>N^?Y,I=W?@5";

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

static const char *msg_success = "success";
static const char *msg_failure = "failure";
static const char *msg_fatal   = "fatal";
static const char *msg_weight  = "weight:";
static const char *msg_eot = NULL;
char msg_delim = ' '; /* also used in tasks.c */

static const char *shell = NULL;

static int resistant = 0;
static int resistance_timeout = 0;
static int resistance_last_restart = 0;

static int wait_mode = 0;

static int exec_mode = 0;

static int use_weights = 0;

char eol_char = '\n';

static char replstr[2] = "";

struct envvar_entry {
	SLIST_ENTRY(envvar_entry) entries;      /* List. */
	char* name;
	char* value;
};
static SLIST_HEAD(envvar_head, envvar_entry) envvars = SLIST_HEAD_INITIALIZER(envvars);

static void add_envvar (const char *name, const char *value)
{
	struct envvar_entry *val = calloc(1, sizeof (struct envvar_entry));
	val->name  = strdup (name);
	val->value = (value ? strdup (value) : NULL);
	SLIST_INSERT_HEAD(&envvars, val, entries);
}

static void assign_str (char **ptr, const char *str)
{
	size_t len = strlen (str);

	*ptr = xrealloc (*ptr, len+1);
	memcpy (*ptr, str, len+1);
}

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
	char buf [4000];
	snprintf (buf, sizeof (buf), "Bad input line: %s\n", line);

	err_fatal (buf);
}

static void init__read_graph_tasks (void)
{
	char *buf = NULL;
	size_t buf_sz = 0;

	ssize_t len = 0;

	int id1, id2;

	char *tok1, *tok2, *tok3, *tok;
	int tok_cnt;
	char *p;

	char buf_copy [2000];

	/* */
	if (debug){
		fprintf (stderr, "start: init__read_graph_tasks\n");
	}

	/* */
	if (!graph_mode){
		/* completely independent tasks */
		return;
	}

	/* reading all tasks with their dependancies */
	while (len = xgetdelim(&buf, &buf_sz, eol_char, stdin), len != -1){
		if (len > 0 && buf [len-1] == '\n'){
			buf [len-1] = 0;
			--len;
		}

		strncpy (buf_copy, buf, sizeof (buf_copy));

		tok1 = tok2 = tok3 = tok = NULL;
		tok_cnt = 0;

		for (p=buf; 1; ++p){
			char ch = *p;
			if (ch == msg_delim){
				*p = 0;
			}

			if (!tok)
				tok = p;

			if (*p == 0){
				if (!tok1){
					tok1 = tok;
					tok_cnt = 1;
				}else if (!tok2){
					tok2 = tok;
					tok_cnt = 2;
				}else if (!tok3){
					tok3 = tok;
					tok_cnt = 3;
				}else{
					bad_input_line (buf_copy);
				}
				tok = NULL;
			}

			if (!ch)
				break;
		}

		if (tok_cnt == 3 && !strcmp (tok1, msg_weight)){
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

	if (buf)
		free (buf);

	/* */
	if (debug){
		fprintf (stderr, "end: init__read_graph_tasks\n");
	}
}

static char const runner_function1[] = "run() { %s \"$1\"; }";
static char const runner_function2[] = "run() { %s; }";

static char* generate_run_command(void)
{
	static char run_command [4096];
	char *repl_ptr = NULL;

	if (replstr[0]){
		repl_ptr = arg_cmd;
		while (repl_ptr = strstr(repl_ptr, replstr), repl_ptr != NULL){
			repl_ptr[0] = '$';
			repl_ptr[1] = '1';
			repl_ptr += 2;
		}

		snprintf(run_command, sizeof(run_command), runner_function2, arg_cmd);
	}else{
		snprintf(run_command, sizeof(run_command), runner_function1, arg_cmd);
	}

	if (strlen(run_command) + 1 == sizeof(run_command)){
		err_fatal ("paexec: Internal error6! (buffer size)");
	}

//	fprintf(stderr, "run_command: %s\n", run_command);

	return run_command;
}

static void init__postproc_arg_cmd (void)
{
	char shq_cmd [4096];
	char cmd [4096];
	char tmp [4096];
	char env_str [4096]="";
	char tmp2 [4096]="";
	struct envvar_entry *p;

	if (exec_mode){
		char cond_cmd [4096] = "";

		if (exec_mode == 'x'){
			strlcpy(tmp, "  printf '%s\\n' \"$res\";", sizeof (tmp));
		}

		if (graph_mode){
			snprintf (
				cond_cmd, sizeof (cond_cmd),
				"if test $ex = 0; then echo '%s'; else echo '%s'; fi;",
				msg_success, msg_failure);
		}

		if (snprintf (cmd, sizeof (cmd),
				  "%s; IFS=; "
				  "while read -r f; do"
				  "  res=`run \"$f\"`;"
				  "  ex=$?;"
				  "  %s" /* printing result */
				  "  %s" /* condition. success/failure */
				  "  echo '%s';" /* EOT */
				  "done", generate_run_command(), tmp, cond_cmd, magic_eot) >= sizeof (cmd)){
			err_fatal ("paexec: Internal error7! (buffer size)");
		}

		xfree (arg_cmd);
		arg_cmd = xstrdup (cmd);
	}

	xshquote (arg_cmd, shq_cmd, sizeof (shq_cmd));

	/* env(1) arg for environment variables */
	add_envvar("PAEXEC_EOT", msg_eot);

	SLIST_FOREACH (p, &envvars, entries){
		xshquote ((p->value ? p->value : ""), tmp, sizeof (tmp));
		if (snprintf (tmp2, sizeof (tmp2), "%s=%s ", p->name, tmp) >= sizeof (tmp2)){
            err_fatal ("paexec: Internal error! (buffer size)");
        }
		strlcat (env_str, tmp2, sizeof (env_str));
	}

	/**/
	if (snprintf (cmd, sizeof (cmd), "env %s %s -c %s", env_str, shell, shq_cmd) >= sizeof (cmd)){
        err_fatal ("paexec: Internal error! (buffer size)");
    }
	xfree (arg_cmd);
	arg_cmd = xstrdup (cmd);

	/**/
	if (arg_transport){
		/* one more shquote(3) for ssh-like transport */
		xshquote (arg_cmd, shq_cmd, sizeof (shq_cmd));
		xfree (arg_cmd);
		arg_cmd = xstrdup (shq_cmd);
	}

	if (strlen (cmd) + 20 >= sizeof (shq_cmd)){
		err_fatal ("paexec: internal error, buffer size limit");
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

		if (arg_transport)
			snprintf (full_cmd, sizeof (full_cmd), "%s %s %s",
					  arg_transport, nodes [i], arg_cmd);
		else
			snprintf (full_cmd, sizeof (full_cmd), "%s", arg_cmd);

		if (debug){
			fprintf (stderr, "running cmd: %s\n", full_cmd);
		}

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

	if (busy [node]){
		busy [node] = 0;
		--busy_count;
		tasks__mark_task_as_failed (node2taskid [node]);
		--alive_nodes_count;
	}

	if (fd_in [node] >= 0)
		close (fd_in  [node]);
	if (fd_out [node] >= 0)
		close (fd_out [node]);

	fd_in  [node] = -1;
	fd_out [node] = -1;

	unblock_signals ();
	waitpid (pids [node], NULL, WNOHANG);
	block_signals ();

	pids [node] = (pid_t) -1;
}

static void init (void)
{
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
	init__read_graph_tasks ();

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

	/* */
	init__postproc_arg_cmd ();

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

void kill_childs (void)
{
	int i;

	if (!pids)
		return;

	for (i=0; i < nodes_count; ++i){
		if (pids [i] != (pid_t) -1){
			kill (pids [i], SIGTERM);
		}
	}
}

void wait_for_childs (void)
{
	int i;

	if (!pids)
		return;

	if (debug)
		printf ("wait for childs\n");

	for (i=0; i < nodes_count; ++i){
		if (pids [i] != (pid_t) -1){
			mark_node_as_dead (i);
		}
	}
}

static int find_free_node (void)
{
	int i;
	for (i=0; i < nodes_count; ++i){
		if (pids [i] != (pid_t) -1 && !busy [i])
			return i;
	}

	err_internal (__func__, "there is no free node");
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
		print_line (num, msg_eot);
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
			if (msg_fatal [0])
				print_line (n, msg_fatal);
			print_EOT (n);

			if (alive_nodes_count == 0 && !wait_mode){
				err_fatal ("all nodes failed");
			}
			return;
		}else{
			err_fatal_errno ("paexec: Sending task to the node failed:");
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
		err_fatal (msg);
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

				assert (bufsize_out [i] > size_out [i]);

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
					/* read error or unexpected end of file */
					if (resistant){
						FD_CLR (fd_out [i], &rset);
						mark_node_as_dead (i);
						if (msg_fatal [0])
							print_line (i, msg_fatal);
						print_EOT (i);

						if (alive_nodes_count == 0 && !wait_mode){
							err_fatal ("all nodes failed");
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

						err_fatal (msg);
					}
				}

				printed = 0;
				cnt += size_out [i];
				for (j=size_out [i]; j < cnt; ++j){
					if (buf_out_i [j] == '\n'){
						buf_out_i [j] = 0;

						curr_line = buf_out_i + printed;

						if (!strcmp (curr_line, msg_eot)){
							/* end of task marker */
							assert (busy [i] == 1);

							busy [i] = 0;
							--busy_count;

							if (end_of_stdin){
								close (fd_in [i]);
								fd_in [i] = -1;
							}

							/* EOT line means end-of-task */
							if (graph_mode){
								switch (ret_codes [i]){
									case rt_failure:
										print_header (i);
										tasks__delete_task_rec (node2taskid [i]);
										printf ("\n");
										break;
									case rt_success:
										tasks__delete_task (node2taskid [i], 0, 0);
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
								tasks__delete_task (node2taskid [i], 0, 0);
							}

							print_EOT (i);
							break;
						}

						if (graph_mode){
							if (!strcmp (curr_line, msg_success)){
								ret_codes [i] = rt_success;
							}else if (!strcmp (curr_line, msg_failure)){
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
}

static void check_msg (const char *msg)
{
	if (strpbrk (msg, "'\"")){
		err_fatal ("paexec: symbols ' and \" are not allowed in -m argument");
	}
}

static char *gen_cmd (int *argc, char ***argv)
{
	char cmd [4096];
	char *curr_token;
	size_t len;
	size_t curr_len;
	int i;

	len = 0;
	for (i=0; i < *argc; ++i){
		curr_token = (*argv) [i];
		if (replstr[0] && !strcmp(curr_token, replstr)){
			cmd[len+0] = '"';
			cmd[len+1] = '$';
			cmd[len+2] = '1';
			cmd[len+3] = '"';
			curr_len = 4;
		}else{
			curr_len = shquote (curr_token, cmd+len, sizeof (cmd)-len-1);
		}
		if (curr_len == (size_t)-1){
			err_fatal ("paexec: Internal error4! (buffer size)");
		}
		len += curr_len;
		cmd [len++] = ' ';

		if (len >= sizeof (cmd)-20){ /* 20 chars is enough for "$1" :-) */
			err_fatal ("paexec: Internal error5! (buffer size)");
		}
	}

	cmd [len++] = 0;

	assert (!arg_cmd);
	return xstrdup (cmd);
}

static void process_args (int *argc, char ***argv)
{
	int c;
	int mode_C = 0;

	/* leading + is for shitty GNU libc */
	static const char optstring [] = "+0c:CdeEghiIJ:lm:n:prst:VwW:xXyzZ:";

	while (c = getopt (*argc, *argv, optstring), c != EOF){
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
				assign_str (&arg_nodes, optarg);
				break;
			case 'c':
				mode_C = 0;
				arg_cmd = xstrdup (optarg);
				break;
			case 'C':
				mode_C = 1;
				break;
			case 't':
				optarg += strspn (optarg, " \t");
				if (optarg [0])
					assign_str (&arg_transport, optarg);
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
			case 'J':
				if (strlen(optarg) != 2){
					fprintf(stderr, "-Jreplstr argument must have 2-characters length");
					exit(2);
				}

				replstr[0] = optarg[0];
				replstr[1] = optarg[1];

				if (!msg_eot)
					msg_eot = magic_eot;
				if (!exec_mode)
					exec_mode = 'x';
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
				if (optarg [0] == 's' && optarg [1] == '='){
					msg_success = xstrdup (optarg+2);
					check_msg (msg_success);
				}else if (optarg [0] == 'f' && optarg [1] == '='){
					msg_failure = xstrdup (optarg+2);
					check_msg (msg_failure);
				}else if (optarg [0] == 'F' && optarg [1] == '='){
					msg_fatal = xstrdup (optarg+2);
					check_msg (msg_fatal);
				}else if (optarg [0] == 't' && optarg [1] == '='){
					msg_eot = xstrdup (optarg+2);
				}else if (optarg [0] == 'd' && optarg [1] == '='){
					if (optarg [2] != 0 && optarg [3] != 0){
						err_fatal ("paexec: bad argument for -md=. At most one character is allowed");
					}
					msg_delim = optarg [2];
				}else if (optarg [0] == 'w' && optarg [1] == '='){
					msg_weight = xstrdup (optarg+2);
				}else{
					err_fatal ("paexec: bad argument for -m");
				}

				break;
			case 'W':
				use_weights = atoi (optarg);
				graph_mode  = 1;
				break;
			case 'x':
				exec_mode = 'x';
				msg_eot = magic_eot;
				break;
			case 'X':
				exec_mode = 'X';
				msg_eot = magic_eot;
				break;
			case 'y':
				msg_eot = magic_eot;
				break;
			case '0':
				eol_char = '\0';
				break;
			default:
				usage ();
				exit (1);
		}
	}

	*argv += optind;
	*argc -= optind;

	if (!msg_eot){
		msg_eot = "";
	}

	if (mode_C){
		if (!*argc){
			err_fatal ("paexec: missing arguments. Run paexec -h for details");
		}

		arg_cmd = gen_cmd (argc, argv);
	}else{
		if (*argc){
			err_fatal ("paexec: extra arguments. Run paexec -h for details");
		}
	}

	if (!resistance_timeout && wait_mode){
		err_fatal ("paexec: -w is useless without -Z");
	}

	if (arg_nodes){
		if (arg_nodes [0] == '+'){
			free (arg_transport);
			arg_transport = NULL;
		}

		nodes_create (arg_nodes);
	}else{
		err_fatal ("paexec: -n option is mandatory!");
	}

	if (!arg_cmd){
		err_fatal ("paexec: -c option is mandatory!");
	}

	if (use_weights < 0 || use_weights > 2){
		err_fatal ("paexec: Only -W1 and -W2 are supported!");
	}

	if (arg_transport && !arg_transport [0]){
		free (arg_transport);
		arg_transport = NULL;
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

	nodes_destroy ();

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

static void init_env (void)
{
	char *tok;
	char *env_msg_eot   = getenv ("PAEXEC_EOT");
	if (env_msg_eot)
		msg_eot = env_msg_eot;

	char *env_bufsize   = getenv ("PAEXEC_BUFSIZE");
	if (env_bufsize)
		initial_bufsize = atoi (env_bufsize);

	char *env_transport = getenv ("PAEXEC_TRANSPORT");
	if (env_transport)
		assign_str (&arg_transport, env_transport);

	char *env_nodes = getenv ("PAEXEC_NODES");
	if (env_nodes)
		assign_str (&arg_nodes, env_nodes);

	char *env_shell   = getenv ("PAEXEC_SH");
	if (env_shell)
		shell = env_shell;
	else
		shell = "/bin/sh";

	char *paexec_env = getenv ("PAEXEC_ENV");
	if (paexec_env){
		for (tok = strtok (paexec_env, " ,");
			 tok;
			 tok = strtok (NULL, " ,"))
		{
			add_envvar (tok, getenv (tok));
		}
	}
}

int main (int argc, char **argv)
{
	int i;
	pid_t pid;
	int status;

	block_signals ();

	init_env ();

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

	set_sig_handler (SIGCHLD, SIG_DFL);
	set_sig_handler (SIGALRM, SIG_IGN);

	unblock_signals ();

	while (pid = waitpid (-1, &status, WNOHANG), pid > 0){
	}

	return 0;
}
