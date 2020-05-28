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

#ifndef _TASKS_H_
#define _TASKS_H_

/* The following variables are read-only, do set them directly! */

/* a number of tasks */
extern int tasks_count;
/* true "paexec -s" */
extern int graph_mode;
/* last read task and its id */
extern char *current_task;
extern int current_taskid;
/* a number of tasks to be done */
extern int remained_tasks_count;
/* a number of tasks with FATAL failure (e.g. connection lost) */
extern int failed_taskids_count;

void tasks__init(void);
int  tasks__add_task(char *s, int weight);
void tasks__add_task_arc(int task_from, int task_to);
void tasks__check_for_cycles(void);
void tasks__delete_task(int task, int print_task, int with_prefix);
void tasks__delete_task_rec(int task);
void tasks__destroy(void);
const char *tasks__get_new_task(void);
void tasks__mark_task_as_failed(int taskid);
void tasks__make_sum_weights(void);
void tasks__make_max_weights(void);
void tasks__print_sum_weights(void);

#endif // _TASKS_H_
