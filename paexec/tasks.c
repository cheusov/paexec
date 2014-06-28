/*
 * Copyright (c) 2007-2014 Aleksey Cheusov <vle@gmx.net>
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

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "decls.h"

#include "tasks.h"
#include "wrappers.h"

extern char msg_delim; /* from paexec.c */

static int *deleted_tasks = NULL;

struct task_entry {
	SLIST_ENTRY (task_entry) entries;      /* List. */
	int task_id;
};
static SLIST_HEAD (task_head, task_entry) *arcs_outg = NULL, *arcs_inco = NULL;

static int arcs_count = 0;

static int *tasks_graph_deg = NULL;

int tasks_count = 1; /* 0 - special meaning, not task ID */
int remained_tasks_count = 0;

typedef struct task_struct {
	RB_ENTRY(task_struct) linkage;
	char *task;
	int task_id;
} task_t;

static int tasks_cmp (task_t *a, task_t *b)
{
	return strcmp (a->task, b->task);
}

static RB_HEAD (tasks_entries, task_struct) tasks = RB_INITIALIZER(&tasks);

RB_PROTOTYPE (tasks_entries, task_struct, linkage, tasks_cmp)
RB_GENERATE (tasks_entries, task_struct, linkage, tasks_cmp)

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
}

void tasks__destroy (void)
{
	task_t *data, *next;
	data = (task_t *) RB_MIN (tasks_entries, &tasks);
	while (data){
		next = (task_t *) RB_NEXT (tasks_entries, &tasks, data);
		RB_REMOVE (tasks_entries, &tasks, data);
		free (data->task);
		free (data);

		data = next;
	}

	if (deleted_tasks)
		xfree (deleted_tasks);

	if (id2weight)
		xfree (id2weight);
	if (id2sum_weight)
		xfree (id2sum_weight);
}

void tasks__delete_task (int task, int print_task, int with_prefix)
{
	struct task_entry *p;
	int to;

	assert (task < tasks_count);
	assert (task >= 0);

	if (!graph_mode){
		if (id2task [task]){
			xfree (id2task [task]);
			id2task [task] = NULL;
		}
	}

	if (graph_mode){
		SLIST_FOREACH (p, &arcs_outg [task], entries){
			to = p->task_id;
			if (tasks_graph_deg [to] > 0)
				--tasks_graph_deg [to];
		}

		if (tasks_graph_deg [task] >= -1){
			tasks_graph_deg [task] = -2;

			--remained_tasks_count;
		}
	}

	if (print_task){
		if (!deleted_tasks [task]){
			if (with_prefix)
				printf ("%c", msg_delim);

			printf ("%s", id2task [task]);

			deleted_tasks [task] = 1;
		}
	}
}

static void delete_task_rec2 (int task, int with_prefix)
{
	struct task_entry *p;
	int to;

	assert (task >= 0);

	tasks__delete_task (task, 1, with_prefix);

	SLIST_FOREACH (p, &arcs_outg [task], entries){
		to = p->task_id;
		delete_task_rec2 (to, 1);
	}
}

void tasks__delete_task_rec (int task)
{
	memset (deleted_tasks, 0, tasks_count * sizeof (*deleted_tasks));

	delete_task_rec2 (task, 0);
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
	static char *task = NULL;
	static size_t task_sz = 0;

	ssize_t sz = 0;

	sz = xgetline (&task, &task_sz, stdin);
	if (sz == -1)
		return NULL;

	if (sz > 0 && task [sz-1] == '\n')
		task [sz-1] = 0;

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

int tasks__add_task (char *s, int weight)
{
	task_t *n = malloc (sizeof (*n));
	task_t *data;
	int task_id;

	n->task = s;

	data = RB_INSERT (tasks_entries, &tasks, n);
	if (data){
		task_id = data->task_id;
		if (id2weight [task_id] < weight){
			id2weight     [task_id] = weight;
			id2sum_weight [task_id] = weight;
		}
		free (s);
		free (n);
		return task_id;
	}else{
		n->task_id = tasks_count;

		++tasks_count;
		++remained_tasks_count;

		arcs_outg = (struct task_head *) xrealloc (
			arcs_outg, tasks_count * sizeof (*arcs_outg));
		SLIST_INIT(&arcs_outg [tasks_count-1]);

		arcs_inco = (struct task_head *) xrealloc (
			arcs_inco, tasks_count * sizeof (*arcs_inco));
		SLIST_INIT(&arcs_inco [tasks_count-1]);

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
	struct task_entry *p1,*p2;
	++arcs_count;

	p1 = xmalloc (sizeof (*p1));
	memset (p1, 0, sizeof (*p1));
	p1->task_id = task_to;
	SLIST_INSERT_HEAD (&arcs_outg [task_from], p1, entries);

	p2 = xmalloc (sizeof (*p2));
	memset (p2, 0, sizeof (*p2));
	p2->task_id = task_from;
	SLIST_INSERT_HEAD (&arcs_inco [task_to], p2, entries);

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
	struct task_entry *p;
	int j;
	int s, t;
	int loop;
	int to;
	int from = check_cycles__stack [stack_sz-1];

	assert (stack_sz > 0);

	assert (check_cycles__mark [from] == 0);
	check_cycles__mark [from] = 2; /* currently in the path */

	SLIST_FOREACH (p, &arcs_outg [from], entries){
		to = p->task_id;
		assert (stack_sz < tasks_count);

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

static char *seen = NULL;

static void tasks__make_sum_weights_rec (int *accu_w, int task)
{
	struct task_entry *p;
	int to;

	seen [task] = 1;
	SLIST_FOREACH (p, &arcs_outg [task], entries){
		to = p->task_id;
		if (seen [to])
			continue;

		tasks__make_sum_weights_rec (accu_w, to);
		*accu_w += id2weight [to];
		seen [to] = 1;
	}
}

void tasks__make_sum_weights (void)
{
	int i;

	if (!graph_mode)
		return;

	seen = (char *) xmalloc (tasks_count);

	for (i=1; i < tasks_count; ++i){
		memset (seen, 0, tasks_count);
		tasks__make_sum_weights_rec (&id2sum_weight [i], i);
	}

	xfree (seen);
}

static void tasks__make_max_weights_rec (int *accu_w, int task)
{
	struct task_entry *p;
	int to;
	int curr_w;

	seen [task] = 1;
	SLIST_FOREACH (p, &arcs_outg [task], entries){
		to = p->task_id;
		if (seen [to])
			continue;

		tasks__make_max_weights_rec (accu_w, to);
		curr_w = id2weight [to];
		if (*accu_w < curr_w)
			*accu_w = curr_w;
		seen [to] = 1;
	}
}

void tasks__make_max_weights (void)
{
	int i;

	if (!graph_mode)
		return;

	seen = (char *) xmalloc (tasks_count);

	for (i=1; i < tasks_count; ++i){
		memset (seen, 0, tasks_count);

		tasks__make_max_weights_rec (&id2sum_weight [i], i);
	}

	xfree (seen);
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
