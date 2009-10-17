#ifndef _TASKS_H_
#define _TASKS_H_

extern int tasks_count;
extern const char ** id2task;
extern int poset_of_tasks;
extern char *current_task;
extern int taskid;
extern int remained_tasks_count;

void init_tasks (void);
int add_task (const char *s);
void add_task_arc (int task_from, int task_to);
void init__check_cycles (void);
void delete_task (int task, int print_task);
void delete_task_rec (int task);
void destroy_tasks (void);
const char *get_new_task (void);
void mark_task_as_failed (int taskid);

#endif // _TASKS_H_
