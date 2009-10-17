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

void init_tasks (int nodes_count);
int add_task (char *s);
void add_task_arc (int task_from, int task_to);
void init__check_cycles (void);
void delete_task (int task, int print_task);
void delete_task_rec (int task);
void destroy_tasks (void);
const char *get_new_task (void);
void mark_task_as_failed (int taskid);

#endif // _TASKS_H_
