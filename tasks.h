#ifndef _TASKS_H_
#define _TASKS_H_

/* The following variables are read-only, do set them directly! */

/* a number of tasks */
extern int tasks_count;
/* numeric task id to textual representation*/
extern char ** id2task;
/* true "paexec -s" */
extern int graph_mode;
/* last read task and its id */
extern char *current_task;
extern int current_taskid;
/* a number of tasks to be done */
extern int remained_tasks_count;
/* a number of tasks with FATAL failure (e.g. connection lost) */
extern int failed_taskids_count;

void tasks__init (void);
int  tasks__add_task (char *s);
void tasks__add_task_arc (int task_from, int task_to);
void tasks__check_for_cycles (void);
void tasks__delete_task (int task, int print_task);
void tasks__delete_task_rec (int task);
void tasks__destroy (void);
const char *tasks__get_new_task (void);
void tasks__mark_task_as_failed (int taskid);

#endif // _TASKS_H_
