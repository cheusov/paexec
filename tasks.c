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

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <maa.h>

#include "tasks.h"
#include "wrappers.h"

int *deleted_tasks = NULL;

int *arcs_from = NULL;
int *arcs_to   = NULL;
int arcs_count = 0;

int *tasks_graph_deg = NULL;

int tasks_count = 1; /* 0 - special meaning, not task ID */
int remained_tasks_count = 0;

hsh_HashTable tasks;

int poset_of_tasks  = 0;
const char ** id2task = NULL;

char *current_task     = NULL;
size_t current_task_sz = 0;

int current_taskid = 0;

static int *failed_taskids     = NULL;
static int failed_taskids_count = 0;

void init_tasks (void)
{
	tasks = hsh_create (NULL, NULL);
}

void destroy_tasks (void)
{
	if (poset_of_tasks){
		hsh_destroy (tasks);
	}

	if (deleted_tasks)
		xfree (deleted_tasks);
}

void delete_task (int task, int print_task)
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

void delete_task_rec (int task)
{
	memset (deleted_tasks, 0, tasks_count * sizeof (*deleted_tasks));

	delete_task_rec2 (task);
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

	current_taskid = num;
	tasks_graph_deg [num] = -1;
	return id2task [num];
}

static const char * get_new_task_from_stdin (void)
{
	const char *task = NULL;
	size_t sz = 0;

	task = xfgetln (stdin, &sz);

	return task;
}

const char *get_new_task (void)
{
	const char *task = NULL;
	size_t task_len = 0;

//	taskid = -1;
	if (failed_taskids_count > 0){
		current_taskid = failed_taskids [--failed_taskids_count];
		task = id2task [current_taskid];
		assert (task);
	}else if (poset_of_tasks){
		task = get_new_task_from_graph ();
	}else{
		task = get_new_task_from_stdin ();
	}

	if (!task)
		return NULL;

	task_len = strlen (task);

	if (task_len >= current_task_sz){
		current_task_sz = task_len+1;
		current_task = (char *) xrealloc (current_task, current_task_sz);
	}

	memcpy (current_task, task, task_len+1);

	if (!poset_of_tasks){
		++current_taskid;
	}

	return current_task;
}

typedef union {
	int integer;
	const void *ptr;
} int_ptr_union_t;

int add_task (const char *s)
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

		deleted_tasks = (int *) xrealloc (
			deleted_tasks, tasks_count * sizeof (*deleted_tasks));
		deleted_tasks [tasks_count-1] = -1;

		tasks_graph_deg = (int *) xrealloc (
			tasks_graph_deg, tasks_count * sizeof (*tasks_graph_deg));
		tasks_graph_deg [tasks_count-1] = 0;

		return tasks_count-1;
	}
}

void add_task_arc (int task_from, int task_to)
{
	++arcs_count;
	arcs_from = (int *) xrealloc (arcs_from,
								  arcs_count * sizeof (*arcs_from));
	arcs_to = (int *) xrealloc (arcs_to,
								arcs_count * sizeof (*arcs_to));

	arcs_from [arcs_count-1] = task_from;
	arcs_to [arcs_count-1]   = task_to;

	++tasks_graph_deg [task_to];
}

void mark_task_as_failed (int id)
{
	failed_taskids = (int *) xrealloc (
		failed_taskids,
		(failed_taskids_count+1) * sizeof (*failed_taskids));

	failed_taskids [failed_taskids_count++] = id;
}

static int *check_cycles__stack;
static int *check_cycles__mark;

static void check_cycles__outgoing (int stack_sz)
{
	assert (stack_sz > 0);

	int from = check_cycles__stack [stack_sz-1];
	int i, j;
	int s, t;
	int loop;

	assert (check_cycles__mark [from] == 0);
	check_cycles__mark [from] = 2; /* currently in the path */

	for (i=0; i < arcs_count; ++i){
		if (arcs_from [i] != from)
			continue;

		assert (stack_sz < tasks_count);

		int to = arcs_to [i];
		check_cycles__stack [stack_sz] = to;

		switch (check_cycles__mark [to]){
			case 2:
				loop = 0;
				fprintf (stderr, "Cyclic dependancy detected:\n");
				for (j=1; j <= stack_sz; ++j){
					s = check_cycles__stack [j-1];
					t = check_cycles__stack [j];

					if (!loop && s != to)
						continue;

					loop = 1;
					fprintf (stderr, "  %s -> %s\n", id2task [s], id2task [t]);
				}

				exit (1);
			case 0:
				check_cycles__outgoing (stack_sz + 1);
				break;
			case 1:
				break;
			default:
				abort (); /* this should not happen */
		}
	}

	check_cycles__mark [from] = 1; /* already seen */
}

void init__check_cycles (void)
{
	int i;

	check_cycles__stack = xmalloc (
		tasks_count * sizeof (check_cycles__stack [0]));

	check_cycles__mark = xmalloc (
		tasks_count * sizeof (check_cycles__mark [0]));
	memset (check_cycles__mark, 0,
		tasks_count * sizeof (check_cycles__mark [0]));

	/* */
	for (i=0; i < tasks_count; ++i){
		switch (check_cycles__mark [i]){
			case 0:
				check_cycles__stack [0] = i;
				check_cycles__outgoing (1);
				break;
			case 1:
				break;
			case 2:
				abort (); /* this should not happen */
		}
	}

	xfree (check_cycles__mark);
	xfree (check_cycles__stack);
}
