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

static int *deleted_tasks = NULL;

static int *arcs_from = NULL;
static int *arcs_to   = NULL;
static int arcs_count = 0;

static int *tasks_graph_deg = NULL;

int tasks_count = 1; /* 0 - special meaning, not task ID */
int remained_tasks_count = 0;

static hsh_HashTable tasks;

int graph_mode  = 0;
/* numeric task id to textual representation*/
static char ** id2task = NULL;
/* numeric task id to weight */
static int *id2weight = NULL;
static int *id2sum_weight = NULL;

char *current_task     = NULL;
size_t current_task_sz = 0;

int current_taskid = 0;

static int *failed_taskids     = NULL;
int failed_taskids_count = 0;

void tasks__init (void)
{
	tasks = hsh_create (NULL, NULL);
}

void tasks__destroy (void)
{
	if (tasks){
		hsh_destroy (tasks);
	}

	if (deleted_tasks)
		xfree (deleted_tasks);

	if (id2weight)
		xfree (id2weight);
	if (id2sum_weight)
		xfree (id2sum_weight);
}

void tasks__delete_task (int task, int print_task)
{
	int i, to;

	assert (task < tasks_count);
	assert (task >= 0);

	if (!graph_mode){
		if (id2task [task]){
			xfree (id2task [task]);
			id2task [task] = NULL;
		}
	}

	if (graph_mode){
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

	tasks__delete_task (task, 1);

	for (i=0; i < arcs_count; ++i){
		if (arcs_from [i] == task){
			to = arcs_to [i];
			delete_task_rec2 (to);
		}
	}
}

void tasks__delete_task_rec (int task)
{
	memset (deleted_tasks, 0, tasks_count * sizeof (*deleted_tasks));

	delete_task_rec2 (task);
}

static int get_new_task_num_from_graph (void)
{
	int i;
	int best_weight = 0;
	int best_task = -1;

	for (i=1; i < tasks_count; ++i){
		assert (tasks_graph_deg [i] >= -2);

		if (tasks_graph_deg [i] == 0){
			if (id2sum_weight [i] > best_weight){
				best_weight = id2sum_weight [i];
				best_task = i;
			}
		}
	}

	return best_task;
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
	if (!task)
		return NULL;

	current_taskid = tasks_count++;

	id2task = (char **) xrealloc (
		id2task, tasks_count * sizeof (*id2task));
	id2task [current_taskid] = xstrdup(task);

	return task;
}

const char *tasks__get_new_task (void)
{
	const char *task = NULL;
	size_t task_len = 0;

	if (failed_taskids_count > 0){
		current_taskid = failed_taskids [--failed_taskids_count];
		assert (current_taskid < tasks_count);
		task = id2task [current_taskid];
		assert (task);
	}else if (graph_mode){
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

	return current_task;
}

typedef union {
	int integer;
	const void *ptr;
} int_ptr_union_t;

int tasks__add_task (char *s, int weight)
{
	int_ptr_union_t r;

	r.integer = 0;
	r.ptr = hsh_retrieve (tasks, s);
	if (r.ptr){
		if (id2weight [r.integer] < weight){
			id2weight     [r.integer] = weight;
			id2sum_weight [r.integer] = weight;
		}
		return r.integer;
	}else{
		r.ptr = NULL;
		r.integer = tasks_count;
		hsh_insert (tasks, s, r.ptr);

		++tasks_count;
		++remained_tasks_count;

		id2task = (char **) xrealloc (
			id2task, tasks_count * sizeof (*id2task));
		id2task [tasks_count-1] = s;

		id2weight = (int *) xrealloc (
			id2weight, tasks_count * sizeof (*id2weight));
		id2weight [tasks_count-1] = weight;

		id2sum_weight = (int *) xrealloc (
			id2sum_weight, tasks_count * sizeof (*id2sum_weight));
		id2sum_weight [tasks_count-1] = weight;

		deleted_tasks = (int *) xrealloc (
			deleted_tasks, tasks_count * sizeof (*deleted_tasks));
		deleted_tasks [tasks_count-1] = -1;

		tasks_graph_deg = (int *) xrealloc (
			tasks_graph_deg, tasks_count * sizeof (*tasks_graph_deg));
		tasks_graph_deg [tasks_count-1] = 0;

		return tasks_count-1;
	}
}

void tasks__add_task_arc (int task_from, int task_to)
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

void tasks__mark_task_as_failed (int id)
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

void tasks__check_for_cycles (void)
{
	int i;

	check_cycles__stack = xmalloc (
		tasks_count * sizeof (check_cycles__stack [0]));

	check_cycles__mark = xmalloc (
		tasks_count * sizeof (check_cycles__mark [0]));
	memset (check_cycles__mark, 0,
		tasks_count * sizeof (check_cycles__mark [0]));

	/* */
	for (i=1; i < tasks_count; ++i){
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

static int *tasks_seen;
static int curr_leaf_node;

static void add_weight_rec (int task_id, int accu)
{
	/* add accu to all parents' m_id2sum_weight but only once */

	int i;
	int id;
	int temp_accu;

	if (!tasks_seen [task_id])
		tasks_seen [task_id] = curr_leaf_node;

	for (i=0; i < arcs_count; ++i){
		if (arcs_to [i] == task_id){
			id = arcs_from [i];

			if (tasks_seen [id] == curr_leaf_node)
				continue;

//			fprintf (stderr, " sum_weight [%s] += %d\n", id2task [id], accu);
			id2sum_weight [id] += accu;

			if (tasks_seen [id])
				temp_accu = accu;
			else
				temp_accu = accu + id2weight [id];

			add_weight_rec (id, temp_accu);
		}
	}

	tasks_seen [task_id] = curr_leaf_node;
}

void tasks__make_sum_weights (void)
{
	int j;
	int leaf = 0;
	size_t seen_arr_sz;

	if (!graph_mode)
		return;

	seen_arr_sz = tasks_count * sizeof (*tasks_seen);
	tasks_seen = (int *) xmalloc (seen_arr_sz);
	memset (tasks_seen, 0, seen_arr_sz);

	for (curr_leaf_node=1; curr_leaf_node < tasks_count; ++curr_leaf_node){
		/* leaf node? */
		leaf = 1;

		for (j=0; j < arcs_count; ++j){
			if (arcs_from [j] == curr_leaf_node){
				leaf = 0;
				break;
			}
		}

		if (!leaf)
			continue;

		/* yes */
//		fprintf (stderr, "-%s:\n", id2task [curr_leaf_node]);
		add_weight_rec (curr_leaf_node, id2sum_weight [curr_leaf_node]);
	}

	xfree (tasks_seen);
}

void tasks__print_sum_weights (void)
{
	int i;

	if (!graph_mode)
		return;

	for (i=1; i < tasks_count; ++i){
		fprintf (stderr, "weight [%s]=%d\n", id2task [i], id2weight [i]);
		fprintf (stderr, "sum_weight [%s]=%d\n", id2task [i], id2sum_weight [i]);
	}
}
